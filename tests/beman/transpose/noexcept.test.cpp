// tests/beman/transpose/noexcept.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// The exception specifications of the typeclass operations
// (docs/transpose/decisions.md#operation-noexcept). Every operation's
// specification is computed, never promised: from the callable's invocation,
// the carrier's constructors, and for a derived operation the route it
// takes. So the tests here are of propagation. A throwing callable's
// `noexcept(false)` must be visible through every layer between the caller
// and the instance, and a non-throwing one must come back `noexcept(true)`
// exactly where the carrier's own constructors are.
//
// The carrier's constructors are the standard library's, and the standard
// leaves `optional`'s converting constructor without an exception
// specification; libstdc++ gives it a conditional one. The assertions are
// therefore stated as equalities with the trait the computation reads, which
// hold on every library, and the bare `noexcept(true)` form is asserted where
// that library is known to make it so.
//
// `[[nodiscard]]` is an attribute, which no expression can observe. The audit
// is recorded in the section at the end and was verified by hand with
// `-Werror=unused-result` on a scratch file dropping each operation's result
// (docs/transpose/decisions.md#operation-noexcept, 2026-11-01).

#include <beman/transpose/apply.hpp>
#include <beman/transpose/apply.hpp> // re-inclusion / idempotency check

#include <beman/transpose/eliminate.hpp>
#include <beman/transpose/error_set.hpp>
#include <beman/transpose/expected.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/optional.hpp>
#include <beman/transpose/overloaded.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bt = beman::transpose;

namespace {

// Two callables that differ only in what they promise. Neither throws; the
// first says so and the second does not, which is all the type system reads.
struct nothrow_increment {
    constexpr auto operator()(int x) const noexcept -> int { return x + 1; }
};

struct throwing_increment {
    constexpr auto operator()(int x) const -> int { return x + 1; }
};

struct nothrow_add {
    constexpr auto operator()(int x, int y) const noexcept -> int {
        return x + y;
    }
};

struct throwing_add {
    constexpr auto operator()(int x, int y) const -> int { return x + y; }
};

// Continuations for bind, and effects for traverse, in both flavours.
struct nothrow_lift {
    constexpr auto operator()(int x) const noexcept -> std::optional<int> {
        return std::optional<int>{x};
    }
};

struct throwing_lift {
    constexpr auto operator()(int x) const -> std::optional<int> {
        return std::optional<int>{x};
    }
};

// What the optional instance's computation reads for an `int` result: the
// carrier's own constructor from the value.
inline constexpr bool optional_int_nothrow =
    std::is_nothrow_constructible_v<std::optional<int>, int>;

using Err = std::string;
using E1 = std::expected<int, Err>;

} // namespace

TEST_CASE("noexcept test bootstrap", "[noexcept]") { REQUIRE(true); }

TEST_CASE("the applicative basis on optional propagates the callable's answer",
          "[noexcept][optional]") {
    using O = std::optional<int>;
    const O a{1};
    const O b{2};

    // invoke: the callable's own specification, and the carrier's constructor
    // from the result.
    STATIC_REQUIRE(!noexcept(bt::invoke(throwing_add{}, a, b)));
    STATIC_REQUIRE(noexcept(bt::invoke(nothrow_add{}, a, b)) ==
                   optional_int_nothrow);

    // The same through the instance object directly, and through the
    // derived operations that route to it.
    STATIC_REQUIRE(!noexcept(bt::applicative<O>.invoke(throwing_add{}, a, b)));
    STATIC_REQUIRE(noexcept(bt::applicative<O>.invoke(nothrow_add{}, a, b)) ==
                   optional_int_nothrow);
    STATIC_REQUIRE(!noexcept(bt::zip_with(throwing_add{}, a, b)));
    STATIC_REQUIRE(noexcept(bt::zip_with(nothrow_add{}, a, b)) ==
                   optional_int_nothrow);
    STATIC_REQUIRE(!noexcept(bt::applicative<O>.map(throwing_increment{}, a)));
    STATIC_REQUIRE(noexcept(bt::applicative<O>.map(nothrow_increment{}, a)) ==
                   optional_int_nothrow);

    // discard_first and discard_second apply a non-throwing evaluator, so
    // only the carrier's constructor is left to ask about.
    STATIC_REQUIRE(noexcept(bt::discard_first(a, b)) == optional_int_nothrow);
    STATIC_REQUIRE(noexcept(bt::discard_second(a, b)) == optional_int_nothrow);

    // pure: the carrier's constructor from the value, and nothing else. An
    // optional<string> built from an rvalue string is a move, which does not
    // allocate; built from an lvalue it is a copy, which may.
    STATIC_REQUIRE(noexcept(bt::pure<O>(1)) == optional_int_nothrow);
    const std::string s;
    STATIC_REQUIRE(!noexcept(bt::pure<std::optional<std::string>>(s)));
    STATIC_REQUIRE(
        noexcept(bt::pure<std::optional<std::string>>(std::string{})) ==
        std::is_nothrow_constructible_v<std::optional<std::string>,
                                        std::string>);

    // zero builds a disengaged optional, which never throws.
    STATIC_REQUIRE(noexcept(bt::zero<O>()));

    REQUIRE(bt::invoke(nothrow_add{}, a, b) == O{3});
}

