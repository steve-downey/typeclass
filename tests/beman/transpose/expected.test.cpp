// tests/beman/transpose/expected.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/expected.hpp>

#include <beman/transpose.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <beman/transpose/error_set.hpp>

#include <expected>
#include <future>
#include <ios>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

namespace bt = beman::transpose;

namespace {

using errc = std::errc;
using exp_int = std::expected<int, errc>;

auto fail(errc code) -> exp_int { return exp_int{std::unexpect, code}; }

/** Only positive inputs succeed; the rest carry a distinguishable error. */
auto positive(int x) -> exp_int {
    if (x > 0) {
        return exp_int{x * 2};
    }
    return fail(errc::invalid_argument);
}

} // namespace

TEST_CASE("expected: applicative pure and invoke") {
    const auto &app = bt::applicative<exp_int>;

    REQUIRE(app.pure(7) == std::expected<int, errc>{7});

    auto sum =
        app.invoke([](int a, int b) { return a + b; }, exp_int{2}, exp_int{3});
    REQUIRE(sum == exp_int{5});
}

TEST_CASE("expected: invoke short-circuits and the leftmost error wins") {
    const auto &app = bt::applicative<exp_int>;
    auto add = [](int a, int b, int c) { return a + b + c; };

    // Effect order is observable and part of the contract: with two failing
    // operands, the LEFTMOST error is the one propagated.
    auto both_fail = app.invoke(add, exp_int{1}, fail(errc::invalid_argument),
                                fail(errc::result_out_of_range));
    REQUIRE_FALSE(both_fail.has_value());
    REQUIRE(both_fail.error() == errc::invalid_argument);

    auto later_fails = app.invoke(add, exp_int{1}, exp_int{2},
                                  fail(errc::result_out_of_range));
    REQUIRE_FALSE(later_fails.has_value());
    REQUIRE(later_fails.error() == errc::result_out_of_range);
}

TEST_CASE("expected: applicative map and the derived operations") {
    const auto &app = bt::applicative<exp_int>;

    auto mapped = app.map([](int x) { return x * 1.5; }, exp_int{4});
    REQUIRE(mapped == std::expected<double, errc>{6.0});

    auto mapped_error =
        app.map([](int x) { return x * 1.5; }, fail(errc::invalid_argument));
    REQUIRE_FALSE(mapped_error.has_value());
    REQUIRE(mapped_error.error() == errc::invalid_argument);

    REQUIRE(app.discard_first(exp_int{1}, exp_int{2}) == exp_int{2});
    REQUIRE(app.discard_second(exp_int{1}, exp_int{2}) == exp_int{1});
}

TEST_CASE("expected: monad bind short-circuits") {
    const auto &monad = bt::monad<exp_int>;

    auto to_text = [](int x) {
        return std::expected<std::string, errc>{std::to_string(x)};
    };

    REQUIRE(monad.bind(exp_int{12}, to_text) ==
            std::expected<std::string, errc>{"12"});

    auto short_circuited = monad.bind(fail(errc::invalid_argument), to_text);
    REQUIRE_FALSE(short_circuited.has_value());
    REQUIRE(short_circuited.error() == errc::invalid_argument);
}

TEST_CASE("expected: monad join flattens") {
    auto nested = std::expected<exp_int, errc>{exp_int{9}};
    REQUIRE(bt::join(nested) == exp_int{9});

    auto outer_failed =
        std::expected<exp_int, errc>{std::unexpect, errc::invalid_argument};
    REQUIRE(bt::join(outer_failed).error() == errc::invalid_argument);
}

TEST_CASE("expected: traverse over a vector transposes into one expected") {
    auto ok = bt::traverse(positive, std::vector<int>{1, 2, 3});
    REQUIRE(ok == std::expected<std::vector<int>, errc>{{2, 4, 6}});

    auto bad = bt::traverse(positive, std::vector<int>{1, -1, 3});
    REQUIRE_FALSE(bad.has_value());
    REQUIRE(bad.error() == errc::invalid_argument);
}

