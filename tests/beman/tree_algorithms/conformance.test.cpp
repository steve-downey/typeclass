// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <beman/transpose/functor.hpp>
#include <beman/tree_algorithms.hpp>

namespace {
namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;

struct LvalueOnly {
    constexpr auto operator()(const int& value) & -> int { return value * 10 + 1; }
    auto           operator()(const int&) && -> int = delete;
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
struct FunctionResult {
    auto operator()(const int&) const -> void (&)();
};
struct UncopyableResult {
    auto operator()(const int&) const -> const std::unique_ptr<int>&;
};

template <class LAYER, class F>
concept object_mappable = requires(const LAYER& layer, F&& f) { bt::functor<LAYER>.fmap(std::forward<F>(f), layer); };
template <class LAYER, class F>
concept operation_mappable = requires(const LAYER& layer, F&& f) { bt::fmap(std::forward<F>(f), layer); };

template <class LAYER>
constexpr auto layer() -> LAYER {
    if constexpr (std::same_as<LAYER, ta::BinaryTreeF<long, int>>) {
        return {99L, ta::make_slot<int>(1), ta::make_slot<int>(2)};
    } else if constexpr (std::same_as<LAYER, ta::FringeTreeF<long, int>>) {
        return ta::FringeBranch<int>{1, 2};
    } else if constexpr (std::same_as<LAYER, ta::RoseF<long, int>>) {
        return {99L, {1, 2}};
    } else if constexpr (std::same_as<LAYER, ta::ExprF<int>>) {
        return ta::Add<int>{ta::make_slot<int>(1), ta::make_slot<int>(2)};
    } else {
        return ta::Succ<int>{ta::make_slot<int>(1)};
    }
}

template <class T>
constexpr auto positions(const ta::BinaryTreeF<long, T>& layer) -> std::vector<T> {
    return {*layer.left, *layer.right};
}
template <class T>
constexpr auto positions(const ta::FringeTreeF<long, T>& layer) -> std::vector<T> {
    const auto& branch = std::get<ta::FringeBranch<T>>(layer);
    return {branch.left, branch.right};
}
template <class T>
constexpr auto positions(const ta::RoseF<long, T>& layer) -> std::vector<T> {
    return layer.children;
}
template <class T>
constexpr auto positions(const ta::ExprF<T>& layer) -> std::vector<T> {
    const auto& add = std::get<ta::Add<T>>(layer);
    return {*add.left, *add.right};
}
template <class T>
constexpr auto positions(const ta::NatF<T>& layer) -> std::vector<T> {
    return {*std::get<ta::Succ<T>>(layer).pred};
}

template <class LAYER>
constexpr auto lvalue_mapping() -> bool {
    const auto result = bt::fmap(LvalueOnly{}, layer<LAYER>());
    if constexpr (std::same_as<LAYER, ta::NatF<int>>)
        return positions(result) == std::vector<int>{11};
    else
        return positions(result) == std::vector<int>{11, 21};
}

static_assert(lvalue_mapping<ta::BinaryTreeF<long, int>>());
static_assert(lvalue_mapping<ta::FringeTreeF<long, int>>());
static_assert(lvalue_mapping<ta::RoseF<long, int>>());
static_assert(lvalue_mapping<ta::ExprF<int>>());
static_assert(lvalue_mapping<ta::NatF<int>>());
static_assert(!operation_mappable<ta::BinaryTreeF<std::unique_ptr<int>, int>, LvalueOnly>);
static_assert(!operation_mappable<ta::FringeTreeF<std::unique_ptr<int>, int>, LvalueOnly>);
static_assert(!operation_mappable<ta::RoseF<std::unique_ptr<int>, int>, LvalueOnly>);
} // namespace

TEMPLATE_TEST_CASE("Layer typeclass declarations match their callable and child type",
                   "",
                   (ta::BinaryTreeF<long, int>),
                   (ta::FringeTreeF<long, int>),
                   (ta::RoseF<long, int>),
                   ta::ExprF<int>,
                   ta::NatF<int>) {
    STATIC_REQUIRE(std::same_as<bt::applicative_value_t<TestType>, int>);
    STATIC_REQUIRE(bt::functor_object<std::remove_cvref_t<decltype(bt::functor<TestType>)>, TestType>);
    STATIC_REQUIRE(object_mappable<TestType, LvalueOnly>);
    STATIC_REQUIRE(operation_mappable<TestType, LvalueOnly>);
    STATIC_REQUIRE(!object_mappable<TestType, RvalueOnly>);
    STATIC_REQUIRE(!operation_mappable<TestType, RvalueOnly>);
    STATIC_REQUIRE(!object_mappable<TestType, NotCallable>);
    STATIC_REQUIRE(!operation_mappable<TestType, NotCallable>);
    STATIC_REQUIRE(!object_mappable<TestType, VoidResult>);
    STATIC_REQUIRE(!operation_mappable<TestType, VoidResult>);
    STATIC_REQUIRE(!object_mappable<TestType, ArrayResult>);
    STATIC_REQUIRE(!operation_mappable<TestType, ArrayResult>);
    STATIC_REQUIRE(!object_mappable<TestType, FunctionResult>);
    STATIC_REQUIRE(!operation_mappable<TestType, FunctionResult>);
    STATIC_REQUIRE(!object_mappable<TestType, UncopyableResult>);
    STATIC_REQUIRE(!operation_mappable<TestType, UncopyableResult>);

    const auto  source = layer<TestType>();
    std::string visits;
    auto        mapper = [&](const int& value) {
        visits += std::to_string(value);
        return std::to_string(value * 10 + 1);
    };
    const auto result = bt::fmap(mapper, source);
    const auto expected =
        std::same_as<TestType, ta::NatF<int>> ? std::vector<std::string>{"11"} : std::vector<std::string>{"11", "21"};
    CHECK(positions(result) == expected);
    CHECK(visits == (std::same_as<TestType, ta::NatF<int>> ? "1" : "12"));

    // Identity and composition agree with mapping the ordered child positions.
    CHECK(positions(bt::fmap(std::identity{}, source)) == positions(source));
    const auto composed = bt::fmap([](int x) { return x - 3; }, bt::fmap(LvalueOnly{}, source));
    CHECK(positions(composed) == positions(bt::fmap([](int x) { return x * 10 - 2; }, source)));
    CHECK(positions(bt::replace(source, 7)) == std::vector<int>(positions(source).size(), 7));
}
