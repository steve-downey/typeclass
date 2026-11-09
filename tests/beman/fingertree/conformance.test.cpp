// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree/tree/finger_tree_foldable.hpp>
#include <beman/fingertree/tree/finger_tree_interval_index.hpp>
#include <beman/fingertree/tree/finger_tree_priority_queue.hpp>
#include <beman/fingertree/tree/finger_tree_random_access.hpp>
#include <beman/fingertree/tree/finger_tree_rope.hpp>
#include <beman/fingertree/tree/finger_tree_traversable.hpp>
#include <beman/transpose/string.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace {
namespace bt = beman::transpose;
namespace ft = beman::fingertree::tree;

template <class T>
using Foldable = std::remove_cvref_t<decltype(bt::foldable<T>)>;
template <class T>
using Traversable = std::remove_cvref_t<decltype(bt::traversable<T>)>;
template <class TREE>
struct ExpectedElement;
template <class T, class M, class P, class A>
struct ExpectedElement<ft::FingerTree<T, M, P, A>> : std::type_identity<T> {};
template <class T, class TREE>
struct ExpectedElement<ft::FingerTreeRandomAccess<T, TREE>> : std::type_identity<T> {};
template <class T, class TREE>
struct ExpectedElement<ft::FingerTreePriorityQueue<T, TREE>> : std::type_identity<T> {};
template <class T, class TREE>
struct ExpectedElement<ft::FingerTreeIntervalIndex<T, TREE>> : std::type_identity<T> {};
template <class TREE>
struct ExpectedElement<ft::FingerTreeRope<TREE>> : std::type_identity<std::string> {};

struct NotCallable {};
struct ToString {
    template <class T>
    auto operator()(const T& value) & -> std::string {
        if constexpr (std::same_as<T, std::string>)
            return value;
        else
            return std::to_string(value);
    }
};
struct RvalueOnly {
    template <class T>
    auto operator()(const T&) && -> std::string {
        return {};
    }
};
struct VoidResult {
    template <class T>
    auto operator()(const T&) const -> void {}
};
struct ArrayResult {
    template <class T>
    auto operator()(const T&) const -> int (&)[2];
};
struct FunctionResult {
    template <class T>
    auto operator()(const T&) const -> void (&)();
};
struct WrongIdentity {
    auto identity() const -> int { return 0; }
    auto combine(const std::string&, const std::string&) const -> std::string { return {}; }
};
struct WrongCombine {
    auto identity() const -> std::string { return {}; }
    auto combine(const std::string&, const std::string&) const -> int { return 0; }
};

template <class TREE, class F, class M>
concept object_foldable =
    requires(const TREE& tree, F&& f, M m) { bt::foldable<TREE>.fold_map(std::forward<F>(f), tree, m); };
template <class TREE, class F, class M>
concept operation_foldable = requires(const TREE& tree, F&& f, M m) { bt::fold_map(std::forward<F>(f), tree, m); };
template <class TREE>
concept operation_traversable =
    requires(const TREE& tree) { bt::traverse([](const auto & value) { return std::optional{value}; }, tree); };
template <class TREE>
concept operation_collectable = requires(const TREE& tree) { bt::to_vector(tree); };
template <class TREE>
concept operation_searchable = requires(const TREE& tree) { bt::find_first(tree, [](const auto&) { return true; }); };
} // namespace

