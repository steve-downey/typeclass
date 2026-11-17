// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/typeclass_tree_adapters/finger_tree.hpp>
#include <beman/typeclass_tree_adapters/finger_tree.hpp> // Re-inclusion check

#include <beman/fingertree.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/string.hpp>
#include <beman/tree_algorithms.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <numeric>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
namespace tt = beman::typeclass_tree_adapters;

// The layer's fmap as the explicit verbs take it.
inline constexpr auto layer_map = [](auto&& fn, const auto& layer) {
    return bt::functor<std::remove_cvref_t<decltype(layer)>>.fmap(std::forward<decltype(fn)>(fn), layer);
};

inline auto spell(int value) -> std::string { return std::to_string(value); }
inline auto spell(const std::string& value) -> std::string { return value; }

// Order-and-shape-pinning algebra over one finger tree layer. Digits are
// bracketed, the spine sits between bars ('-' where a Deep stores none),
// nodes are angle-bracketed. Not idempotent, not commutative: element
// order, grouping and spine position are all visible in the result.
template <class P>
struct Shape {
    auto operator()(const tt::FingerTreeF<P, std::string>& layer) const -> std::string {
        return std::visit(
            bt::overloaded{
                [](const tt::FingerEmpty&) -> std::string { return "()"; },
                [](const tt::FingerSingle<std::string>& s) -> std::string { return "(" + *s.element + ")"; },
                [](const tt::FingerDeep<std::string>& d) -> std::string {
                    std::string out = "[";
                    for (const auto& position : d.left)
                        out += *position + " ";
                    out += "| " + (d.spine ? *d.spine : std::string("-")) + " |";
                    for (const auto& position : d.right)
                        out += " " + *position;
                    return out + "]";
                },
                [](const tt::FingerLeaf<P>& l) -> std::string { return spell(tt::finger_payload(l.value)); },
                [](const tt::FingerNode2<std::string>& n) -> std::string { return "<" + *n.a + " " + *n.b + ">"; },
                [](const tt::FingerNode3<std::string>& n) -> std::string {
                    return "<" + *n.a + " " + *n.b + " " + *n.c + ">";
                },
            },
            layer);
    }
};

// The shape of a tree, folded in its own representation.
template <class TREE>
auto shape(const TREE& tree) -> std::string {
    using Shared = tt::FingerPayloadRef<typename TREE::value_type>;
    return ta::fold_with<std::string>(Shape<Shared>{}, layer_map, tt::finger_tree_project, &tree);
}

// Rebuild a tree layer by layer: project it, embed it again.
template <class TREE>
auto round_trip(const TREE& tree, const tt::FingerTreeEmbed<TREE>& embed) -> TREE {
    using Part = tt::FingerTreePart<TREE>;
    return ta::unfold_with<Part>(tt::finger_tree_project, layer_map, embed, Part(tree)).sequence();
}

template <class TREE>
auto by_snoc(int n, TREE tree = TREE{}) -> TREE {
    for (int i = 0; i < n; ++i)
        tree = tree.snoc(i);
    return tree;
}

template <class TREE>
auto by_cons(int n) -> TREE {
    TREE tree;
    for (int i = n; i-- > 0;)
        tree = tree.cons(i);
    return tree;
}

auto iota(int n) -> std::vector<int> {
    std::vector<int> values(static_cast<std::size_t>(n));
    std::iota(values.begin(), values.end(), 0);
    return values;
}

// Every tree of n elements this file knows how to make. They hold the same
// sequence in different shapes.
auto shapes_of(int n) -> std::vector<ft::FingerTree<int>> {
    using Tree = ft::FingerTree<int>;
    std::vector<Tree> out{by_snoc<Tree>(n), by_cons<Tree>(n), Tree::from_sequence(iota(n))};
    // Two halves appended: the join regroups the digits that meet.
    const auto half = n / 2;
    Tree       head;
    Tree       tail;
    for (int i = 0; i < half; ++i)
        head = head.snoc(i);
    for (int i = half; i < n; ++i)
        tail = tail.snoc(i);
    out.push_back(head.append(tail));
    return out;
}

