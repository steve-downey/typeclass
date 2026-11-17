// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/typeclass_tree_adapters/finger_tree.hpp>

#include <beman/fingertree.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/string.hpp>
#include <beman/tree_algorithms.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
namespace tt = beman::typeclass_tree_adapters;

using Tree     = ft::FingerTree<int>;
using Part     = tt::FingerTreePart<Tree>;
using Shared   = tt::FingerPayloadRef<int>;
using MoveOnly = ft::FingerTree<std::unique_ptr<int>>;

struct LvalueOnly {
    auto operator()(const int& value) & -> int { return value * 10 + 1; }
    auto operator()(const int&) && -> int = delete;
};
struct RvalueOnly {
    auto operator()(const int&) & -> int = delete;
    auto operator()(const int&) && -> int { return 1; }
};
struct NotCallable {};
struct VoidResult {
    auto operator()(const int&) const -> void {}
};
struct ArrayResult {
    auto operator()(const int&) const -> int (&)[2];
};

template <class LAYER, class F>
concept object_mappable = requires(const LAYER& layer, F&& f) { bt::functor<LAYER>.fmap(std::forward<F>(f), layer); };
template <class LAYER, class F>
concept operation_mappable = requires(const LAYER& layer, F&& f) { bt::fmap(std::forward<F>(f), layer); };

template <class T>
concept bridge_foldable = requires(const T& tree) {
    ta::fold_map([](const typename T::value_type&) { return 1; }, tree, bt::sum_monoid<int>{});
};
template <class T>
concept converts_to_fix = requires(const T& tree) { tt::to_fix(tree); };
template <class T>
concept projects_owned = requires(const tt::FingerTreePart<T>& part) { tt::finger_tree_project_owned(part); };
template <class T>
concept projects_shared = requires(const tt::FingerTreePart<T>& part) { tt::finger_tree_project(part); };

// ---------------------------------------------------------------------
// The lookups find the adapter's registrations.
// ---------------------------------------------------------------------

static_assert(bt::has_instance_v<decltype(bt::functor<tt::FingerTreeF<int, int>>)>);
static_assert(bt::has_instance_v<decltype(bt::functor<tt::FingerTreeLayerOf<Tree>>)>);
static_assert(bt::has_instance_v<decltype(ta::layer_fold<tt::FingerTreeF<int, int>>)>);
static_assert(bt::has_instance_v<decltype(ta::layer_fold<tt::FingerTreeLayerOf<Tree>>)>);
static_assert(bt::has_instance_v<decltype(ta::project<Tree>)>);
static_assert(bt::has_instance_v<decltype(ta::project<MoveOnly>)>);
static_assert(bt::has_instance_v<decltype(ta::project<ft::pmr::FingerTree<std::string>>)>);

// The structural element of a layer is its recursive position.
static_assert(std::same_as<bt::applicative_value_t<tt::FingerTreeF<long, int>>, int>);
static_assert(std::same_as<bt::applicative_value_t<tt::FingerTreeLayerOf<Tree>>, Part>);

// The layer satisfies the mechanism's deep Functor concept, at both payload
// slots and with a part in the recursive positions.
static_assert(bt::functor_object<tt::FingerTreeFFunctorMap<long, int>, tt::FingerTreeF<long, int>>);
static_assert(bt::functor_object<tt::FingerTreeFFunctorMap<Shared, int>, tt::FingerTreeF<Shared, int>>);
static_assert(bt::functor_object<tt::FingerTreeFFunctorMap<Shared, Part>, tt::FingerTreeLayerOf<Tree>>);

// ---------------------------------------------------------------------
// One position type and one layer type, whatever the depth.
// ---------------------------------------------------------------------

// A projection of the tree and a projection of any part of it are the same
// type, so a fold recurses through one instantiation of everything.
static_assert(std::same_as<decltype(tt::finger_tree_project(std::declval<const Tree*>())),
                           decltype(tt::finger_tree_project(std::declval<const Part&>()))>);
static_assert(
    std::same_as<decltype(tt::finger_tree_project(std::declval<const Part&>())), tt::FingerTreeLayerOf<Tree>>);
static_assert(std::same_as<std::invoke_result_t<tt::FingerTreeEmbed<Tree>, tt::FingerTreeLayerOf<Tree>>, Part>);
static_assert(std::same_as<decltype(std::declval<const Part&>().level().spine()), Part::level_type>);
static_assert(std::copyable<Part>);

// ---------------------------------------------------------------------
// Calls the adapter cannot make are rejected softly.
// ---------------------------------------------------------------------