TEST_CASE("expected: traverse preserves shape and empty is pure") {
    auto empty = bt::traverse(positive, std::vector<int>{});
    REQUIRE(empty == std::expected<std::vector<int>, errc>{std::vector<int>{}});

    auto result = bt::traverse(positive, std::vector<int>{5, 4, 3, 2, 1});
    REQUIRE(result.has_value());
    REQUIRE(*result == std::vector<int>{10, 8, 6, 4, 2});
}

TEST_CASE("expected: transpose a vector of expected") {
    auto all_ok =
        bt::transpose(std::vector<exp_int>{exp_int{1}, exp_int{2}, exp_int{3}});
    REQUIRE(all_ok == std::expected<std::vector<int>, errc>{{1, 2, 3}});

    auto one_bad = bt::transpose(std::vector<exp_int>{
        exp_int{1}, fail(errc::result_out_of_range), exp_int{3}});
    REQUIRE_FALSE(one_bad.has_value());
    REQUIRE(one_bad.error() == errc::result_out_of_range);
}

TEST_CASE("expected: mixing two error types joins them into an error_set") {
    using mixed = bt::error_set<errc, std::io_errc>;
    const auto &app = bt::applicative<exp_int>;

    auto add = [](int a, int b) { return a + b; };
    std::expected<int, std::io_errc> io_ok{10};

    auto both_ok = app.invoke(add, exp_int{5}, io_ok);
    static_assert(std::is_same_v<decltype(both_ok), std::expected<int, mixed>>);
    REQUIRE(both_ok == std::expected<int, mixed>{15});

    // Each side's error widens into the joined set, keeping its identity.
    auto left_failed = app.invoke(add, fail(errc::invalid_argument), io_ok);
    REQUIRE_FALSE(left_failed.has_value());
    REQUIRE(left_failed.error().holds<errc>());

    std::expected<int, std::io_errc> io_bad{std::unexpect,
                                            std::io_errc::stream};
    auto right_failed = app.invoke(add, exp_int{5}, io_bad);
    REQUIRE_FALSE(right_failed.has_value());
    REQUIRE(right_failed.error().holds<std::io_errc>());
}

TEST_CASE("expected: a bare operand is promoted and leaves no trace") {
    const auto &app = bt::applicative<exp_int>;
    auto add = [](int a, int b) { return a + b; };

    // The bare 5 is empty-graded, so the join is a no-op and the result is
    // still expected<int, errc> -- NOT expected<int, error_set<errc>>.
    auto result = app.invoke(add, exp_int{37}, 5);
    static_assert(std::is_same_v<decltype(result), exp_int>);
    REQUIRE(result == exp_int{42});

    auto short_circuited = app.invoke(add, fail(errc::invalid_argument), 5);
    REQUIRE_FALSE(short_circuited.has_value());
    REQUIRE(short_circuited.error() == errc::invalid_argument);
}

TEST_CASE("expected: graded bind joins the continuation's error") {
    using mixed = bt::error_set<errc, std::io_errc>;
    const auto &monad = bt::monad<exp_int>;

    auto to_io = [](int x) {
        return std::expected<double, std::io_errc>{static_cast<double>(x)};
    };

    auto ok = monad.bind(exp_int{4}, to_io);
    static_assert(std::is_same_v<decltype(ok), std::expected<double, mixed>>);
    REQUIRE(ok == std::expected<double, mixed>{4.0});

    // The upstream error widens rather than being lost.
    auto upstream = monad.bind(fail(errc::invalid_argument), to_io);
    REQUIRE_FALSE(upstream.has_value());
    REQUIRE(upstream.error().holds<errc>());

    auto downstream = monad.bind(exp_int{4}, [](int) {
        return std::expected<double, std::io_errc>{std::unexpect,
                                                   std::io_errc::stream};
    });
    REQUIRE_FALSE(downstream.has_value());
    REQUIRE(downstream.error().holds<std::io_errc>());
}

