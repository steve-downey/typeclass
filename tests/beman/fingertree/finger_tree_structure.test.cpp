// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree/tree/finger_tree_structure.hpp>
#include <beman/fingertree/tree/finger_tree_structure.hpp> // Re-inclusion check

#include <beman/fingertree/tree/finger_tree_pmr.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace ft = beman::fingertree::tree;

// The measure is the text itself, so it records element order and nesting:
// any regrouping or reordering of a combine is observable.
struct Text {
    using value_type = std::string;
    auto identity() const -> std::string { return {}; }
    auto combine(const std::string& a, const std::string& b) const -> std::string { return a + b; }
};
struct Spell {
    auto operator()(const int& value) const -> std::string { return std::to_string(value) + ";"; }
};
using Spelled = ft::FingerTree<int, Text, Spell>;

// A monoid with state: the modulus travels with the tree.
struct ModSum {
    using value_type    = std::size_t;
    std::size_t modulus = 97;
    auto        identity() const -> std::size_t { return 0; }
    auto        combine(std::size_t a, std::size_t b) const -> std::size_t { return (a + b) % modulus; }
};
struct Weight {
    auto operator()(const int& value) const -> std::size_t { return static_cast<std::size_t>(value); }
};
using Weighted = ft::FingerTree<int, ModSum, Weight>;

// Counts every copy of a payload.
struct Counted {
    inline static int copies = 0;
    int               value;
    explicit Counted(int v) : value(v) {}
    Counted(const Counted& other) : value(other.value) { ++copies; }
    Counted(Counted&&)                         = default;
    auto operator=(const Counted&) -> Counted& = default;
    auto operator=(Counted&&) -> Counted&      = default;
};

template <class TREE>
auto counting(int n) -> TREE {
    TREE tree;
    for (int i = 0; i < n; ++i)
        tree = tree.snoc(i);
    return tree;
}

// Render a tree's real shape. Digits are bracketed, the spine sits between
// bars, nodes are angle-bracketed: order and nesting are both visible.
template <class NODE>
auto show(const NODE& node) -> std::string {
    if (node.form() == ft::FingerTreeNodeForm::LEAF)
        return std::to_string(node.value());
    std::string out = "<";
    for (const auto& child : node.children())
        out += show(child) + " ";
    out.back() = '>';
    return out;
}

template <class TREE>
auto show(const ft::FingerTreeLevel<TREE>& level) -> std::string {
    switch (level.form()) {
    case ft::FingerTreeForm::EMPTY:
        return "()";
    case ft::FingerTreeForm::SINGLE:
        return "(" + show(level.element()) + ")";
    case ft::FingerTreeForm::DEEP:
        break;
    }
    std::string out = "[";
    for (const auto& node : level.left())
        out += show(node) + " ";
    out += "| " + show(level.spine()) + " |";
    for (const auto& node : level.right())
        out += " " + show(node);
    return out + "]";
}

// Check every structural invariant the representation promises and collect
// the payloads in the order the structure presents them.
template <class TREE>
struct Audit {
    typename TREE::monoid_type             monoid;
    std::vector<typename TREE::value_type> payloads;
    bool                                   ok = true;

    using Level = ft::FingerTreeLevel<TREE>;
    using Node  = typename Level::node_type;
    using Tag   = typename TREE::tag_type;

    auto node(const Node& n, std::size_t height) -> Tag {
        ok = ok && n.height() == height;
        if (n.form() == ft::FingerTreeNodeForm::LEAF) {
            ok = ok && height == 0 && n.children().empty();
            payloads.push_back(n.value());
            return n.measure();
        }
        const auto children = n.children();
        ok        = ok && height > 0 && children.size() == (n.form() == ft::FingerTreeNodeForm::NODE2 ? 2u : 3u);
        Tag total = monoid.identity();
        for (const auto& child : children)
            total = monoid.combine(total, node(child, height - 1));
        ok = ok && total == n.measure();
        return n.measure();
    }

