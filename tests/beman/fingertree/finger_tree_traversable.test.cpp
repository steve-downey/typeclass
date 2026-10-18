// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree/tree/finger_tree_traversable.hpp>
#include <beman/fingertree/tree/finger_tree_traversable.hpp> // Re-inclusion check

#include <beman/fingertree/tree/finger_tree_interval_index.hpp>
#include <beman/fingertree/tree/finger_tree_priority_queue.hpp>
#include <beman/fingertree/tree/finger_tree_random_access.hpp>
#include <beman/fingertree/tree/finger_tree_rope.hpp>
#include <beman/transpose/traverse.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

TEST_CASE("FingerTreeTraversableTest - TraverseOptionalSucceeds") {
    using Tree  = beman::fingertree::tree::FingerTree<int>;
    auto t      = Tree::from_sequence({1, 2, 3});
    auto result = beman::transpose::traverse([](int x) { return std::optional{x * 10}; }, t);
    REQUIRE(result.has_value());
    CHECK(result->flatten() == (std::vector<int>{10, 20, 30}));
}

TEST_CASE("FingerTreeTraversableTest - TraverseOptionalFailsOnNullopt") {
    using Tree  = beman::fingertree::tree::FingerTree<int>;
    auto t      = Tree::from_sequence({1, 2, 3});
    auto result = beman::transpose::traverse(
        [](int x) -> std::optional<int> { return x == 2 ? std::nullopt : std::optional{x}; }, t);
    CHECK_FALSE(result.has_value());
}

TEST_CASE("FingerTreeTraversableTest - TraversePreservesShape") {
    using Tree  = beman::fingertree::tree::FingerTree<int>;
    auto t      = Tree::from_sequence({10, 20, 30, 40});
    auto result = beman::transpose::traverse([](int x) { return std::optional{x + 1}; }, t);
    REQUIRE(result.has_value());
    CHECK(result->flatten() == (std::vector<int>{11, 21, 31, 41}));
}

TEST_CASE("FingerTreeTraversableTest - TraverseOverLargeTree") {
    // Cover the spine-Node3 path: tree large enough that the spine has
    // structure when traverse materializes it.
    using Tree = beman::fingertree::tree::FingerTree<int>;
    auto t     = Tree{};
    for (int i = 0; i < 100; ++i)
        t = t.snoc(i);

    auto result = beman::transpose::traverse([](int x) { return std::optional{x}; }, t);
    REQUIRE(result.has_value());
    auto v = result->flatten();
    REQUIRE(v.size() == 100U);
    for (int i = 0; i < 100; ++i)
        CHECK(v[static_cast<std::size_t>(i)] == i);
}

// steve-downey/typeclass#8: each finger-tree Traversable instance's traverse
// is constrained, so a probe with a callable it cannot apply is a constraint
// that does not match, not a diagnostic from inside the deduced-return body.
namespace {

struct NotCallable {};

template <class STRUCTURE, class F>
concept has_traverse = requires(F f) {
    beman::transpose::traversable<STRUCTURE>.traverse(
        beman::transpose::applicative<std::optional<
            typename std::remove_cvref_t<decltype(beman::transpose::traversable<STRUCTURE>)>::element_type>>,
        f,
        std::declval<const STRUCTURE&>());
};

template <class STRUCTURE, class F>
concept has_for_each =
    requires(F f) { beman::transpose::traversable<STRUCTURE>.for_each(std::declval<const STRUCTURE&>(), f); };

struct Wrap {
    template <class T>
    auto operator()(const T& x) const -> std::optional<T> {
        return std::optional<T>{x};
    }
};

struct NotAContext {
    template <class T>
    auto operator()(const T&) const -> int {
        return 0;
    }
};

} // namespace

TEMPLATE_TEST_CASE("FingerTreeTraversableTest - InvalidTraverseIsASoftFailure",
                   "",
                   beman::fingertree::tree::FingerTree<int>,
                   beman::fingertree::tree::FingerTreeRope<>,
                   beman::fingertree::tree::FingerTreeIntervalIndex<std::string>,
                   beman::fingertree::tree::FingerTreePriorityQueue<int>,
                   beman::fingertree::tree::FingerTreeRandomAccess<int>) {
    STATIC_REQUIRE(has_traverse<TestType, Wrap>);
    STATIC_REQUIRE(!has_traverse<TestType, NotCallable>);
    STATIC_REQUIRE(!has_traverse<TestType, NotAContext>);
    STATIC_REQUIRE(has_for_each<TestType, Wrap>);
    STATIC_REQUIRE(!has_for_each<TestType, NotCallable>);
}
