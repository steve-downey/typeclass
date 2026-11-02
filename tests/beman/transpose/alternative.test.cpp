// tests/beman/transpose/alternative.test.cpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/alternative.hpp>
#include <beman/transpose/alternative.hpp> // Re-inclusion: verifies include guard

#include <beman/transpose/error_set.hpp>
#include <beman/transpose/expected.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/optional.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <ios>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bt = beman::transpose;

namespace {

using opt_int = std::optional<int>;
using opt_str = std::optional<std::string>;
using exp_int = std::expected<int, std::errc>;
using exp_str = std::expected<std::string, std::errc>;
using exp_int_io = std::expected<int, std::io_errc>;
using exp_void = std::expected<void, std::errc>;
using mixed = bt::error_set<std::errc, std::io_errc>;
using exp_mixed = std::expected<int, mixed>;

// Availability is asked through named concepts: a bare requires-expression at
// block scope hard-errors on an invalid expression instead of yielding false.
template <class... OPERANDS>
concept has_alt = requires(OPERANDS... operands) { bt::alt(operands...); };
template <class CONTEXT>
concept has_alternative_zero = requires { bt::alternative<CONTEXT>.zero(); };

} // namespace

TEST_CASE("alternative: header is idempotent") { REQUIRE(true); }

TEST_CASE("alternative: the registered instances are alternative objects") {
    STATIC_REQUIRE(
        bt::alternative_object<decltype(bt::alternative<opt_int>), opt_int>);
    STATIC_REQUIRE(
        bt::alternative_object<decltype(bt::alternative<exp_int>), exp_int>);
    STATIC_REQUIRE(
        bt::alternative_impl<bt::optional_alternative_impl<int>, opt_int>);
    STATIC_REQUIRE(
        bt::alternative_impl<bt::expected_alternative_impl<int, std::errc>,
                             exp_int>);
    // optional has an identity; expected's failure state needs a witness.
    STATIC_REQUIRE(has_alternative_zero<opt_int>);
    STATIC_REQUIRE(!has_alternative_zero<exp_int>);
    // A context with no disjunction has no instance.
    STATIC_REQUIRE(
        !bt::has_instance_v<decltype(bt::alternative<std::vector<int>>)>);
}

// -- optional ---------------------------------------------------------------

TEST_CASE("alternative: optional yields the leftmost engaged operand") {
    opt_int a{1};
    opt_int b{2};
    opt_int none;
    STATIC_REQUIRE(std::is_same_v<decltype(bt::alt(a, b)), opt_int>);
    REQUIRE(bt::alt(a, b) == opt_int{1});
    REQUIRE(bt::alt(none, b) == opt_int{2});
    REQUIRE(bt::alt(none, none) == opt_int{});
    REQUIRE(bt::alt(none, none, b) == opt_int{2});
    // The operation object agrees with the lookup object it dispatches
    // through, and with the explicit override.
    REQUIRE(bt::alt(none, b) == bt::alternative<opt_int>.alt(none, b));
    REQUIRE(bt::alt(none, b) == bt::alternative<opt_int>.alt_with(
                                    bt::alternative<opt_int>, none, b));
}

TEST_CASE("alternative: differing value types join into a variant in operand "
          "order, holding the chosen value at its own alternative") {
    opt_int none;
    opt_str s{"two"};
    auto joined = bt::alt(none, s);
    STATIC_REQUIRE(
        std::is_same_v<decltype(joined),
                       std::optional<std::variant<int, std::string>>>);
    REQUIRE(joined.has_value());
    REQUIRE(std::get<std::string>(*joined) == "two");

    auto first = bt::alt(opt_int{1}, s);
    REQUIRE(std::get<int>(*first) == 1);

    // Duplicates collapse, keeping the first position; the join is not
    // sorted, as eliminate's is not.
    STATIC_REQUIRE(
        std::is_same_v<decltype(bt::alt(s, none, s)),
                       std::optional<std::variant<std::string, int>>>);
}