TEST_CASE("expected: an existing error_set absorbs rather than nests") {
    using set_errc = bt::error_set<errc>;
    using mixed = bt::error_set<errc, std::io_errc>;

    const auto &app = bt::applicative<std::expected<int, set_errc>>;
    auto add = [](int a, int b) { return a + b; };

    std::expected<int, set_errc> already_a_set{3};
    std::expected<int, std::io_errc> io_ok{4};

    auto joined = app.invoke(add, already_a_set, io_ok);
    static_assert(std::is_same_v<decltype(joined), std::expected<int, mixed>>);
    REQUIRE(joined == std::expected<int, mixed>{7});
}

TEST_CASE("expected: for_each over the Identity traversable") {
    const auto &t = bt::traversable<bt::test::Identity<int>>;
    auto result = t.for_each(bt::test::Identity<int>{20}, positive);
    REQUIRE(result == std::expected<bt::test::Identity<int>, errc>{
                          bt::test::Identity<int>{40}});
}

// =========================================================================
// THE UNIT. std::expected<void, E> is a context whose element is the unit,
// std::monostate (docs/transpose/decisions.md#unit-payload-spelling). A
// callable returning void yields expected<void, E> from every object; the
// object over expected<void, E> maps pure(monostate) back to expected<void,
// E>, so the element reading and pure round-trip. Stage unit-and-closure of
// docs/transpose/transpose-surface-plan.md.
// =========================================================================

namespace {

using exp_void = std::expected<void, errc>;

/** The validation-pass shape: succeeds silently or reports why not. */
auto check_positive(int x) -> exp_void {
    if (x > 0) {
        return exp_void{};
    }
    return exp_void{std::unexpect, errc::invalid_argument};
}

} // namespace

TEST_CASE("expected: the element of expected<void, E> is the unit, and the "
          "void context is a registered applicative and monad") {
    STATIC_REQUIRE(
        std::is_same_v<bt::applicative_value_t<exp_void>, std::monostate>);
    STATIC_REQUIRE(bt::applicative_object<
                   std::remove_cvref_t<decltype(bt::applicative<exp_void>)>,
                   exp_void>);
    STATIC_REQUIRE(bt::applicative_object<
                   std::remove_cvref_t<
                       decltype(bt::accumulating_applicative<exp_void>)>,
                   exp_void>);
    STATIC_REQUIRE(
        bt::monad_object<std::remove_cvref_t<decltype(bt::monad<exp_void>)>,
                         exp_void>);
}

TEST_CASE("expected: pure inverts the element reading of the void context") {
    // On the void object, the unit embeds as expected<void, E> ...
    auto unit = bt::applicative<exp_void>.pure(std::monostate{});
    STATIC_REQUIRE(std::is_same_v<decltype(unit), exp_void>);
    REQUIRE(unit.has_value());

    // ... and on every other object, and for every other value, pure is what
    // it always was. A user's expected<monostate, E> is not a void expected.
    auto held = bt::applicative<exp_int>.pure(std::monostate{});
    STATIC_REQUIRE(
        std::is_same_v<decltype(held), std::expected<std::monostate, errc>>);
    auto other = bt::applicative<exp_void>.pure(7);
    STATIC_REQUIRE(std::is_same_v<decltype(other), exp_int>);
}

TEST_CASE("expected: a void-returning callable yields expected<void, E>, "
          "and is invoked exactly when every operand holds a value") {
    const auto &app = bt::applicative<exp_int>;
    int calls = 0;
    auto effect = [&calls](int a, int b) { calls += a + b; };

    auto ran = app.invoke(effect, exp_int{2}, exp_int{3});
    STATIC_REQUIRE(std::is_same_v<decltype(ran), exp_void>);
    REQUIRE(ran.has_value());
    REQUIRE(calls == 5);

    auto skipped = app.invoke(effect, exp_int{2}, fail(errc::invalid_argument));
    REQUIRE_FALSE(skipped.has_value());
    REQUIRE(skipped.error() == errc::invalid_argument);
    REQUIRE(calls == 5);
}