const int k_sizes[] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 17, 33, 100, 500};

// A noncommutative measure: the text of the elements, in order.
struct Text {
    using value_type = std::string;
    auto identity() const -> std::string { return {}; }
    auto combine(const std::string& a, const std::string& b) const -> std::string { return a + b; }
};
struct Spell {
    auto operator()(const int& value) const -> std::string { return std::to_string(value) + ";"; }
};
using Spelled = ft::FingerTree<int, Text, Spell>;

// A monoid with state.
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

// A monoid and a mapper that both carry state.
struct Join {
    using value_type      = std::string;
    std::string separator = "+";
    auto        identity() const -> std::string { return {}; }
    auto        combine(const std::string& a, const std::string& b) const -> std::string {
        if (a.empty())
            return b;
        if (b.empty())
            return a;
        return a + separator + b;
    }
};
struct Numbered {
    int  calls = 0;
    auto operator()(const int& value) -> std::string { return std::to_string(++calls) + ":" + std::to_string(value); }
};

struct Counted {
    inline static int copies = 0;
    int               value;
    explicit Counted(int v) : value(v) {}
    Counted(const Counted& other) : value(other.value) { ++copies; }
    Counted(Counted&&)                         = default;
    auto operator=(const Counted&) -> Counted& = default;
    auto operator=(Counted&&) -> Counted&      = default;
};

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

class ForbidDefaultResource {
    std::pmr::memory_resource* d_previous;

  public:
    ForbidDefaultResource() : d_previous(std::pmr::set_default_resource(std::pmr::null_memory_resource())) {}
    ~ForbidDefaultResource() { std::pmr::set_default_resource(d_previous); }
    ForbidDefaultResource(const ForbidDefaultResource&)                    = delete;
    auto operator=(const ForbidDefaultResource&) -> ForbidDefaultResource& = delete;
};

// A seed for building a tree that was never a sequence first. A level seed
// stands for `count` nodes of `height`, the first covering the leaf `first`;
// a node seed stands for one node of `height`. The layout is fixed and
// uses Node3 only, so the leaves under a node of height h number 3^h.
struct Seed {
    bool is_node;
    int  first;
    int  count;
    int  height;
};

constexpr auto leaves_under(int height) -> int {
    int n = 1;
    for (int i = 0; i < height; ++i)
        n *= 3;
    return n;
}

// The coalgebra: one layer from one seed. Payloads are made here, fresh,
// and the layer owns them.
template <class T, class MAKE>
struct Grow {
    MAKE make;

    using Layer = tt::FingerTreeF<T, Seed>;

    auto node_at(const Seed& level, int index) const -> ta::child_slot_t<Seed> {
        return ta::make_slot<Seed>(Seed{true, level.first + index * leaves_under(level.height), 1, level.height});
    }

    auto operator()(const Seed& seed) const -> Layer {
        if (seed.is_node) {
            if (seed.height == 0)
                return tt::FingerLeaf<T>{make(seed.first)};
            const auto step  = leaves_under(seed.height - 1);
            const auto child = [&](int index) {
                return ta::make_slot<Seed>(Seed{true, seed.first + index * step, 1, seed.height - 1});
            };
            return tt::FingerNode3<Seed>{child(0), child(1), child(2)};
        }
        if (seed.count == 0)
            return tt::FingerEmpty{};
        if (seed.count == 1)
            return tt::FingerSingle<Seed>{node_at(seed, 0)};

        tt::FingerDeep<Seed> deep;
        if (seed.count <= 8) {
            const auto split = seed.count / 2;
            for (int i = 0; i < split; ++i)
                deep.left.push_back(node_at(seed, i));
            for (int i = split; i < seed.count; ++i)
                deep.right.push_back(node_at(seed, i));
            return deep;
        }
        // Two nodes on each side; the rest, in threes, one level down.
        deep.left.push_back(node_at(seed, 0));
        deep.left.push_back(node_at(seed, 1));
        deep.spine = ta::make_slot<Seed>(
            Seed{false, seed.first + 2 * leaves_under(seed.height), (seed.count - 4) / 3, seed.height + 1});
        deep.right.push_back(node_at(seed, seed.count - 2));
        deep.right.push_back(node_at(seed, seed.count - 1));
        return deep;
    }
};