    auto level(const Level& l, std::size_t depth) -> Tag {
        ok = ok && (l.empty() || l.level() == depth);
        switch (l.form()) {
        case ft::FingerTreeForm::EMPTY:
            ok = ok && l.empty() && l.measure() == monoid.identity();
            return l.measure();
        case ft::FingerTreeForm::SINGLE:
            ok = ok && node(l.element(), depth) == l.measure();
            return l.measure();
        case ft::FingerTreeForm::DEEP:
            break;
        }
        const auto left  = l.left();
        const auto right = l.right();
        ok               = ok && !left.empty() && left.size() <= 4 && !right.empty() && right.size() <= 4;
        Tag total        = monoid.identity();
        for (const auto& n : left)
            total = monoid.combine(total, node(n, depth));
        total = monoid.combine(total, level(l.spine(), depth + 1));
        for (const auto& n : right)
            total = monoid.combine(total, node(n, depth));
        ok = ok && total == l.measure();
        return l.measure();
    }
};

template <class TREE>
auto audit(const TREE& tree) -> bool {
    Audit<TREE> a{tree.get_monoid(), {}, true};
    const auto  measure = a.level(ft::FingerTreeLevel<TREE>(tree), 0);
    return a.ok && measure == tree.measure() && a.payloads == tree.flatten();
}

// Rebuild a tree node for node through the builder, copying each payload.
template <class TREE>
auto rebuild(const ft::FingerTreeStructureBuilder<TREE>&          builder,
             const typename ft::FingerTreeLevel<TREE>::node_type& node) ->
    typename ft::FingerTreeLevel<TREE>::node_type {
    const auto children = node.children();
    if (children.empty())
        return builder.leaf(node.value());
    if (children.size() == 2)
        return builder.node2(rebuild(builder, children[0]), rebuild(builder, children[1]));
    return builder.node3(rebuild(builder, children[0]), rebuild(builder, children[1]), rebuild(builder, children[2]));
}

template <class TREE>
auto rebuild(const ft::FingerTreeStructureBuilder<TREE>& builder, const ft::FingerTreeLevel<TREE>& level)
    -> ft::FingerTreeLevel<TREE> {
    switch (level.form()) {
    case ft::FingerTreeForm::EMPTY:
        return builder.empty();
    case ft::FingerTreeForm::SINGLE:
        return builder.single(rebuild(builder, level.element()));
    case ft::FingerTreeForm::DEEP:
        break;
    }
    typename ft::FingerTreeLevel<TREE>::digit_type left;
    typename ft::FingerTreeLevel<TREE>::digit_type right;
    for (const auto& node : level.left())
        left.push_back(rebuild(builder, node));
    for (const auto& node : level.right())
        right.push_back(rebuild(builder, node));
    return builder.deep(std::move(left), rebuild(builder, level.spine()), std::move(right));
}

// A resource that counts what it hands out and forwards upstream.
class CountingResource : public std::pmr::memory_resource {
    std::pmr::memory_resource* d_upstream;

  public:
    std::size_t allocations = 0;
    explicit CountingResource(std::pmr::memory_resource* upstream) : d_upstream(upstream) {}

  private:
    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override {
        ++allocations;
        return d_upstream->allocate(bytes, alignment);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        d_upstream->deallocate(p, bytes, alignment);
    }
    auto do_is_equal(const std::pmr::memory_resource& other) const noexcept -> bool override { return this == &other; }
};

// Any allocation that reaches the default resource while this is alive
// throws, so a build that strays from its own resource fails loudly.
class ForbidDefaultResource {
    std::pmr::memory_resource* d_previous;

  public:
    ForbidDefaultResource() : d_previous(std::pmr::set_default_resource(std::pmr::null_memory_resource())) {}
    ~ForbidDefaultResource() { std::pmr::set_default_resource(d_previous); }
    ForbidDefaultResource(const ForbidDefaultResource&)                    = delete;
    auto operator=(const ForbidDefaultResource&) -> ForbidDefaultResource& = delete;
};

} // namespace

