// tests/beman/transpose/traverse.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/traverse.hpp>

#include <beman/transpose/sequence.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

TEST_CASE("traverse: front-door over vector with optional-returning function") {
    auto positive = [](int x) {
        return x > 0 ? std::optional<int>{x * 2} : std::optional<int>{};
    };
    auto ok = bt::traverse(positive, std::vector<int>{1, 2, 3});
    REQUIRE(ok == std::optional<std::vector<int>>{{2, 4, 6}});

    auto bad = bt::traverse(positive, std::vector<int>{1, -1, 3});
    REQUIRE(bad == std::optional<std::vector<int>>{});
}

TEST_CASE("traverse: preserves element order and count") {
    auto wrap = [](int x) { return std::optional<int>{x}; };
    auto result = bt::traverse(wrap, std::vector<int>{5, 4, 3, 2, 1});
    REQUIRE(result.has_value());
    REQUIRE(*result == std::vector<int>{5, 4, 3, 2, 1});
}

TEST_CASE("traverse: empty vector yields pure empty structure") {
    auto wrap = [](int x) { return std::optional<int>{x}; };
    auto result = bt::traverse(wrap, std::vector<int>{});
    REQUIRE(result == std::optional<std::vector<int>>{std::vector<int>{}});
}

TEST_CASE(
    "traverse: for_each on Identity infers applicative from return type") {
    const auto &t = bt::traversable<bt::test::Identity<int>>;
    auto result = t.for_each(bt::test::Identity<int>{20},
                             [](int x) { return std::optional<int>{x + 1}; });
    REQUIRE(result == std::optional<bt::test::Identity<int>>{
                          bt::test::Identity<int>{21}});
}

TEST_CASE("traverse: the default policy retains the first failure and still "
          "applies function to every element") {
    // The wording used to say the default policy "stops at the first failing
    // element", which reads as though later elements' contexts are never
    // produced. They are: independent contextual composition means an
    // element's context does not depend on any other element's value, so
    // every context is computed before any composition sees it, and only
    // the reconstruction of the value in context stops. This pins the
    // difference the old phrasing hid.
    int applications = 0;
    auto fails_on_two = [&applications](int element) {
        ++applications;
        return element == 2 ? std::optional<int>{}
                            : std::optional<int>{element};
    };

    auto result = bt::traverse(fails_on_two, std::vector<int>{1, 2, 3, 4});

    REQUIRE_FALSE(result.has_value());
    REQUIRE(applications == 4);
}

// The derived operations are constant-evaluable: each is pinned by a
// static_assert, traversing a vector in the optional applicative. The
// expected vectors are in order, so a traversal that reorders fails.
TEST_CASE("traverse: the derived operations are constexpr") {
    constexpr const auto &t = bt::traversable<std::vector<std::optional<int>>>;
    constexpr const auto &ti = bt::traversable<std::vector<int>>;
    constexpr auto some = [](int x) { return std::optional<int>{x * 10}; };
    static_assert(t.transpose(std::vector<std::optional<int>>{1, 2, 3}) ==
                  std::optional<std::vector<int>>{std::vector<int>{1, 2, 3}});
    static_assert(t.transpose(std::vector<std::optional<int>>{1, {}, 3}) ==
                  std::optional<std::vector<int>>{});
    static_assert(ti.traverse_with(ti, some, std::vector<int>{1, 2}) ==
                  std::optional<std::vector<int>>{std::vector<int>{10, 20}});
    static_assert(ti.traverse_with(ti, bt::applicative<std::optional<int>>,
                                   some, std::vector<int>{1, 2}) ==
                  std::optional<std::vector<int>>{std::vector<int>{10, 20}});
    static_assert(t.transpose_with(t, std::vector<std::optional<int>>{4, 5}) ==
                  std::optional<std::vector<int>>{std::vector<int>{4, 5}});
    static_assert(ti.for_each(std::vector<int>{1, 2}, some) ==
                  std::optional<std::vector<int>>{std::vector<int>{10, 20}});
    SUCCEED();
}