// 34 = 2 + 3 * 10 + 2, and 10 = 2 + 3 * 2 + 2: three spine levels.
constexpr Seed k_thirty_four{false, 0, 34, 0};
const char*    k_thirty_four_shape = "[0 1 | [<2 3 4> <5 6 7> | [<<8 9 10> <11 12 13> <14 15 16>> | - | "
                                     "<<17 18 19> <20 21 22> <23 24 25>>] | <26 27 28> <29 30 31>] | 32 33]";

} // namespace

TEST_CASE("FingerTreeAdapter - DirectFoldPinsShapeAndOrder", "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::FingerTree<int>;
    CHECK(shape(Tree{}) == "()");
    CHECK(shape(by_snoc<Tree>(1)) == "(0)");
    CHECK(shape(by_snoc<Tree>(2)) == "[0 | - | 1]");
    CHECK(shape(by_snoc<Tree>(5)) == "[0 | - | 1 2 3 4]");
    CHECK(shape(by_snoc<Tree>(6)) == "[0 | (<1 2 3>) | 4 5]");
    CHECK(shape(by_snoc<Tree>(9)) == "[0 | [<1 2 3> | - | <4 5 6>] | 7 8]");
    CHECK(shape(by_cons<Tree>(6)) == "[0 1 | (<2 3 4>) | 5]");
    // The same sequence in another shape: structural folds see the shape.
    CHECK(shape(Tree::from_sequence(iota(8))) == "[0 1 2 3 | - | 4 5 6 7]");
    CHECK(shape(by_snoc<Tree>(8)) == "[0 | (<1 2 3>) | 4 5 6 7]");
    // Node2 appears where an append regroups the inner digits.
    const auto five = by_snoc<Tree>(5);
    CHECK(shape(five.append(five)) == "[0 | [<1 2 3> | - | <4 0>] | 1 2 3 4]");
}

TEST_CASE("FingerTreeAdapter - DirectFixAndFusedFoldsAgree", "[typeclass_tree_adapters::finger_tree]") {
    using Tree   = ft::FingerTree<int>;
    using Part   = tt::FingerTreePart<Tree>;
    using Shared = tt::FingerPayloadRef<int>;
    for (int n : k_sizes) {
        for (const auto& tree : shapes_of(n)) {
            CAPTURE(n);
            const auto direct = shape(tree);
            CAPTURE(direct);

            // A fold started from a part is the fold started from the tree.
            CHECK(ta::fold_with<std::string>(Shape<Shared>{}, layer_map, tt::finger_tree_project, Part(tree)) ==
                  direct);

            // The Fix form holds the same layers.
            const auto fixed = tt::to_fix(tree);
            CHECK(ta::fold_fix<std::string>(Shape<int>{}, layer_map, fixed) == direct);
            CHECK(ta::fold_fix<std::string>(Shape<int>{}, fixed) == direct);

            // Fused: no Fix is materialized, explicit fmap and looked up.
            CHECK(ta::refold<std::string, tt::FingerTreeLayer<int>::F>(
                      Shape<int>{}, tt::finger_tree_project_owned, layer_map, Part(tree)) == direct);
            CHECK(ta::refold<std::string, tt::FingerTreeLayer<int>::F>(
                      Shape<int>{}, tt::finger_tree_project_owned, Part(tree)) == direct);
        }
    }
}