TEST_CASE("FingerTreeStructure - FormsAtTheDigitBoundaries", "[fingertree::structure]") {
    using Tree  = ft::FingerTree<int>;
    using Level = ft::FingerTreeLevel<Tree>;

    CHECK(Level(Tree{}).form() == ft::FingerTreeForm::EMPTY);
    CHECK(show(Level(Tree{})) == "()");

    const auto one = counting<Tree>(1);
    CHECK(Level(one).form() == ft::FingerTreeForm::SINGLE);
    CHECK(Level(one).element().form() == ft::FingerTreeNodeForm::LEAF);
    CHECK(show(Level(one)) == "(0)");

    // snoc fills the right digit to four, then pushes a Node3 down the spine.
    CHECK(show(Level(counting<Tree>(2))) == "[0 | () | 1]");
    CHECK(show(Level(counting<Tree>(5))) == "[0 | () | 1 2 3 4]");
    CHECK(show(Level(counting<Tree>(6))) == "[0 | (<1 2 3>) | 4 5]");
    CHECK(show(Level(counting<Tree>(9))) == "[0 | [<1 2 3> | () | <4 5 6>] | 7 8]");

    // from_sequence chooses a different shape for the same sequence: both
    // digits full, no spine. Structural access reports what is there.
    const auto bulk = Tree::from_sequence({0, 1, 2, 3, 4, 5, 6, 7});
    CHECK(show(Level(bulk)) == "[0 1 2 3 | () | 4 5 6 7]");
    CHECK(bulk == counting<Tree>(8));
    CHECK(show(Level(bulk)) != show(Level(counting<Tree>(8))));
}

TEST_CASE("FingerTreeStructure - NodeTwoAndNodeThreeBothAppear", "[fingertree::structure]") {
    using Tree  = ft::FingerTree<int>;
    using Level = ft::FingerTreeLevel<Tree>;
    // Appending regroups the inner digits, which is where Node2 comes from.
    const auto left   = counting<Tree>(5);
    const auto joined = left.append(left);
    const auto shape  = show(Level(joined));
    CHECK(shape == "[0 | [<1 2 3> | () | <4 0>] | 1 2 3 4]");
    const auto spine = Level(joined).spine();
    CHECK(spine.left().front().form() == ft::FingerTreeNodeForm::NODE3);
    CHECK(spine.right().front().form() == ft::FingerTreeNodeForm::NODE2);
    CHECK(spine.right().front().height() == 1);
    CHECK(joined.flatten() == std::vector<int>{0, 1, 2, 3, 4, 0, 1, 2, 3, 4});
}

TEST_CASE("FingerTreeStructure - InvariantsHoldAcrossSizesAndConstructions", "[fingertree::structure]") {
    using Tree = ft::FingerTree<int>;
    for (int n = 0; n <= 130; ++n) {
        CAPTURE(n);
        std::vector<int> values;
        Tree             by_cons;
        for (int i = 0; i < n; ++i) {
            values.push_back(i);
            by_cons = by_cons.cons(n - 1 - i);
        }
        const auto by_snoc = counting<Tree>(n);
        const auto bulk    = Tree::from_sequence(values);
        CHECK(audit(by_snoc));
        CHECK(audit(by_cons));
        CHECK(audit(bulk));
        CHECK(audit(by_snoc.append(bulk)));
        if (n > 2) {
            const auto parts = bulk.split_at_measure(static_cast<std::size_t>(n / 2));
            CHECK(audit(parts.d_left));
            CHECK(audit(parts.d_right));
        }
    }
}

TEST_CASE("FingerTreeStructure - SeveralSpineLevels", "[fingertree::structure]") {
    using Tree      = ft::FingerTree<int>;
    using Level     = ft::FingerTreeLevel<Tree>;
    const auto tree = counting<Tree>(2000);
    CHECK(audit(tree));

    std::size_t depth = 0;
    for (auto level = Level(tree); level.form() == ft::FingerTreeForm::DEEP; level = level.spine()) {
        CHECK(level.level() == depth);
        CHECK(level.left().front().height() == depth);
        CHECK(level.is_sequence() == (depth == 0));
        ++depth;
    }
    CHECK(depth == tree.spine_depth());
    CHECK(depth >= 5);
}

