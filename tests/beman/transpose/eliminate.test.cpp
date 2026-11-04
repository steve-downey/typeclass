// tests/beman/transpose/eliminate.test.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/eliminate.hpp>
#include <beman/transpose/eliminate.hpp> // Re-inclusion: verifies include guard

#include <beman/transpose/overloaded.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace bt = beman::transpose;
using bt::eliminate;
using bt::eliminate_indexed;
using bt::overloaded;

namespace {

// A variant-shaped layer in the shape of the tree algorithms' NatF: a layer
// functor's fmap rebuilds the layer at a new element type, and every arm
// produces a different alternative of the result. This is the client that
// motivated the join-of-arms result type
// (docs/transpose/decisions.md#heterogeneous-arm-elimination).
struct Zero {
    friend auto operator==(const Zero &, const Zero &) -> bool = default;
};
template <class A>
struct Succ {
    A pred;
    friend auto operator==(const Succ &, const Succ &) -> bool = default;
};
template <class A>
using NatF = std::variant<Zero, Succ<A>>;

} // namespace

TEST_CASE("eliminate: header is idempotent") { REQUIRE(true); }

TEST_CASE("eliminate: arms that agree yield the common type, as visit does") {
    std::variant<int, std::string> v = 42;
    auto arms = overloaded{
        [](int) { return 1.0; },
        [](const std::string &) { return 2.0; },
    };
    auto joined = eliminate(v, arms);
    STATIC_REQUIRE(std::is_same_v<decltype(joined), double>);
    // The equivalence law pinning eliminate to std::visit where both apply.
    REQUIRE(joined == std::visit(arms, v));
    v = std::string{"x"};
    REQUIRE(eliminate(v, arms) == std::visit(arms, v));
}

TEST_CASE("eliminate: arms that differ yield a variant of their results, in "
          "alternative order, holding the result at its type") {
    std::variant<int, std::string> v = 42;
    auto joined =
        eliminate(v, overloaded{
                         [](int x) { return x * 2; },
                         [](const std::string &s) { return s.size(); },
                     });
    STATIC_REQUIRE(
        std::is_same_v<decltype(joined), std::variant<int, std::size_t>>);
    REQUIRE(std::get<int>(joined) == 84);

    v = std::string{"four"};
    auto other = eliminate(v, overloaded{
                                  [](int x) { return x * 2; },
                                  [](const std::string &s) { return s.size(); },
                              });
    REQUIRE(std::get<std::size_t>(other) == 4);
}

TEST_CASE("eliminate: duplicate results collapse, keeping the first position; "
          "the join is not sorted") {
    std::variant<int, double, std::string> v = 2.5;
    auto joined =
        eliminate(v, overloaded{
                         [](int) { return 1; },
                         [](double) { return 2; },
                         [](const std::string &) { return std::string{"s"}; },
                     });
    // int appears once, first, because the first two arms both produce it.
    STATIC_REQUIRE(
        std::is_same_v<decltype(joined), std::variant<int, std::string>>);
    REQUIRE(std::get<int>(joined) == 2);
}

TEST_CASE("eliminate: rebuilding a layer at a new element type names no "
          "result type on any arm") {
    NatF<int> layer = Succ<int>{3};
    auto to_string = [](int x) { return std::to_string(x); };
    auto mapped =
        eliminate(layer, overloaded{
                             [](const Zero &) { return Zero{}; },
                             [&](const Succ<int> &s) {
                                 return Succ<std::string>{to_string(s.pred)};
                             },
                         });
    // Positional identity is what makes this the layer type and not merely a
    // variant of the same alternatives in another order.
    STATIC_REQUIRE(std::is_same_v<decltype(mapped), NatF<std::string>>);
    REQUIRE(mapped == NatF<std::string>{Succ<std::string>{"3"}});

    NatF<int> zero = Zero{};
    auto still_zero =
        eliminate(zero, overloaded{
                            [](const Zero &) { return Zero{}; },
                            [&](const Succ<int> &s) {
                                return Succ<std::string>{to_string(s.pred)};
                            },
                        });
    REQUIRE(still_zero == NatF<std::string>{Zero{}});
}