TEST_CASE("FingerTreeAdapter - RoundTripsRebuildAnEquivalentStructure", "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::FingerTree<int>;
    for (int n : k_sizes) {
        for (const auto& tree : shapes_of(n)) {
            CAPTURE(n);
            const tt::FingerTreeEmbed<Tree> embed(tree);

            const auto direct = round_trip(tree, embed);
            CHECK(direct == tree);
            CHECK(shape(direct) == shape(tree));
            CHECK(direct.measure() == tree.measure());

            const auto through_fix = tt::from_fix(tt::to_fix(tree), embed);
            CHECK(through_fix == tree);
            CHECK(shape(through_fix) == shape(tree));
            CHECK(through_fix.measure() == tree.measure());

            // What comes back is an ordinary tree.
            CHECK(direct.snoc(n).cons(-1).size() == static_cast<std::size_t>(n) + 2);
        }
    }
}

TEST_CASE("FingerTreeAdapter - RoundTripRecomputesANoncommutativeMeasure", "[typeclass_tree_adapters::finger_tree]") {
    const auto  tree = by_snoc<Spelled>(41);
    std::string expected;
    for (int i = 0; i < 41; ++i)
        expected += std::to_string(i) + ";";
    REQUIRE(tree.measure() == expected);

    const auto rebuilt = round_trip(tree, tt::FingerTreeEmbed<Spelled>(tree));
    CHECK(rebuilt.measure() == expected);
    CHECK(rebuilt == tree);
    CHECK(shape(rebuilt) == shape(tree));
    CHECK(tt::from_fix(tt::to_fix(tree), tt::FingerTreeEmbed<Spelled>(tree)).measure() == expected);

    // Every part carries the measure of what it holds: the parts of the
    // root layer, read left to right, spell the whole.
    using Part        = tt::FingerTreePart<Spelled>;
    const auto  layer = tt::finger_tree_project(Part(tree));
    const auto* deep  = std::get_if<tt::FingerDeep<Part>>(&layer);
    REQUIRE(deep != nullptr);
    REQUIRE(deep->spine);
    std::string spelled;
    for (const auto& position : deep->left)
        spelled += position->measure();
    spelled += deep->spine->measure();
    for (const auto& position : deep->right)
        spelled += position->measure();
    CHECK(spelled == expected);
    CHECK(Part(tree).measure() == expected);
}

TEST_CASE("FingerTreeAdapter - RoundTripKeepsMonoidState", "[typeclass_tree_adapters::finger_tree]") {
    const auto tree = by_snoc<Weighted>(30, Weighted({}, ModSum{7}));
    REQUIRE(tree.get_monoid().modulus == 7);
    REQUIRE(tree.measure() == (29u * 30u / 2u) % 7u);

    const auto rebuilt = round_trip(tree, tt::FingerTreeEmbed<Weighted>(tree));
    CHECK(rebuilt.get_monoid().modulus == 7);
    CHECK(rebuilt.measure() == tree.measure());
    CHECK(shape(rebuilt) == shape(tree));

    // The embedding owns the configuration: another modulus, another law.
    using Builder    = tt::FingerTreeEmbed<Weighted>::builder_type;
    const auto other = round_trip(tree, tt::FingerTreeEmbed<Weighted>(Builder({}, ModSum{11})));
    CHECK(other.get_monoid().modulus == 11);
    CHECK(other.measure() == (29u * 30u / 2u) % 11u);
    CHECK(other == tree);
}

TEST_CASE("FingerTreeAdapter - RoundTripAllocatesOnlyFromTheEmbeddingsResource",
          "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::pmr::FingerTree<int>;
    CountingResource source_resource(std::pmr::new_delete_resource());
    CountingResource target_resource(std::pmr::new_delete_resource());
    const auto       tree = by_snoc<Tree>(90, Tree(&source_resource));

    const auto before = source_resource.allocations;
    Tree       rebuilt(&target_resource);
    {
        const ForbidDefaultResource     forbid;
        const tt::FingerTreeEmbed<Tree> embed{
            tt::FingerTreeEmbed<Tree>::builder_type(Tree::allocator_type(&target_resource))};
        rebuilt = round_trip(tree, embed);
    }
    CHECK(rebuilt.get_allocator().resource() == &target_resource);
    CHECK(target_resource.allocations > 0);
    CHECK(source_resource.allocations == before);
    CHECK(rebuilt == tree);
    CHECK(shape(rebuilt) == shape(tree));
}