TEST_CASE("FingerTreeStructure - NoncommutativeMeasureIsReadInOrder", "[fingertree::structure]") {
    Spelled     tree;
    std::string expected;
    for (int i = 0; i < 40; ++i) {
        tree = tree.snoc(i);
        expected += std::to_string(i) + ";";
    }
    CHECK(tree.measure() == expected);
    CHECK(audit(tree));
}

TEST_CASE("FingerTreeStructure - ReadingCopiesNoPayload", "[fingertree::structure]") {
    using Tree  = ft::FingerTree<Counted>;
    using Level = ft::FingerTreeLevel<Tree>;
    Tree tree;
    for (int i = 0; i < 60; ++i)
        tree = tree.snoc(Counted(i));

    Counted::copies = 0;
    int        sum  = 0;
    const auto walk = [&](const auto& self, const Level& level) -> void {
        const auto node = [&](const auto& again, const typename Level::node_type& n) -> void {
            if (n.form() == ft::FingerTreeNodeForm::LEAF) {
                sum += n.value().value;
                return;
            }
            for (const auto& child : n.children())
                again(again, child);
        };
        if (level.form() == ft::FingerTreeForm::SINGLE)
            node(node, level.element());
        if (level.form() != ft::FingerTreeForm::DEEP)
            return;
        for (const auto& n : level.left())
            node(node, n);
        self(self, level.spine());
        for (const auto& n : level.right())
            node(node, n);
    };
    walk(walk, Level(tree));
    CHECK(sum == 59 * 60 / 2);
    CHECK(Counted::copies == 0);
}

TEST_CASE("FingerTreeStructure - PartsOutliveTheTreeTheyWereReadFrom", "[fingertree::structure]") {
    using Tree  = ft::FingerTree<std::string>;
    using Level = ft::FingerTreeLevel<Tree>;

    std::shared_ptr<const std::string> payload;
    std::optional<Level>               spine;
    {
        Tree tree;
        for (int i = 0; i < 30; ++i)
            tree = tree.snoc("payload number " + std::to_string(i));
        const Level root(tree);
        payload = root.left().front().share_value();
        spine.emplace(root.spine());
    }
    // The tree is gone. What was read from it still owns its share.
    CHECK(*payload == "payload number 0");
    REQUIRE(spine->form() == ft::FingerTreeForm::DEEP);
    CHECK(spine->left().front().children().front().value() == "payload number 1");
    CHECK_FALSE(spine->is_sequence());
}

TEST_CASE("FingerTreeStructure - BuilderRebuildsAnEquivalentTree", "[fingertree::structure]") {
    using Tree  = ft::FingerTree<int>;
    using Level = ft::FingerTreeLevel<Tree>;
    for (int n : {0, 1, 2, 5, 6, 9, 23, 64, 301}) {
        CAPTURE(n);
        const auto tree    = counting<Tree>(n);
        const auto rebuilt = rebuild(ft::FingerTreeStructureBuilder<Tree>(tree), Level(tree));
        CHECK(show(rebuilt) == show(Level(tree)));
        REQUIRE(rebuilt.is_sequence());
        const auto sequence = rebuilt.sequence();
        CHECK(sequence == tree);
        CHECK(sequence.measure() == tree.measure());
        CHECK(audit(sequence));
        // The rebuilt tree is an ordinary tree: the sequence interface works.
        CHECK(sequence.snoc(n).size() == static_cast<std::size_t>(n) + 1);
    }
}