TEST_CASE("alternative: a bare last operand makes the disjunction total") {
    opt_int none;
    opt_int one{1};
    auto fallback = bt::alt(none, 7);
    STATIC_REQUIRE(std::is_same_v<decltype(fallback), int>);
    REQUIRE(fallback == 7);
    REQUIRE(bt::alt(one, 7) == 1);

    auto joined = bt::alt(none, opt_str{}, 2.5);
    STATIC_REQUIRE(std::is_same_v<decltype(joined),
                                  std::variant<int, std::string, double>>);
    REQUIRE(std::get<double>(joined) == 2.5);

    // Only the last operand may be bare: an operand after one that cannot
    // fail would never be consulted.
    STATIC_REQUIRE(!has_alt<opt_int, int, opt_int>);
    // A carrier of another context is not a bare value: a disjunction is
    // over one carrier.
    STATIC_REQUIRE(!has_alt<opt_int, exp_int>);
    STATIC_REQUIRE(!has_alt<exp_int, opt_int>);
    // Two operands at least.
    STATIC_REQUIRE(!has_alt<opt_int>);
}

TEST_CASE("alternative: an rvalue operand's value is moved through") {
    using opt_ptr = std::optional<std::unique_ptr<int>>;
    auto taken = bt::alt(opt_ptr{}, opt_ptr{std::make_unique<int>(7)});
    REQUIRE(taken.has_value());
    REQUIRE(**taken == 7);

    auto total = bt::alt(opt_ptr{}, std::make_unique<int>(8));
    STATIC_REQUIRE(std::is_same_v<decltype(total), std::unique_ptr<int>>);
    REQUIRE(*total == 8);
}

TEST_CASE("alternative: zero is a two-sided identity of alt on optional") {
    opt_int one{1};
    opt_int none;
    const auto zero = bt::alternative<opt_int>.zero();
    REQUIRE(zero == bt::zero<opt_int>());
    REQUIRE(bt::alt(zero, one) == one);
    REQUIRE(bt::alt(one, zero) == one);
    REQUIRE(bt::alt(zero, none) == none);
    REQUIRE(bt::alt(zero, zero) == zero);
}

TEST_CASE("alternative: alt is associative, and the n-ary form is its fold") {
    opt_int a;
    opt_int b{2};
    opt_int c{3};
    REQUIRE(bt::alt(bt::alt(a, b), c) == bt::alt(a, bt::alt(b, c)));
    REQUIRE(bt::alt(a, b, c) == bt::alt(bt::alt(a, b), c));
}

TEST_CASE("alternative: first_monoid is the optional disjunction, with zero "
          "as its identity") {
    bt::first_monoid<int> m;
    opt_int a;
    opt_int b{2};
    opt_int c{3};
    REQUIRE(m.identity() == bt::alternative<opt_int>.zero());
    REQUIRE(m.combine(a, b) == bt::alt(a, b));
    REQUIRE(m.combine(b, c) == bt::alt(b, c));
    REQUIRE(m.combine(a, a) == bt::alt(a, a));
    REQUIRE(m.combine(c, b) == bt::alt(c, b));
}

TEST_CASE("alternative: the optional disjunction is constexpr") {
    constexpr auto chosen = bt::alt(opt_int{}, opt_int{5});
    STATIC_REQUIRE(chosen == opt_int{5});
    constexpr auto total = bt::alt(opt_int{}, 9);
    STATIC_REQUIRE(total == 9);
    constexpr auto joined = bt::alt(opt_int{}, std::optional<char>{'c'});
    STATIC_REQUIRE(std::get<char>(*joined) == 'c');
    STATIC_REQUIRE(bt::alt(bt::alternative<opt_int>.zero(),
                           bt::alternative<opt_int>.zero()) == opt_int{});
}

TEST_CASE("alternative: the exception specification is computed from the "
          "contributions, not promised") {
    opt_int a;
    opt_str s;
    STATIC_REQUIRE(noexcept(bt::alt(a, a)));
    STATIC_REQUIRE(noexcept(bt::alt(a, 1)));
    STATIC_REQUIRE(noexcept(bt::alternative<opt_int>.zero()));
    // Copying a string out of an lvalue operand may allocate; moving it out
    // of an rvalue operand does not.
    STATIC_REQUIRE(!noexcept(bt::alt(s, s)));
    STATIC_REQUIRE(noexcept(bt::alt(std::move(s), std::move(s))));
}

// -- expected ---------------------------------------------------------------

TEST_CASE("alternative: expected yields the leftmost operand holding a value, "
          "keeping the declared alternative") {
    exp_int ok{1};
    exp_int other{2};
    exp_int bad{std::unexpect, std::errc::invalid_argument};
    STATIC_REQUIRE(std::is_same_v<decltype(bt::alt(ok, other)), exp_int>);
    REQUIRE(bt::alt(ok, other) == exp_int{1});
    REQUIRE(bt::alt(bad, other) == exp_int{2});
    REQUIRE(bt::alt(bad, bad, other) == exp_int{2});
    REQUIRE(bt::alt(bad, other) == bt::alternative<exp_int>.alt(bad, other));
}