TEST_CASE("FingerTreeAdapter - ElementwiseFoldsAgreeWithTheNativeFoldable", "[typeclass_tree_adapters::finger_tree]") {
    // String concatenation: not commutative, so order is observable.
    const auto text   = [](const int& value) { return std::to_string(value) + ","; };
    const auto concat = [](const std::string& a, const std::string& b) { return a + b; };

    for (int n : k_sizes) {
        for (const auto& tree : shapes_of(n)) {
            CAPTURE(n);
            std::string expected;
            for (int i = 0; i < n; ++i)
                expected += std::to_string(i) + ",";

            const auto native = bt::fold_map(text, tree, bt::monoid<std::string>);
            CHECK(native == expected);

            // Explicit: every ingredient passed by hand.
            CHECK(ta::fold_map<std::string>(text,
                                            concat,
                                            std::string{},
                                            tt::finger_tree_layer_fold_map,
                                            layer_map,
                                            tt::finger_tree_project,
                                            &tree) == native);
            // Lookup: the projection, the layer fold and fmap are found.
            CHECK(ta::fold_map<std::string>(text, concat, std::string{}, tree) == native);
            // The monoid-object bridge.
            CHECK(ta::fold_map(text, tree, bt::monoid<std::string>) == native);
            // The Fix form, explicit and looked up.
            const auto fixed = tt::to_fix(tree);
            CHECK(ta::fold_map<std::string>(
                      text, concat, std::string{}, tt::finger_tree_layer_fold_map, layer_map, fixed) == native);
            CHECK(ta::fold_map<std::string>(text, concat, std::string{}, fixed) == native);
        }
    }
}

TEST_CASE("FingerTreeAdapter - ElementwiseFoldKeepsCallableAndMonoidState", "[typeclass_tree_adapters::finger_tree]") {
    const auto tree = by_snoc<ft::FingerTree<int>>(4);
    Numbered   numbered;
    CHECK(ta::fold_map(numbered, tree, Join{"|"}) == "1:0|2:1|3:2|4:3");
    CHECK(numbered.calls == 4);
}

TEST_CASE("FingerTreeAdapter - FoldingCopiesNoPayload", "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::FingerTree<Counted>;
    Tree tree;
    for (int i = 0; i < 80; ++i)
        tree = tree.snoc(Counted(i));
    const auto value = [](const Counted& c) { return c.value; };
    const auto plus  = [](int a, int b) { return a + b; };

    Counted::copies = 0;
    CHECK(ta::fold_map<int>(value, plus, 0, tree) == 79 * 80 / 2);
    CHECK(Counted::copies == 0);

    const auto leaves = ta::fold_with<int>(
        [](const tt::FingerTreeF<tt::FingerPayloadRef<Counted>, int>& layer) {
            return tt::finger_tree_layer_fold_map([](const Counted&) { return 1; }, std::plus<int>{}, 0, layer);
        },
        layer_map,
        tt::finger_tree_project,
        &tree);
    CHECK(leaves == 80);
    CHECK(Counted::copies == 0);

    // The projection that owns its payloads copies each exactly once.
    using Part       = tt::FingerTreePart<Tree>;
    const auto layer = tt::finger_tree_project_owned(Part(Part(tree).level().left().front()));
    CHECK(std::holds_alternative<tt::FingerLeaf<Counted>>(layer));
    CHECK(Counted::copies == 1);

    // An embedding consumes a layer it is given: the payload moves.
    Counted::copies = 0;
    const tt::FingerTreeEmbed<Tree> embed;
    const auto                      part = embed(tt::FingerTreeF<Counted, Part>(tt::FingerLeaf<Counted>{Counted(7)}));
    CHECK(part.node().value().value == 7);
    CHECK(Counted::copies == 0);
    // And copies from one it is only shown.
    const tt::FingerTreeF<Counted, Part> kept(tt::FingerLeaf<Counted>{Counted(8)});
    CHECK(embed(kept).node().value().value == 8);
    CHECK(Counted::copies == 1);
}