TEST_CASE("the functor and monad bases on optional propagate the same way",
          "[noexcept][optional]") {
    using O = std::optional<int>;
    const O a{1};

    STATIC_REQUIRE(!noexcept(bt::fmap(throwing_increment{}, a)));
    STATIC_REQUIRE(noexcept(bt::fmap(nothrow_increment{}, a)) ==
                   optional_int_nothrow);

    // bind constructs the continuation's result from what it returns: a
    // move of an optional<int>, which is non-throwing on every library.
    STATIC_REQUIRE(!noexcept(bt::bind(a, throwing_lift{})));
    STATIC_REQUIRE(noexcept(bt::bind(a, nothrow_lift{})));

    // join is bind with the identity continuation: one move of the inner
    // optional.
    STATIC_REQUIRE(noexcept(bt::join(std::optional<O>{a})));

    // The monad object's derived fmap and invoke route through bind with
    // continuations that embed with pure, so the carrier's constructor
    // appears again.
    STATIC_REQUIRE(!noexcept(bt::monad<O>.fmap(throwing_increment{}, a)));
    STATIC_REQUIRE(noexcept(bt::monad<O>.fmap(nothrow_increment{}, a)) ==
                   optional_int_nothrow);
    STATIC_REQUIRE(!noexcept(bt::monad<O>.invoke(throwing_add{}, a, a)));
    STATIC_REQUIRE(noexcept(bt::monad<O>.invoke(nothrow_add{}, a, a)) ==
                   optional_int_nothrow);

    REQUIRE(bt::bind(a, nothrow_lift{}) == a);
}

#if defined(__GLIBCXX__)
TEST_CASE("on libstdc++ a non-throwing callable over optional<int> is "
          "noexcept(true) outright",
          "[noexcept][optional]") {
    // libstdc++ gives optional's converting constructor a conditional
    // specification, so every equality above collapses to `true` here. This
    // is the shape the stage asked for; the equalities are the portable
    // statement of the same fact.
    using O = std::optional<int>;
    const O a{1};
    STATIC_REQUIRE(optional_int_nothrow);
    STATIC_REQUIRE(noexcept(bt::invoke(nothrow_add{}, a, a)));
    STATIC_REQUIRE(noexcept(bt::fmap(nothrow_increment{}, a)));
    STATIC_REQUIRE(noexcept(bt::bind(a, nothrow_lift{})));
    STATIC_REQUIRE(noexcept(bt::pure<O>(1)));
    STATIC_REQUIRE(
        noexcept(bt::pure<std::optional<std::string>>(std::string{})));
    REQUIRE(true);
}
#endif

TEST_CASE("traverse over vector into optional propagates noexcept(false)",
          "[noexcept][traverse]") {
    const std::vector<int> v{1, 2, 3};

    // A throwing effect is visible through the operation object, the
    // traversable object and the vector instance.
    STATIC_REQUIRE(!noexcept(bt::traverse(throwing_lift{}, v)));
    STATIC_REQUIRE(!noexcept(bt::traversable<std::vector<int>>.traverse(
        bt::applicative<std::optional<int>>, throwing_lift{}, v)));

    // So is a non-throwing one: the vector traversal collects into a vector,
    // which allocates, and carries no specification. The pipeline reports
    // the instance's answer rather than promising on its behalf.
    STATIC_REQUIRE(!noexcept(bt::traverse(nothrow_lift{}, v)));
    STATIC_REQUIRE(!noexcept(bt::transpose(std::vector<std::optional<int>>{})));

    REQUIRE(bt::traverse(nothrow_lift{}, v) ==
            std::optional<std::vector<int>>{{1, 2, 3}});
}