// steve-downey/typeclass#8: every Traversable entry point must answer a
// probe with a constraint that does not match, never with a diagnostic from
// inside a body whose return type is deduced.
namespace {

struct NotCallable {};

template <class MAP, class APPLICATIVE, class F, class T>
concept has_traverse = requires(const MAP &m, const APPLICATIVE &a, F f) {
    m.traverse(a, f, std::declval<T>());
};

template <class MAP, class F, class T>
concept has_for_each =
    requires(const MAP &m, F f) { m.for_each(std::declval<T>(), f); };

template <class MAP, class F, class T>
concept has_traverse_with =
    requires(const MAP &m, F f) { m.traverse_with(m, f, std::declval<T>()); };

template <class MAP, class APPLICATIVE, class F, class T>
concept has_traverse_with_applicative =
    requires(const MAP &m, const APPLICATIVE &a, F f) {
        m.traverse_with(m, a, f, std::declval<T>());
    };

template <class MAP, class T>
concept has_transpose =
    requires(const MAP &m) { m.transpose(std::declval<T>()); };

template <class MAP, class T>
concept has_transpose_with =
    requires(const MAP &m) { m.transpose_with(m, std::declval<T>()); };

} // namespace

TEST_CASE("traverse: the vector object's traverse is a soft failure for a "
          "call it cannot make") {
    using VI = std::vector<int>;
    using Map = std::remove_cvref_t<decltype(bt::traversable<VI>)>;
    using App =
        std::remove_cvref_t<decltype(bt::applicative<std::optional<int>>)>;
    auto wrap = [](int x) { return std::optional<int>{x}; };
    auto not_a_context = [](int x) { return x; };
    auto consuming = [](int &&x) { return std::optional<int>{x}; };

    STATIC_REQUIRE(has_traverse<Map, App, decltype(wrap), const VI &>);
    STATIC_REQUIRE(has_traverse<Map, App, decltype(wrap), VI>);
    STATIC_REQUIRE(!has_traverse<Map, App, NotCallable, const VI &>);
    STATIC_REQUIRE(!has_traverse<Map, App, NotCallable, VI>);
    STATIC_REQUIRE(!has_traverse<Map, App, decltype(not_a_context), VI>);
    STATIC_REQUIRE(!has_traverse<Map, std::false_type, decltype(wrap), VI>);
    STATIC_REQUIRE(
        !has_traverse<Map, App, decltype(wrap), std::vector<std::string>>);

    // An rvalue vector presents rvalue elements and an lvalue one presents
    // const lvalues, and the constraint asks about exactly that call.
    STATIC_REQUIRE(has_traverse<Map, App, decltype(consuming), VI>);
    STATIC_REQUIRE(!has_traverse<Map, App, decltype(consuming), const VI &>);
    STATIC_REQUIRE(!has_traverse<Map, App, decltype(consuming), VI &>);

    const auto &traversable = bt::traversable<VI>;
    REQUIRE(traversable.traverse(bt::applicative<std::optional<int>>, consuming,
                                 VI{1, 2}) == std::optional<VI>{VI{1, 2}});
}

TEST_CASE("traverse: the derived Traversable operations are soft failures "
          "for a call they cannot make") {
    using VI = std::vector<int>;
    using VO = std::vector<std::optional<int>>;
    using Map = std::remove_cvref_t<decltype(bt::traversable<VI>)>;
    using OptionalsMap = std::remove_cvref_t<decltype(bt::traversable<VO>)>;
    using App =
        std::remove_cvref_t<decltype(bt::applicative<std::optional<int>>)>;
    auto wrap = [](int x) { return std::optional<int>{x}; };
    using Wrap = decltype(wrap);

    STATIC_REQUIRE(has_for_each<Map, Wrap, VI>);
    STATIC_REQUIRE(!has_for_each<Map, NotCallable, VI>);
    STATIC_REQUIRE(!has_for_each<Map, NotCallable, const VI &>);

    STATIC_REQUIRE(has_traverse_with<Map, Wrap, VI>);
    STATIC_REQUIRE(!has_traverse_with<Map, NotCallable, VI>);
    STATIC_REQUIRE(has_traverse_with_applicative<Map, App, Wrap, VI>);
    STATIC_REQUIRE(!has_traverse_with_applicative<Map, App, NotCallable, VI>);

    STATIC_REQUIRE(has_transpose<OptionalsMap, VO>);
    STATIC_REQUIRE(has_transpose<OptionalsMap, const VO &>);
    STATIC_REQUIRE(!has_transpose<OptionalsMap, std::vector<std::string>>);
    STATIC_REQUIRE(!has_transpose<OptionalsMap, VI>);
    STATIC_REQUIRE(has_transpose_with<OptionalsMap, VO>);
    STATIC_REQUIRE(!has_transpose_with<OptionalsMap, std::vector<std::string>>);

    REQUIRE(bt::traversable<VI>.for_each(VI{1, 2}, wrap) ==
            std::optional<VI>{VI{1, 2}});
    REQUIRE(bt::traversable<VO>.transpose(VO{1, 2}) ==
            std::optional<VI>{VI{1, 2}});
}