TEST_CASE(
    "eliminate: an rvalue variant presents its alternative as an rvalue") {
    std::variant<std::unique_ptr<int>, int> v = std::make_unique<int>(7);
    auto taken =
        eliminate(std::move(v), overloaded{
                                    [](std::unique_ptr<int> &&p) { return *p; },
                                    [](int x) { return x; },
                                });
    STATIC_REQUIRE(std::is_same_v<decltype(taken), int>);
    REQUIRE(taken == 7);
}

TEST_CASE("eliminate: a generic arm is a complete overload set on its own") {
    std::variant<int, std::string> v = 42;
    auto size = eliminate(v, [](const auto &x) { return sizeof(x); });
    STATIC_REQUIRE(std::is_same_v<decltype(size), std::size_t>);
    REQUIRE(size == sizeof(int));
}

TEST_CASE("eliminate_indexed: the arm receives the injection tag") {
    // Two alternatives of one type: only the index tells them apart.
    std::variant<int, int> v{std::in_place_index<1>, 5};
    auto which = eliminate_indexed(
        v, overloaded{
               [](std::in_place_index_t<0>, int x) { return x; },
               [](std::in_place_index_t<1>, int x) { return -x; },
           });
    STATIC_REQUIRE(std::is_same_v<decltype(which), int>);
    REQUIRE(which == -5);

    // Heterogeneous arms join exactly as for eliminate.
    std::variant<int, std::string> w = std::string{"abc"};
    auto joined = eliminate_indexed(
        w, overloaded{
               [](std::in_place_index_t<0>, int x) { return x; },
               [](std::in_place_index_t<1>, const std::string &s) {
                   return s.size();
               },
           });
    STATIC_REQUIRE(
        std::is_same_v<decltype(joined), std::variant<int, std::size_t>>);
    REQUIRE(std::get<std::size_t>(joined) == 3);

    // A generic arm can take the index as a template argument.
    auto index =
        eliminate_indexed(w, []<std::size_t I>(std::in_place_index_t<I>,
                                               const auto &) { return I; });
    REQUIRE(index == 1);
}

TEST_CASE("eliminate: the operations are constexpr") {
    constexpr auto common =
        eliminate(std::variant<int, char>{'a'}, overloaded{
                                                    [](int) { return 1; },
                                                    [](char) { return 2; },
                                                });
    STATIC_REQUIRE(common == 2);

    constexpr auto joined =
        eliminate(std::variant<int, char>{'a'}, overloaded{
                                                    [](int x) { return x; },
                                                    [](char c) { return c; },
                                                });
    STATIC_REQUIRE(std::get<char>(joined) == 'a');

    constexpr auto indexed = eliminate_indexed(
        std::variant<int, int>{std::in_place_index<0>, 9},
        overloaded{
            [](std::in_place_index_t<0>, int x) { return x; },
            [](std::in_place_index_t<1>, int x) { return -x; },
        });
    STATIC_REQUIRE(indexed == 9);
}

// A missing arm is a compile error naming the alternative. There is no
// compile-fail test mechanism in this repository, so the negative control is
// recorded here as it was hand-verified with GCC 15:
//
//     eliminate(std::variant<int, std::string>{}, overloaded{[](int) { return
//     1; }});
//
// produces, in order,
//
//     In instantiation of 'struct beman::transpose::detail::arm_result<
//         overloaded<lambda(int)>, const std::__cxx11::basic_string<char>&>'
//     error: static assertion failed: eliminate: no arm accepts this
//         alternative -- add a case
//
// and then overloaded's own catch-all diagnostic. The alternative is named in
// the instantiation context, which is the acceptance criterion of stage
// set-elimination (docs/transpose/transpose-surface-plan.md#set-elimination).
TEST_CASE("eliminate: a missing arm is a named compile error (hand-verified)") {
    SUCCEED("negative control recorded in the comment above");
}