TEST_CASE("expected: a void operand hands its callable the unit") {
    const auto &app = bt::applicative<exp_void>;
    auto count = [](std::monostate, int x) { return x + 1; };

    auto both = app.invoke(count, check_positive(1), exp_int{41});
    STATIC_REQUIRE(std::is_same_v<decltype(both), exp_int>);
    REQUIRE(both == exp_int{42});

    // The void operand's failure propagates like any other operand's.
    auto failed = app.invoke(count, check_positive(-1), exp_int{41});
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(failed.error() == errc::invalid_argument);

    // Bare operands and the unit together: the second core.
    auto bare = app.invoke(count, check_positive(1), 9);
    REQUIRE(bare == exp_int{10});
}

TEST_CASE("expected: the void context mixes grades like any other") {
    using exp_void_io = std::expected<void, std::io_errc>;
    const auto &app = bt::applicative<exp_void>;
    auto both = [](std::monostate, std::monostate) { return 1; };

    auto joined = app.invoke(both, exp_void{}, exp_void_io{});
    STATIC_REQUIRE(
        std::is_same_v<decltype(joined),
                       std::expected<int, bt::error_set<errc, std::io_errc>>>);
    REQUIRE(joined == decltype(joined){1});

    auto silent = app.invoke([](std::monostate, std::monostate) {}, exp_void{},
                             exp_void_io{});
    STATIC_REQUIRE(
        std::is_same_v<decltype(silent),
                       std::expected<void, bt::error_set<errc, std::io_errc>>>);
    REQUIRE(silent.has_value());
}

TEST_CASE("expected: the accumulating object collects void operands' errors") {
    const auto &accumulating = bt::accumulating_applicative<exp_void>;
    using exp_void_io = std::expected<void, std::io_errc>;
    auto io_fail = exp_void_io{std::unexpect, std::io_errc::stream};

    auto both = accumulating.invoke([](std::monostate, std::monostate) {},
                                    check_positive(-1), io_fail);
    STATIC_REQUIRE(
        std::is_same_v<decltype(both),
                       std::expected<void, bt::error_set<errc, std::io_errc>>>);
    REQUIRE_FALSE(both.has_value());
    REQUIRE(both.error().witness_count() == 2);
    REQUIRE(both.error().holds<errc>());
    REQUIRE(both.error().holds<std::io_errc>());
}

TEST_CASE("expected: bind hands a void computation's continuation the unit, "
          "and a continuation may return a void expected") {
    auto from_unit =
        bt::bind(exp_void{}, [](std::monostate) { return exp_int{42}; });
    STATIC_REQUIRE(std::is_same_v<decltype(from_unit), exp_int>);
    REQUIRE(from_unit == exp_int{42});

    auto to_unit =
        bt::bind(exp_int{3}, [](int x) { return check_positive(x); });
    STATIC_REQUIRE(std::is_same_v<decltype(to_unit), exp_void>);
    REQUIRE(to_unit.has_value());

    auto failed_first =
        bt::bind(exp_void{std::unexpect, errc::invalid_argument},
                 [](std::monostate) { return exp_int{42}; });
    REQUIRE_FALSE(failed_first.has_value());

    // Sequencing into a different error alternative joins, void or not.
    using exp_void_io = std::expected<void, std::io_errc>;
    auto joined = bt::bind(exp_int{3}, [](int) { return exp_void_io{}; });
    STATIC_REQUIRE(
        std::is_same_v<decltype(joined),
                       std::expected<void, bt::error_set<errc, std::io_errc>>>);
    REQUIRE(joined.has_value());
}

TEST_CASE("expected: void itself is still not a value a context holds") {
    // The tripwire of stage unit-and-closure: the unit is a value; void is
    // not, and nothing here admitted it.
    STATIC_REQUIRE(!bt::detail::holdable_value<void>);
    STATIC_REQUIRE(bt::detail::holdable_value<std::monostate>);
}