TEST_CASE("FingerTreeStructure - BuilderComputesEveryMeasure", "[fingertree::structure]") {
    using Builder = ft::FingerTreeStructureBuilder<Spelled>;
    using Level   = ft::FingerTreeLevel<Spelled>;
    const Builder builder;

    // [1 | (<2 3> <4 5 6>) | 7 8]: built by hand, in a shape no sequence
    // operation was asked for.
    Level::digit_type left;
    left.push_back(builder.leaf(1));
    Level::digit_type inner_left;
    inner_left.push_back(builder.node2(builder.leaf(2), builder.leaf(3)));
    Level::digit_type inner_right;
    inner_right.push_back(builder.node3(builder.leaf(4), builder.leaf(5), builder.leaf(6)));
    Level::digit_type right;
    right.push_back(builder.leaf(7));
    right.push_back(builder.leaf(8));

    auto spine = builder.deep(std::move(inner_left), builder.empty(), std::move(inner_right));
    CHECK(spine.level() == 1);
    CHECK_FALSE(spine.is_sequence());
    CHECK(spine.measure() == "2;3;4;5;6;");

    const auto level = builder.deep(std::move(left), std::move(spine), std::move(right));
    CHECK(show(level) == "[1 | [<2 3> | () | <4 5 6>] | 7 8]");
    REQUIRE(level.is_sequence());
    const auto tree = level.sequence();
    CHECK(tree.measure() == "1;2;3;4;5;6;7;8;");
    CHECK(tree.flatten() == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8});
    CHECK(audit(tree));

    // The single form and the empty form.
    CHECK(builder.single(builder.leaf(9)).sequence().flatten() == std::vector<int>{9});
    CHECK(builder.empty().sequence().empty());
    CHECK(builder.empty().is_sequence());
}

TEST_CASE("FingerTreeStructure - BuilderKeepsMonoidState", "[fingertree::structure]") {
    using Level = ft::FingerTreeLevel<Weighted>;
    Weighted tree({}, ModSum{7});
    for (int i = 1; i <= 25; ++i)
        tree = tree.snoc(i);
    REQUIRE(tree.measure() == (25u * 26u / 2u) % 7u);

    const ft::FingerTreeStructureBuilder<Weighted> builder(tree);
    CHECK(builder.get_monoid().modulus == 7);
    const auto rebuilt = rebuild(builder, Level(tree)).sequence();
    CHECK(rebuilt.get_monoid().modulus == 7);
    CHECK(rebuilt.measure() == tree.measure());
    CHECK(audit(rebuilt));

    // A builder with other state measures the same payloads differently:
    // the configuration is the builder's, not the source's.
    const auto other = rebuild(ft::FingerTreeStructureBuilder<Weighted>({}, ModSum{11}), Level(tree)).sequence();
    CHECK(other.get_monoid().modulus == 11);
    CHECK(other.measure() == (25u * 26u / 2u) % 11u);
    CHECK(other == tree);
}

TEST_CASE("FingerTreeStructure - BuilderAllocatesOnlyFromItsResource", "[fingertree::structure]") {
    using Tree  = ft::pmr::FingerTree<int>;
    using Level = ft::FingerTreeLevel<Tree>;

    CountingResource source_resource(std::pmr::new_delete_resource());
    CountingResource target_resource(std::pmr::new_delete_resource());
    Tree             tree(&source_resource);
    for (int i = 0; i < 70; ++i)
        tree = tree.snoc(i);

    const auto source_allocations = source_resource.allocations;
    Tree       rebuilt(&target_resource);
    {
        const ForbidDefaultResource                forbid;
        const ft::FingerTreeStructureBuilder<Tree> builder{Tree::allocator_type(&target_resource)};
        rebuilt = rebuild(builder, Level(tree)).sequence();
    }
    CHECK(rebuilt.get_allocator().resource() == &target_resource);
    CHECK(target_resource.allocations > 0);
    CHECK(source_resource.allocations == source_allocations);
    CHECK(rebuilt == tree);
    CHECK(audit(rebuilt));
}

TEST_CASE("FingerTreeStructure - MoveOnlyPayloads", "[fingertree::structure]") {
    using Tree    = ft::FingerTree<std::unique_ptr<int>>;
    using Builder = ft::FingerTreeStructureBuilder<Tree>;
    using Level   = ft::FingerTreeLevel<Tree>;
    const Builder builder;

    Level::digit_type left;
    left.push_back(builder.leaf(std::make_unique<int>(1)));
    Level::digit_type right;
    right.push_back(builder.leaf(std::make_unique<int>(2)));
    const auto tree = builder.deep(std::move(left), builder.empty(), std::move(right)).sequence();
    CHECK(tree.size() == 2);

    const Level level(tree);
    CHECK(*level.left().front().value() == 1);
    CHECK(*level.right().front().value() == 2);
}