TEMPLATE_TEST_CASE("Fingertree Foldable conforms over payloads and rejects invalid calls",
                   "",
                   ft::FingerTree<int>,
                   ft::FingerTree<std::string>,
                   ft::FingerTreeRandomAccess<int>,
                   ft::FingerTreeRandomAccess<std::string>,
                   ft::FingerTreePriorityQueue<int>,
                   ft::FingerTreePriorityQueue<std::string>,
                   ft::FingerTreeIntervalIndex<int>,
                   ft::FingerTreeIntervalIndex<std::string>,
                   ft::FingerTreeRope<>) {
    STATIC_REQUIRE(std::same_as<bt::applicative_value_t<TestType>, typename ExpectedElement<TestType>::type>);
    STATIC_REQUIRE(bt::foldable_object<Foldable<TestType>, TestType>);
    using Monoid = std::remove_cvref_t<decltype(bt::monoid<std::string>)>;
    STATIC_REQUIRE(object_foldable<TestType, ToString, Monoid>);
    STATIC_REQUIRE(operation_foldable<TestType, ToString, Monoid>);
    STATIC_REQUIRE(!object_foldable<TestType, RvalueOnly, Monoid>);
    STATIC_REQUIRE(!operation_foldable<TestType, RvalueOnly, Monoid>);
    STATIC_REQUIRE(!object_foldable<TestType, NotCallable, Monoid>);
    STATIC_REQUIRE(!operation_foldable<TestType, NotCallable, Monoid>);
    STATIC_REQUIRE(!object_foldable<TestType, VoidResult, Monoid>);
    STATIC_REQUIRE(!operation_foldable<TestType, VoidResult, Monoid>);
    STATIC_REQUIRE(!object_foldable<TestType, ArrayResult, Monoid>);
    STATIC_REQUIRE(!operation_foldable<TestType, ArrayResult, Monoid>);
    STATIC_REQUIRE(!object_foldable<TestType, FunctionResult, Monoid>);
    STATIC_REQUIRE(!operation_foldable<TestType, FunctionResult, Monoid>);
    STATIC_REQUIRE(!object_foldable<TestType, ToString, NotCallable>);
    STATIC_REQUIRE(!operation_foldable<TestType, ToString, NotCallable>);
    STATIC_REQUIRE(!object_foldable<TestType, ToString, WrongIdentity>);
    STATIC_REQUIRE(!operation_foldable<TestType, ToString, WrongIdentity>);
    STATIC_REQUIRE(!object_foldable<TestType, ToString, WrongCombine>);
    STATIC_REQUIRE(!operation_foldable<TestType, ToString, WrongCombine>);
}

TEMPLATE_TEST_CASE("Fingertree generic Traversable conforms over its payloads",
                   "",
                   ft::FingerTree<int>,
                   ft::FingerTree<std::string>,
                   ft::FingerTreeRandomAccess<int>,
                   ft::FingerTreeRandomAccess<std::string>,
                   ft::FingerTreePriorityQueue<int>,
                   ft::FingerTreePriorityQueue<std::string>,
                   ft::FingerTreeIntervalIndex<int>,
                   ft::FingerTreeIntervalIndex<std::string>) {
    STATIC_REQUIRE(std::same_as<bt::applicative_value_t<TestType>, typename ExpectedElement<TestType>::type>);
    STATIC_REQUIRE(bt::traversable_object<Traversable<TestType>, TestType>);
}

TEST_CASE("Rope advertises only its supported chunk Foldable") {
    STATIC_REQUIRE(!bt::has_instance_v<Traversable<ft::FingerTreeRope<>>>);
    STATIC_REQUIRE(!operation_traversable<ft::FingerTreeRope<>>);
    const auto rope = ft::FingerTreeRope<>::from_chunks({"ab", "c", "de"});
    CHECK(bt::combine_all(rope) == "abcde");
    CHECK(bt::foldable<ft::FingerTreeRope<>>.fold(rope) == "abcde");
    CHECK(bt::fold_left(rope, std::string{}, [](auto acc, const auto& value) { return acc + "[" + value + "]"; }) ==
          "[ab][c][de]");
    CHECK(bt::fold_right(rope, std::string{}, [](const auto& value, auto acc) { return "[" + value + "]" + acc; }) ==
          "[ab][c][de]");
}

TEST_CASE("A persistent fold reads move-only elements without copying them") {
    using Tree = ft::FingerTree<std::unique_ptr<int>>;
    STATIC_REQUIRE(!bt::foldable_object<Foldable<Tree>, Tree>);
    STATIC_REQUIRE(!operation_collectable<Tree>);
    STATIC_REQUIRE(!operation_searchable<Tree>);
    const auto tree = Tree{}.snoc(std::make_unique<int>(1)).snoc(std::make_unique<int>(2));
    const auto read = [](const std::unique_ptr<int>& value) { return std::to_string(*value); };
    STATIC_REQUIRE(object_foldable<Tree, decltype(read), std::remove_cvref_t<decltype(bt::monoid<std::string>)>>);
    CHECK(bt::fold_map(read, tree, bt::monoid<std::string>) == "12");
    CHECK(bt::length(tree) == 2);
}