// =========================================================================
// RAISE. The dual of pure: the generic entry into the failure state, so that
// graded code can fail without naming the carrier
// (docs/transpose/decisions.md#failure-injection-spelling). Carrier spelling
// stays lazy: raising the declared alternative leaves the spelling alone, and
// a different alternative widens it into an error set. Stage failure-side of
// docs/transpose/transpose-surface-plan.md.
// =========================================================================

TEST_CASE("expected: raise with the declared alternative keeps the spelling") {
    const auto &app = bt::applicative<exp_int>;
    auto failed = app.raise(errc::invalid_argument);
    STATIC_REQUIRE(std::is_same_v<decltype(failed), exp_int>);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(failed.error() == errc::invalid_argument);

    // The operation object names the context, as pure does.
    auto raised = bt::raise<exp_int>(errc::result_out_of_range);
    STATIC_REQUIRE(std::is_same_v<decltype(raised), exp_int>);
    REQUIRE(raised.error() == errc::result_out_of_range);
}

TEST_CASE("expected: raise with a different alternative joins the grades") {
    auto widened = bt::raise<exp_int>(std::io_errc::stream);
    STATIC_REQUIRE(
        std::is_same_v<decltype(widened),
                       std::expected<int, bt::error_set<errc, std::io_errc>>>);
    REQUIRE_FALSE(widened.has_value());
    REQUIRE(widened.error().holds<std::io_errc>());
    REQUIRE_FALSE(widened.error().holds<errc>());

    // Onto a set: a member leaves the set alone, a newcomer joins it.
    using exp_set = std::expected<int, bt::error_set<errc, std::io_errc>>;
    auto member = bt::raise<exp_set>(errc::invalid_argument);
    STATIC_REQUIRE(std::is_same_v<decltype(member), exp_set>);
    auto newcomer = bt::raise<exp_set>(std::future_errc::broken_promise);
    STATIC_REQUIRE(
        std::is_same_v<decltype(newcomer),
                       std::expected<int, bt::error_set<errc, std::io_errc,
                                                        std::future_errc>>>);
    REQUIRE(newcomer.error().holds<std::future_errc>());

    // A whole set may be raised; its members join.
    auto raised_set =
        bt::raise<exp_int>(bt::error_set<std::io_errc>{std::io_errc::stream});
    STATIC_REQUIRE(
        std::is_same_v<decltype(raised_set),
                       std::expected<int, bt::error_set<errc, std::io_errc>>>);
}

TEST_CASE("expected: raise from the uniform spelling, the void context, and "
          "the accumulating object") {
    // The uniform spelling of the empty grade acquires its first alternative.
    auto from_empty =
        bt::raise<std::expected<int, bt::error_set<>>>(errc::invalid_argument);
    STATIC_REQUIRE(std::is_same_v<decltype(from_empty),
                                  std::expected<int, bt::error_set<errc>>>);

    using exp_void = std::expected<void, errc>;
    auto void_failed = bt::raise<exp_void>(errc::invalid_argument);
    STATIC_REQUIRE(std::is_same_v<decltype(void_failed), exp_void>);
    REQUIRE_FALSE(void_failed.has_value());

    auto accumulated =
        bt::accumulating_applicative<exp_int>.raise(std::io_errc::stream);
    STATIC_REQUIRE(
        std::is_same_v<decltype(accumulated),
                       std::expected<int, bt::error_set<errc, std::io_errc>>>);
    REQUIRE(accumulated.error().holds<std::io_errc>());
}

namespace {

// Availability is asked through NAMED CONCEPTS: a bare requires-expression at
// block scope hard-errors on an invalid expression instead of yielding false.
template <class CONTEXT>
concept has_zero = requires { bt::zero<CONTEXT>(); };
template <class CONTEXT, class ERROR>
concept has_raise = requires(ERROR error) { bt::raise<CONTEXT>(error); };
} // namespace

TEST_CASE("expected: there is no zero, because its failure state needs a "
          "witness") {
    STATIC_REQUIRE(!has_zero<exp_int>);
    STATIC_REQUIRE(has_zero<std::optional<int>>);
    STATIC_REQUIRE(has_raise<exp_int, errc>);
}