TEST_CASE("FingerTreeAdapter - LayersOutliveTheTreeTheyWereProjectedFrom", "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::FingerTree<std::string>;
    using Part = tt::FingerTreePart<Tree>;
    std::optional<tt::FingerTreeLayerOf<Tree>> layer;
    {
        Tree tree;
        for (int i = 0; i < 12; ++i)
            tree = tree.snoc("a payload long enough to live on the heap " + std::to_string(i));
        layer.emplace(tt::finger_tree_project(&tree));
    }
    // The tree is gone; the layer still owns its share of every position.
    const auto* deep = std::get_if<tt::FingerDeep<Part>>(&*layer);
    REQUIRE(deep != nullptr);
    REQUIRE(deep->spine);
    const auto  leaf = tt::finger_tree_project(*deep->left.front());
    const auto* held = std::get_if<tt::FingerLeaf<tt::FingerPayloadRef<std::string>>>(&leaf);
    REQUIRE(held != nullptr);
    CHECK(*held->value == "a payload long enough to live on the heap 0");
    CHECK(ta::fold_with<std::string>(
              Shape<tt::FingerPayloadRef<std::string>>{}, layer_map, tt::finger_tree_project, *deep->spine)
              .size() > 0);
}

TEST_CASE("FingerTreeAdapter - UnfoldFromSeedsBuildsAMeasuredTree", "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::FingerTree<int>;
    using Part = tt::FingerTreePart<Tree>;
    const Grow<int, decltype([](int leaf) { return leaf; })> grow{};

    // Direct build into the tree's own representation.
    const auto tree = ta::unfold_with<Part>(grow, layer_map, tt::FingerTreeEmbed<Tree>{}, k_thirty_four).sequence();
    CHECK(tree.flatten() == iota(34));
    CHECK(tree.measure() == 34);
    CHECK(shape(tree) == k_thirty_four_shape);

    // The tree is an ordinary sequence: it grows and splits like any other.
    const auto grown = tree.snoc(34).cons(-1);
    CHECK(grown.size() == 36);
    const auto parts = grown.split_at_measure(std::size_t{10});
    CHECK(parts.d_left.size() + parts.d_right.size() == 36);
    CHECK(parts.d_right.front() == 8);

    // Through the Fix form: unfold_fix builds the layers, from_fix embeds.
    const auto fixed = ta::unfold_fix<tt::FingerTreeLayer<int>::F>(grow, k_thirty_four);
    CHECK(ta::fold_fix<std::string>(Shape<int>{}, fixed) == k_thirty_four_shape);
    CHECK(tt::from_fix(fixed, tt::FingerTreeEmbed<Tree>{}) == tree);

    // Fused: the same shape and the same total, and no tree is built.
    CHECK(ta::refold<std::string, tt::FingerTreeLayer<int>::F>(Shape<int>{}, grow, k_thirty_four) ==
          k_thirty_four_shape);
    const auto total = ta::refold<int, tt::FingerTreeLayer<int>::F>(
        [](const tt::FingerTreeF<int, int>& layer) {
            return tt::finger_tree_layer_fold_map([](int leaf) { return leaf; }, std::plus<int>{}, 0, layer);
        },
        grow,
        layer_map,
        k_thirty_four);
    CHECK(total == 33 * 34 / 2);
}