TEST_CASE("the expected instances compute from the callable and the carrier",
          "[noexcept][expected]") {
    const E1 a{1};
    const E1 b{2};

    // The failure path copies the error alternative, a string here, so even
    // a non-throwing callable leaves the composition potentially throwing.
    STATIC_REQUIRE(!noexcept(bt::invoke(nothrow_add{}, a, b)));
    STATIC_REQUIRE(!noexcept(bt::invoke(throwing_add{}, a, b)));

    // With a trivially copyable error alternative the callable decides, and
    // what remains is the carrier's two constructors: from the value, and
    // from the error.
    using E2 = std::expected<int, int>;
    const E2 c{1};
    constexpr bool from_value = std::is_nothrow_constructible_v<E2, int>;
    constexpr bool from_error =
        std::is_nothrow_constructible_v<E2, std::unexpect_t, int>;
    STATIC_REQUIRE(!noexcept(bt::invoke(throwing_add{}, c, c)));
    STATIC_REQUIRE(noexcept(bt::invoke(nothrow_add{}, c, c)) ==
                   (from_value && from_error));
    STATIC_REQUIRE(noexcept(bt::pure<E2>(1)) == from_value);
    STATIC_REQUIRE(noexcept(bt::raise<E2>(7)) == from_error);

    // The accumulating object adds the combination of two witnesses, which
    // for a plain `int` alternative is a copy.
    STATIC_REQUIRE(noexcept(bt::accumulating_applicative<E2>.invoke(
                       nothrow_add{}, c, c)) == (from_value && from_error));

    REQUIRE(bt::invoke(nothrow_add{}, c, c) == E2{2});
}

TEST_CASE("the eliminators compute from their arms", "[noexcept][eliminate]") {
    using V = std::variant<int, double>;
    const V v{1};

    auto nothrow_arms = bt::overloaded{
        [](int x) noexcept { return x; },
        [](double d) noexcept { return static_cast<int>(d); },
    };
    auto throwing_arms = bt::overloaded{
        [](int x) { return x; },
        [](double d) { return static_cast<int>(d); },
    };
    STATIC_REQUIRE(noexcept(bt::eliminate(v, nothrow_arms)));
    STATIC_REQUIRE(!noexcept(bt::eliminate(v, throwing_arms)));

    auto nothrow_indexed = [](auto, auto x) noexcept {
        return static_cast<int>(x);
    };
    auto throwing_indexed = [](auto, auto x) { return static_cast<int>(x); };
    STATIC_REQUIRE(noexcept(bt::eliminate_indexed(v, nothrow_indexed)));
    STATIC_REQUIRE(!noexcept(bt::eliminate_indexed(v, throwing_indexed)));

    using S = bt::error_set<int, char>;
    const std::expected<int, S> computation{std::unexpect, 3};
    STATIC_REQUIRE(noexcept(bt::fold_witnesses(
        computation.error(), 0,
        [](int acc, const auto &e) noexcept { return acc + int(e); })));
    STATIC_REQUIRE(!noexcept(
        bt::fold_witnesses(computation.error(), 0, [](int acc, const auto &e) {
            return acc + int(e);
        })));
    STATIC_REQUIRE(noexcept(bt::collapse(
        computation.error(), [](const auto &e) noexcept { return int(e); })));
    STATIC_REQUIRE(!noexcept(bt::collapse(
        computation.error(), [](const auto &e) { return int(e); })));
    // recover re-indexes the carrier at the narrowed grade, so the result's
    // constructors from the value and from the surviving set are what is
    // left once the handler is non-throwing.
    using Narrowed = std::expected<int, bt::error_set<char>>;
    STATIC_REQUIRE(
        noexcept(
            bt::recover<int>(computation, [](int e) noexcept { return e; })) ==
        (std::is_nothrow_constructible_v<Narrowed, const int &> &&
         std::is_nothrow_constructible_v<Narrowed, std::unexpect_t,
                                         const bt::error_set<char> &>));
    STATIC_REQUIRE(
        !noexcept(bt::recover<int>(computation, [](int e) { return e; })));
    STATIC_REQUIRE(noexcept(bt::fallible(1)));

    REQUIRE(bt::eliminate(v, nothrow_arms) == 1);
}

TEST_CASE("[[nodiscard]] audit", "[noexcept][nodiscard]") {
    // An attribute cannot be tested by an expression. Verified by hand on
    // 2026-11-01 with `g++ -std=c++23 -fsyntax-only -Werror=unused-result`
    // over a scratch translation unit that dropped the result of each of:
    // invoke, ap, pure, zero, zip_with, discard_first, discard_second, fmap,
    // replace, bind, join, traverse, transpose, the derived map and lift on
    // an applicative object, and length, empty and fold_left on a vector --
    // each was diagnosed -- and for_each on the vector traversable, which
    // is the one derived operation whose result may be dropped and which
    // compiled silently. See docs/transpose/decisions.md#operation-noexcept.
    REQUIRE(true);
}