// A layer Functor invokes its callable as an lvalue.
static_assert(object_mappable<tt::FingerTreeF<long, int>, LvalueOnly>);
static_assert(operation_mappable<tt::FingerTreeF<long, int>, LvalueOnly>);
static_assert(!object_mappable<tt::FingerTreeF<long, int>, RvalueOnly>);
static_assert(!operation_mappable<tt::FingerTreeF<long, int>, RvalueOnly>);
static_assert(!object_mappable<tt::FingerTreeF<long, int>, NotCallable>);
static_assert(!operation_mappable<tt::FingerTreeF<long, int>, NotCallable>);
static_assert(!object_mappable<tt::FingerTreeF<long, int>, VoidResult>);
static_assert(!object_mappable<tt::FingerTreeF<long, int>, ArrayResult>);

// Mapping copies the payload slot across, so an owned move-only payload
// cannot be mapped; a shared one can, whatever it shares.
static_assert(!object_mappable<tt::FingerTreeF<std::unique_ptr<int>, int>, LvalueOnly>);
static_assert(object_mappable<tt::FingerTreeF<tt::FingerPayloadRef<std::unique_ptr<int>>, int>, LvalueOnly>);

// Reading a move-only tree is supported; anything that must copy a payload
// is not offered for it.
static_assert(projects_shared<MoveOnly>);
static_assert(!projects_owned<MoveOnly>);
static_assert(projects_owned<Tree>);
static_assert(!converts_to_fix<MoveOnly>);
static_assert(converts_to_fix<Tree>);
static_assert(bridge_foldable<Tree>);
static_assert(!bridge_foldable<MoveOnly>);

// An embedding builds one tree type: a layer of another payload, or of
// another tree's parts, is not its argument.
static_assert(std::invocable<tt::FingerTreeEmbed<Tree>, tt::FingerTreeF<int, Part>>);
static_assert(std::invocable<tt::FingerTreeEmbed<Tree>, const tt::FingerTreeF<int, Part>&>);
static_assert(std::invocable<tt::FingerTreeEmbed<Tree>, const tt::FingerTreeLayerOf<Tree>&>);
static_assert(std::invocable<tt::FingerTreeEmbed<Tree>, tt::FingerTreeF<short, Part>>);
static_assert(!std::invocable<tt::FingerTreeEmbed<Tree>, tt::FingerTreeF<std::string, Part>>);
static_assert(!std::invocable<tt::FingerTreeEmbed<Tree>, tt::FingerTreeF<int, tt::FingerTreePart<MoveOnly>>>);
static_assert(!std::invocable<tt::FingerTreeEmbed<Tree>, tt::FingerTreeF<tt::FingerPayloadRef<long>, Part>>);

// A move-only payload embeds from a layer that gives it up, and only then.
static_assert(std::invocable<tt::FingerTreeEmbed<MoveOnly>,
                             tt::FingerTreeF<std::unique_ptr<int>, tt::FingerTreePart<MoveOnly>>>);
static_assert(!std::invocable<tt::FingerTreeEmbed<MoveOnly>,
                              const tt::FingerTreeF<std::unique_ptr<int>, tt::FingerTreePart<MoveOnly>>&>);
static_assert(!std::invocable<tt::FingerTreeEmbed<MoveOnly>, const tt::FingerTreeLayerOf<MoveOnly>&>);

// The capability wrappers are not registered as structural trees. Their
// structure is their backing tree's, reached by naming it.
static_assert(!bt::has_instance_v<decltype(ta::project<ft::FingerTreeRandomAccess<int>>)>);
static_assert(!bt::has_instance_v<decltype(ta::project<ft::FingerTreeIntervalIndex<int>>)>);
static_assert(!bt::has_instance_v<decltype(ta::project<ft::FingerTreeRope<>>)>);
static_assert(!bridge_foldable<ft::FingerTreeRandomAccess<int>>);
static_assert(!bridge_foldable<ft::FingerTreeIntervalIndex<int>>);
static_assert(bt::has_instance_v<
              decltype(ta::project<std::remove_cvref_t<
                           decltype(std::declval<const ft::FingerTreeIntervalIndex<int>&>().backing_tree())>>)>);

// The typeclass instances fingertree registers for itself are untouched.
static_assert(bt::has_instance_v<decltype(bt::foldable<Tree>)>);
static_assert(bt::has_instance_v<decltype(bt::functor<Tree>)>);
static_assert(bt::has_instance_v<decltype(bt::traversable<Tree>)>);

} // namespace