TEST_CASE("alternative: when every operand fails and the alternative has one "
          "slot, the leftmost witness is kept") {
    exp_int first{std::unexpect, std::errc::invalid_argument};
    exp_int second{std::unexpect, std::errc::io_error};
    auto failed = bt::alt(first, second);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(failed.error() == std::errc::invalid_argument);
}

TEST_CASE("alternative: operands of differing grades join lazily, and a total "
          "failure carries every operand's evidence") {
    exp_int c{std::unexpect, std::errc::invalid_argument};
    exp_int_io io{std::unexpect, std::io_errc::stream};
    auto failed = bt::alt(c, io);
    STATIC_REQUIRE(std::is_same_v<decltype(failed), exp_mixed>);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(failed.error().holds<std::errc>());
    REQUIRE(failed.error().holds<std::io_errc>());
    REQUIRE(failed.error().witness_count() == 2);

    // The leftmost success wins, at the joined grade.
    auto chosen = bt::alt(c, exp_int_io{3});
    REQUIRE(chosen.has_value());
    REQUIRE(*chosen == 3);

    // A set operand contributes its members; an identical alternative is
    // carried verbatim, as the cores carry it.
    STATIC_REQUIRE(
        std::is_same_v<decltype(bt::alt(c, std::declval<exp_mixed>())),
                       exp_mixed>);
    STATIC_REQUIRE(std::is_same_v<decltype(bt::alt(std::declval<exp_mixed>(),
                                                   std::declval<exp_mixed>())),
                                  exp_mixed>);
}

TEST_CASE("alternative: differing value types over expected join as they do "
          "over optional") {
    exp_int bad{std::unexpect, std::errc::invalid_argument};
    exp_str s{"two"};
    auto joined = bt::alt(bad, s);
    STATIC_REQUIRE(std::is_same_v<
                   decltype(joined),
                   std::expected<std::variant<int, std::string>, std::errc>>);
    REQUIRE(joined.has_value());
    REQUIRE(std::get<std::string>(*joined) == "two");
}

TEST_CASE("alternative: a bare last operand, or one at the empty grade, makes "
          "the expected disjunction total") {
    exp_int bad{std::unexpect, std::errc::invalid_argument};
    exp_int ok{1};
    auto fallback = bt::alt(bad, 7);
    STATIC_REQUIRE(std::is_same_v<decltype(fallback), int>);
    REQUIRE(fallback == 7);
    REQUIRE(bt::alt(ok, 7) == 1);

    auto uniform = bt::alt(bad, bt::fallible(8));
    STATIC_REQUIRE(std::is_same_v<decltype(uniform), int>);
    REQUIRE(uniform == 8);

    STATIC_REQUIRE(!has_alt<exp_int, int, exp_int>);
}

TEST_CASE("alternative: the void context takes part with the unit, and void "
          "stays void") {
    exp_void done{};
    exp_void failed{std::unexpect, std::errc::io_error};
    auto either = bt::alt(failed, done);
    STATIC_REQUIRE(std::is_same_v<decltype(either), exp_void>);
    REQUIRE(either.has_value());

    auto neither = bt::alt(failed, failed);
    REQUIRE_FALSE(neither.has_value());
    REQUIRE(neither.error() == std::errc::io_error);

    // Beside a valued operand the unit is a value like any other; a bare
    // unit makes the result total.
    STATIC_REQUIRE(
        std::is_same_v<
            decltype(bt::alt(failed, exp_int{1})),
            std::expected<std::variant<std::monostate, int>, std::errc>>);
    STATIC_REQUIRE(std::is_same_v<decltype(bt::alt(failed, std::monostate{})),
                                  std::monostate>);
}

TEST_CASE("alternative: an rvalue expected operand's value is moved through") {
    using exp_ptr = std::expected<std::unique_ptr<int>, std::errc>;
    auto taken = bt::alt(exp_ptr{std::unexpect, std::errc::io_error},
                         exp_ptr{std::make_unique<int>(7)});
    REQUIRE(taken.has_value());
    REQUIRE(**taken == 7);
}

TEST_CASE("alternative: the expected disjunction's exception specification is "
          "computed") {
    exp_int a;
    exp_str s;
    STATIC_REQUIRE(noexcept(bt::alt(a, a)));
    STATIC_REQUIRE(noexcept(bt::alt(a, 1)));
    STATIC_REQUIRE(!noexcept(bt::alt(s, s)));
}