TEST_CASE("FingerTreeAdapter - UnfoldComputesANoncommutativeMeasureUnderPmr",
          "[typeclass_tree_adapters::finger_tree]") {
    using Tree = ft::pmr::FingerTree<int, Text, Spell>;
    using Part = tt::FingerTreePart<Tree>;
    const Grow<int, decltype([](int leaf) { return leaf; })> grow{};

    CountingResource resource(std::pmr::new_delete_resource());
    Tree             tree(&resource);
    {
        const ForbidDefaultResource     forbid;
        const tt::FingerTreeEmbed<Tree> embed{
            tt::FingerTreeEmbed<Tree>::builder_type(Tree::allocator_type(&resource))};
        tree = ta::unfold_with<Part>(grow, layer_map, embed, k_thirty_four).sequence();
    }
    std::string expected;
    for (int i = 0; i < 34; ++i)
        expected += std::to_string(i) + ";";
    CHECK(tree.measure() == expected);
    CHECK(tree.get_allocator().resource() == &resource);
    CHECK(resource.allocations > 0);
    CHECK(tree.flatten() == iota(34));
}

TEST_CASE("FingerTreeAdapter - MoveOnlyPayloadsFoldInPlace", "[typeclass_tree_adapters::finger_tree]") {
    using Tree   = ft::FingerTree<std::unique_ptr<int>>;
    using Shared = tt::FingerPayloadRef<std::unique_ptr<int>>;
    Tree tree;
    for (int i = 0; i < 20; ++i)
        tree = tree.snoc(std::make_unique<int>(i));

    // Elementwise, through lookup: the payloads are read where they are.
    const auto value = [](const std::unique_ptr<int>& p) { return *p; };
    CHECK(ta::fold_map<int>(value, std::plus<int>{}, 0, tree) == 19 * 20 / 2);

    // Structurally: one leaf per element, and no payload is copied to
    // find that out -- there is no copy to make.
    const auto leaves = ta::fold_with<int>(
        [](const tt::FingerTreeF<Shared, int>& layer) {
            return tt::finger_tree_layer_fold_map(
                [](const std::unique_ptr<int>&) { return 1; }, std::plus<int>{}, 0, layer);
        },
        layer_map,
        tt::finger_tree_project,
        &tree);
    CHECK(leaves == 20);
}

TEST_CASE("FingerTreeAdapter - CapabilityWrappersFoldThroughTheirBackingTree",
          "[typeclass_tree_adapters::finger_tree]") {
    const auto concat = [](const std::string& a, const std::string& b) { return a + b; };

    // The structural payload of a sequence or a queue is its element.
    const auto sequence = ft::FingerTreeRandomAccess<int>::from_sequence(iota(12));
    const auto text     = [](const int& value) { return std::to_string(value) + ","; };
    CHECK(ta::fold_map<std::string>(text, concat, std::string{}, sequence.backing_tree()) ==
          bt::fold_map(text, sequence, bt::monoid<std::string>));

    const auto queue = ft::FingerTreePriorityQueue<int>::from_values({5, 1, 4, 2, 3});
    CHECK(ta::fold_map<std::string>(text, concat, std::string{}, queue.backing_tree()) ==
          bt::fold_map(text, queue, bt::monoid<std::string>));

    // The structural payload of an interval index is the whole interval;
    // its typeclass payload is only the value. The route says which.
    const auto index    = ft::FingerTreeIntervalIndex<std::string>::from_intervals({{1, 4, "a"}, {2, 9, "b"}});
    const auto interval = [](const ft::Interval<std::string>& entry) {
        return std::to_string(entry.d_start) + "-" + std::to_string(entry.d_end) + ":" + entry.d_payload + " ";
    };
    CHECK(ta::fold_map<std::string>(interval, concat, std::string{}, index.backing_tree()) == "1-4:a 2-9:b ");
    CHECK(bt::fold_map([](const std::string& value) { return value; }, index, bt::monoid<std::string>) == "ab");

    // A rope's structural payload is its chunks, in order.
    const auto rope = ft::FingerTreeRope<>::from_text("folds over a rope see its chunks, in order", 5);
    CHECK(ta::fold_map<std::string>(
              [](const std::string& chunk) { return chunk; }, concat, std::string{}, rope.backing_tree()) ==
          rope.to_string());
    CHECK(shape(rope.backing_tree()).starts_with("[folds "));
}