TEST_CASE("FingerTreeAdapterConformance - LayerFunctorMapsEveryPositionInOrder",
          "[typeclass_tree_adapters::conformance]") {
    using Layer = tt::FingerTreeF<long, int>;
    tt::FingerDeep<int> deep;
    deep.left.push_back(ta::make_slot<int>(1));
    deep.left.push_back(ta::make_slot<int>(2));
    deep.spine = ta::make_slot<int>(3);
    deep.right.push_back(ta::make_slot<int>(4));

    std::string order;
    const auto  record = [&order](const int& position) {
        order += std::to_string(position);
        return std::to_string(position * 10);
    };
    const auto  mapped = bt::fmap(record, Layer(deep));
    const auto* result = std::get_if<tt::FingerDeep<std::string>>(&mapped);
    REQUIRE(result != nullptr);
    CHECK(order == "1234");
    CHECK(*result->left[0] == "10");
    CHECK(*result->left[1] == "20");
    CHECK(*result->spine == "30");
    CHECK(*result->right[0] == "40");

    // A Deep that stores no spine maps to one that stores none.
    deep.spine.reset();
    order.clear();
    const auto  without = bt::fmap(record, Layer(deep));
    const auto* bare    = std::get_if<tt::FingerDeep<std::string>>(&without);
    REQUIRE(bare != nullptr);
    CHECK(order == "124");
    CHECK_FALSE(bare->spine);

    // A Leaf has no position: its payload crosses untouched.
    const auto leaf = bt::fmap(record, Layer(tt::FingerLeaf<long>{99L}));
    REQUIRE(std::holds_alternative<tt::FingerLeaf<long>>(leaf));
    CHECK(std::get<tt::FingerLeaf<long>>(leaf).value == 99L);

    // replace is the Functor base's derivation over this fmap.
    const auto  replaced = bt::functor<Layer>.replace(Layer(tt::FingerNode3<int>{
                                                          ta::make_slot<int>(1),
                                                          ta::make_slot<int>(2),
                                                          ta::make_slot<int>(3),
                                                      }),
                                                      7);
    const auto* node     = std::get_if<tt::FingerNode3<int>>(&replaced);
    REQUIRE(node != nullptr);
    CHECK(*node->a + *node->b + *node->c == 21);
}

TEST_CASE("FingerTreeAdapterConformance - LayerFoldUsesTheIdentityOnlyForEmpty",
          "[typeclass_tree_adapters::conformance]") {
    using Layer = tt::FingerTreeF<long, std::string>;
    // An identity that is not one: any form that combined with it would
    // show it in the result.
    const std::string poison = "!";
    const auto        fold   = [&](const Layer& layer) {
        return tt::finger_tree_layer_fold_map([](long payload) { return std::to_string(payload); },
                                              [](const std::string& a, const std::string& b) { return a + b; },
                                              poison,
                                              layer);
    };
    const auto slot = [](const char* text) { return ta::make_slot<std::string>(text); };

    CHECK(fold(Layer(tt::FingerEmpty{})) == "!");
    CHECK(fold(Layer(tt::FingerLeaf<long>{42L})) == "42");
    CHECK(fold(Layer(tt::FingerSingle<std::string>{slot("a")})) == "a");
    CHECK(fold(Layer(tt::FingerNode2<std::string>{slot("a"), slot("b")})) == "ab");
    CHECK(fold(Layer(tt::FingerNode3<std::string>{slot("a"), slot("b"), slot("c")})) == "abc");

    tt::FingerDeep<std::string> deep;
    deep.left.push_back(slot("a"));
    deep.left.push_back(slot("b"));
    deep.right.push_back(slot("y"));
    deep.right.push_back(slot("z"));
    CHECK(fold(Layer(deep)) == "abyz");
    deep.spine = slot("-m-");
    CHECK(fold(Layer(deep)) == "ab-m-yz");
}

TEST_CASE("FingerTreeAdapterConformance - PartsReportTheirSort", "[typeclass_tree_adapters::conformance]") {
    Tree tree;
    for (int i = 0; i < 7; ++i)
        tree = tree.snoc(i);

    const Part root(tree);
    CHECK(root.is_level());
    CHECK_FALSE(root.is_node());
    CHECK(root.measure() == 7);
    CHECK(root.sequence() == tree);

    const Part leaf(root.level().left().front());
    CHECK(leaf.is_node());
    CHECK(leaf.measure() == 1);
    CHECK(leaf.node().value() == 0);

    const Part spine(root.level().spine());
    CHECK(spine.is_level());
    CHECK(spine.measure() == 3);
    CHECK_FALSE(spine.level().is_sequence());
}
