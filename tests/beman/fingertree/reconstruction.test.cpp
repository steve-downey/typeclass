// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree.hpp>
#include <beman/fingertree/tree/finger_tree_rebind.hpp> // Re-inclusion check
#include <beman/fingertree/tree/finger_tree_functor.hpp>
#include <beman/fingertree/tree/finger_tree_monoid.hpp>
#include <beman/transpose/string.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include "../collection_test_support.hpp"

#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace reconstruction_test {
namespace ft = beman::fingertree::tree;
struct ModSum {
    using value_type    = std::size_t;
    std::size_t modulus = 97;
    auto        identity() const -> std::size_t { return 0; }
    auto        combine(std::size_t a, std::size_t b) const -> std::size_t { return (a + b) % modulus; }
};
template <int KIND = 0>
struct Weight {
    auto operator()(const int& value) const -> std::size_t { return static_cast<std::size_t>(value); }
};
struct StringWeight {
    auto operator()(const std::string& value) const -> std::size_t { return value.size(); }
};
template <int KIND = 0>
using Weighted = ft::FingerTree<int, ModSum, Weight<KIND>>;
using Rebound  = ft::FingerTree<std::string, ModSum, StringWeight>;
struct DeclaredBuilder {
    ModSum monoid;
    // Deliberately requires a named, non-const lvalue builder.
    auto operator()(std::vector<std::string>&& values) & -> Rebound {
        return Rebound::from_sequence(std::move(values), {}, monoid);
    }
};
struct WrongBuilder {
    auto operator()(std::vector<std::string>&&) -> int { return 0; }
};
struct RvalueBuilder {
    auto operator()(std::vector<std::string>&&) && -> Rebound;
};

struct Join {
    using value_type      = std::string;
    std::string separator = ":";
    auto        identity() const -> std::string { return {}; }
    auto        combine(const std::string& a, const std::string& b) const -> std::string {
        if (a.empty())
            return b;
        if (b.empty())
            return a;
        return a + separator + b;
    }
};
struct TextMeasure {
    auto operator()(const std::string& value) const -> std::string { return value; }
};
} // namespace reconstruction_test

namespace beman::fingertree {
template <>
struct finger_tree_rebind<reconstruction_test::Weighted<1>, std::string> {
    using type = reconstruction_test::Rebound;
    static auto make_builder(const reconstruction_test::Weighted<1>& source) {
        return reconstruction_test::DeclaredBuilder{source.get_monoid()};
    }
};
template <>
struct finger_tree_rebind<reconstruction_test::Weighted<2>, std::string> {
    using type = reconstruction_test::Rebound; // Missing builder.
};
template <>
struct finger_tree_rebind<reconstruction_test::Weighted<3>, std::string> {
    using type = reconstruction_test::Rebound;
    static auto make_builder(const reconstruction_test::Weighted<3>&) { return reconstruction_test::WrongBuilder{}; }
};
template <>
struct finger_tree_rebind<reconstruction_test::Weighted<4>, std::string> {
    using type = reconstruction_test::Rebound;
    static auto make_builder(const reconstruction_test::Weighted<4>&) { return reconstruction_test::RvalueBuilder{}; }
};
} // namespace beman::fingertree

namespace {
namespace bt = beman::transpose;
namespace bf = beman::fingertree;
namespace ft = beman::fingertree::tree;
namespace rt = reconstruction_test;
namespace ct = collection_test;

struct AsString {
    auto operator()(const int& value) & -> std::string { return std::to_string(value * 10 + 1); }
};
struct StringEffect {
    auto operator()(const int& value) & -> std::optional<std::string> { return std::to_string(value * 10 + 1); }
};
struct VoidResult {
    auto operator()(const int&) const -> void {}
};
struct ArrayResult {
    auto operator()(const int&) const -> int (&)[2];
};
struct RvalueOnly {
    auto operator()(const int&) && -> int { return 0; }
};
struct NotCallable {};
struct Unordered {};
struct AsUnordered {
    auto operator()(const int&) const -> Unordered { return {}; }
};

template <class TREE, class F>
concept mappable = requires(const TREE& tree, F&& fn) { bt::fmap(std::forward<F>(fn), tree); };
template <class TREE, class F>
concept object_mappable = requires(const TREE& tree, F&& fn) { bt::functor<TREE>.fmap(std::forward<F>(fn), tree); };
template <class TREE, class F>
concept traversable = requires(const TREE& tree, F&& fn) { bt::traverse(std::forward<F>(fn), tree); };
template <class TREE, class F>
concept object_traversable = requires(const TREE& tree, F&& fn) {
    bt::traversable<TREE>.traverse(bt::applicative<std::optional<std::string>>, std::forward<F>(fn), tree);
};

template <class TREE>
auto source() -> TREE {
    if constexpr (std::same_as<TREE, ft::FingerTreeIntervalIndex<int>>)
        return TREE::from_intervals({{7, 20, 1}, {9, 30, 2}, {12, 40, 3}});
    else if constexpr (std::same_as<TREE, ft::FingerTreePriorityQueue<int>>)
        return TREE::from_values({1, 2, 3});
    else
        return TREE::from_sequence({1, 2, 3});
}
template <class TREE>
auto values(const TREE& tree) {
    std::vector<typename TREE::value_type> result;
    tree.for_each([&](const auto& value) { result.push_back(value); });
    return result;
}

struct Tracked {
    inline static int copies = 0;
    int               value;
    explicit Tracked(int n) : value(n) {}
    Tracked(const Tracked& other) : value(other.value) { ++copies; }
    Tracked(Tracked&& other) noexcept : value(std::exchange(other.value, -1)) {}
};
struct PointerWeight {
    auto operator()(const std::unique_ptr<int>& value) const -> std::size_t {
        return static_cast<std::size_t>(*value);
    }
};
} // namespace

static_assert(bt::functor_object<std::remove_cvref_t<decltype(bt::functor<ft::FingerTree<std::string>>)>,
                                 ft::FingerTree<std::string>>);
static_assert(bt::functor_object<std::remove_cvref_t<decltype(bt::functor<ft::FingerTreeRandomAccess<std::string>>)>,
                                 ft::FingerTreeRandomAccess<std::string>>);
static_assert(bt::functor_object<std::remove_cvref_t<decltype(bt::functor<ft::FingerTreePriorityQueue<std::string>>)>,
                                 ft::FingerTreePriorityQueue<std::string>>);
static_assert(bt::functor_object<std::remove_cvref_t<decltype(bt::functor<ft::FingerTreeIntervalIndex<std::string>>)>,
                                 ft::FingerTreeIntervalIndex<std::string>>);

TEMPLATE_TEST_CASE("Configured payload Functors obey identity, composition and pure traversal",
                   "",
                   ft::FingerTree<int>,
                   ft::FingerTreeRandomAccess<int>,
                   ft::FingerTreePriorityQueue<int>,
                   ft::FingerTreeIntervalIndex<int>) {
    using Map = std::remove_cvref_t<decltype(bt::functor<TestType>)>;
    STATIC_REQUIRE(bt::functor_object<Map, TestType>);
    STATIC_REQUIRE(object_mappable<TestType, AsString>);
    STATIC_REQUIRE(mappable<TestType, AsString>);
    STATIC_REQUIRE(!object_mappable<TestType, NotCallable>);
    STATIC_REQUIRE(!mappable<TestType, NotCallable>);
    STATIC_REQUIRE(!object_mappable<TestType, VoidResult>);
    STATIC_REQUIRE(!mappable<TestType, VoidResult>);
    STATIC_REQUIRE(!object_mappable<TestType, ArrayResult>);
    STATIC_REQUIRE(!mappable<TestType, ArrayResult>);
    STATIC_REQUIRE(!object_mappable<TestType, RvalueOnly>);
    STATIC_REQUIRE(!mappable<TestType, RvalueOnly>);
    const auto tree     = source<TestType>();
    const auto identity = bt::fmap(std::identity{}, tree);
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(identity)>, TestType>);
    CHECK(values(identity) == values(tree));
    auto first  = [](const int& n) { return n * 10 + 1; };
    auto second = [](const int& n) { return n - 3; };
    CHECK(values(bt::fmap(second, bt::fmap(first, tree))) ==
          values(bt::fmap([&](const int& n) { return second(first(n)); }, tree)));
    auto effect = bt::traverse([&](const int& n) { return std::optional{first(n)}; }, tree);
    REQUIRE(effect.has_value());
    CHECK(values(*effect) == values(bt::fmap(first, tree)));
    CHECK(values(bt::replace(tree, 7)) == (std::vector<int>{7, 7, 7}));
    auto units = bt::traverse([](const int&) { return std::optional{std::monostate{}}; }, tree);
    REQUIRE(units.has_value());
    CHECK(bt::length(*units) == 3);
}

TEMPLATE_TEST_CASE("Unsupported reconstruction requests reject softly",
                   "",
                   rt::Weighted<>,
                   rt::Weighted<2>,
                   rt::Weighted<3>,
                   rt::Weighted<4>) {
    STATIC_REQUIRE(!object_mappable<TestType, AsString>);
    STATIC_REQUIRE(!mappable<TestType, AsString>);
    STATIC_REQUIRE(!object_traversable<TestType, StringEffect>);
    STATIC_REQUIRE(!traversable<TestType, StringEffect>);
}

TEST_CASE("Weighted reconstruction retains type and monoid while recomputing measures") {
    const auto tree   = rt::Weighted<>::from_sequence({2, 3, 7}, {}, rt::ModSum{101});
    const auto result = bt::traverse([](const int& n) { return std::optional{n}; }, tree);
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(*result)>, rt::Weighted<>>);
    REQUIRE(result.has_value());
    CHECK(result->measure() == 12);
    CHECK(result->get_monoid().modulus == 101);
    const auto mapped = bt::fmap([](const int& n) { return n * 10 + 1; }, tree);
    CHECK(mapped.measure() == 22);
    CHECK(mapped.get_monoid().modulus == 101);
    CHECK(tree.measure() == 12);
}

TEST_CASE("A declared rebind supplies an owned lvalue-callable builder") {
    const auto tree   = rt::Weighted<1>::from_sequence({2, 3, 7}, {}, rt::ModSum{101});
    const auto mapped = bt::fmap(AsString{}, tree);
    const auto effect = bt::traverse(StringEffect{}, tree);
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(mapped)>, rt::Rebound>);
    REQUIRE(effect.has_value());
    CHECK(mapped.flatten() == (std::vector<std::string>{"21", "31", "71"}));
    CHECK(effect->flatten() == mapped.flatten());
    CHECK(mapped.measure() == 6);
    CHECK(mapped.get_monoid().modulus == 101);
    CHECK(effect->get_monoid().modulus == 101);
}

TEST_CASE("Count-policy rebinding preserves PMR allocation and monoid state") {
    using Alloc  = std::pmr::polymorphic_allocator<std::byte>;
    using Tree   = ft::FingerTree<int, rt::ModSum, ft::UnitMeasure<int, std::size_t>, Alloc>;
    using Result = ft::FingerTree<std::string, rt::ModSum, ft::UnitMeasure<std::string, std::size_t>, Alloc>;
    std::pmr::monotonic_buffer_resource resource;
    for (bool empty : {false, true}) {
        const auto tree =
            Tree::from_sequence(empty ? std::vector<int>{} : std::vector<int>{1, 2, 3}, &resource, rt::ModSum{5});
        const auto mapped = bt::fmap(AsString{}, tree);
        auto       effect = bt::traverse(StringEffect{}, Tree{tree});
        STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(mapped)>, Result>);
        REQUIRE(effect.has_value());
        CHECK(effect->flatten() == mapped.flatten());
        CHECK(mapped.get_allocator().resource() == &resource);
        CHECK(effect->get_allocator().resource() == &resource);
        CHECK(mapped.get_monoid().modulus == 5);
        CHECK(effect->get_monoid().modulus == 5);
        CHECK(mapped.measure() == (empty ? 0 : 3));
    }
}

TEST_CASE("Fresh leaves are measured before moving their move-only payloads") {
    using Tree = ft::FingerTree<std::unique_ptr<int>, bt::sum_monoid<std::size_t>, PointerWeight>;
    std::vector<std::unique_ptr<int>> fresh;
    for (int value : {2, 3, 7})
        fresh.push_back(std::make_unique<int>(value));
    const auto tree = Tree::from_sequence(std::move(fresh));
    CHECK(tree.measure() == 12);
    std::string visits;
    tree.for_each([&](const auto& value) { visits += std::to_string(*value); });
    CHECK(visits == "237");
}

TEST_CASE("Persistent traversal supports move-only sources and fresh results") {
    const auto source = ft::FingerTree<int>::from_sequence({1, 2, 3});
    auto fresh = bt::traverse([](const int& n) { return std::optional{std::make_unique<int>(n * 10 + 1)}; }, source);
    REQUIRE(fresh.has_value());
    auto read   = [](const std::unique_ptr<int>& value) { return std::optional{*value - 3}; };
    auto result = bt::traverse(read, std::move(*fresh));
    REQUIRE(result.has_value());
    CHECK(result->flatten() == (std::vector<int>{8, 18, 28}));
    CHECK(*fresh->head_ref() == 11);
    auto mapped = bt::fmap([](const int& n) { return std::make_unique<int>(n); }, source);
    CHECK(*mapped.head_ref() == 1);
}

TEST_CASE("Fresh construction and reference-reading mapping make no payload copies") {
    std::vector<Tracked> fresh;
    for (int n : {1, 2, 3})
        fresh.emplace_back(n);
    Tracked::copies   = 0;
    const auto tree   = ft::FingerTree<Tracked>::from_sequence(std::move(fresh));
    auto       mapped = bt::fmap([](const Tracked& n) { return Tracked{n.value * 10 + 1}; }, tree);
    auto       result = bt::traverse([](const Tracked& n) { return std::optional{Tracked{n.value * 10 + 1}}; }, tree);
    REQUIRE(result.has_value());
    CHECK(Tracked::copies == 0);
    CHECK(mapped.head_ref().value == 11);
    CHECK(result->last_ref().value == 31);
    CHECK(tree.head_ref().value == 1);
}

TEST_CASE("Capability wrappers preserve declared PMR backing families and endpoints") {
    std::pmr::monotonic_buffer_resource resource;
    using RA = ft::FingerTreeRandomAccess<int, ft::pmr::FingerTree<int>>;
    const RA   random{ft::pmr::FingerTree<int>::from_sequence({1, 2, 3}, &resource)};
    const auto mapped = bt::fmap(AsString{}, random);
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(mapped)>,
                                ft::FingerTreeRandomAccess<std::string, ft::pmr::FingerTree<std::string>>>);
    CHECK(mapped.backing_tree().get_allocator().resource() == &resource);
    CHECK(mapped.at(1) == "21");

    using PQTree = ft::pmr::FingerTree<int, ft::PriorityMonoid<int>, ft::PriorityMeasure<int>>;
    const ft::FingerTreePriorityQueue<int, PQTree> queue{PQTree::from_sequence({1, 2, 3}, &resource)};
    const auto                                     queue_result = bt::traverse(StringEffect{}, queue);
    REQUIRE(queue_result.has_value());
    CHECK(queue_result->backing_tree().get_allocator().resource() == &resource);
    CHECK(queue_result->min() == "11");
    CHECK(queue_result->max() == "31");
    CHECK(queue_result->size() == 3);
    STATIC_REQUIRE(!mappable<decltype(queue), AsUnordered>);

    using IndexTree = ft::pmr::FingerTree<ft::Interval<int>, ft::IntervalMonoid, ft::IntervalMeasure<int>>;
    const ft::FingerTreeIntervalIndex<int, IndexTree> index{
        IndexTree::from_sequence({{7, 20, 1}, {9, 30, 2}, {12, 40, 3}}, &resource)};
    auto result = bt::traverse(StringEffect{}, index);
    REQUIRE(result.has_value());
    CHECK(result->backing_tree().get_allocator().resource() == &resource);
    CHECK(result->query_point(13) == (std::vector<std::string>{"11", "21", "31"}));
    const auto entries = result->entries();
    CHECK(entries[0].d_start == 7);
    CHECK(entries[1].d_end == 30);
    CHECK(entries[2].d_end == 40);
}

TEST_CASE("Interval reconstruction reads move-only payloads and moves fresh results") {
    using Tree = ft::
        FingerTree<ft::Interval<std::unique_ptr<int>>, ft::IntervalMonoid, ft::IntervalMeasure<std::unique_ptr<int>>>;
    const ft::FingerTreeIntervalIndex<std::unique_ptr<int>, Tree> index{
        Tree{}.snoc({7, 20, std::make_unique<int>(1)}).snoc({9, 30, std::make_unique<int>(2)})};
    auto result = bt::traverse([](const auto& n) { return std::optional{std::make_unique<int>(*n * 10 + 1)}; }, index);
    REQUIRE(result.has_value());
    CHECK(*result->backing_tree().head_ref().d_payload == 11);
    CHECK(result->backing_tree().head_ref().d_start == 7);
    CHECK(result->backing_tree().last_ref().d_end == 30);
    CHECK(*index.backing_tree().head_ref().d_payload == 1);
    CHECK(bt::fold_map([](const auto& n) { return std::to_string(*n); }, index, bt::monoid<std::string>) == "12");
}

TEMPLATE_TEST_CASE("Deferred reconstruction owns its PMR and monoid snapshots",
                   "",
                   ct::DeferredMap<false>,
                   ct::DeferredMap<true>) {
    using Tree = ft::pmr::FingerTree<int, rt::ModSum, rt::Weight<>>;
    std::pmr::monotonic_buffer_resource resource;
    std::string                         log;
    auto                                result = [&] {
        const auto tree = Tree::from_sequence({2, 3, 7}, &resource, rt::ModSum{101});
        return bt::traversable<Tree>.traverse(
            TestType{},
            [&](const int& n) {
                return ct::Deferred<int>{[n, &log] {
                    log += std::to_string(n);
                    return n * 10 + 1;
                }};
            },
            tree);
    }();
    CHECK(log.empty());
    const auto actual = result.run();
    CHECK(log == "237");
    CHECK(actual.measure() == 22);
    CHECK(actual.get_monoid().modulus == 101);
    CHECK(actual.get_allocator().resource() == &resource);
}

TEST_CASE("Configured transpose agrees with identity traversal") {
    using Tree            = ft::FingerTree<std::optional<int>>;
    const auto tree       = Tree::from_sequence({std::optional{1}, std::optional{2}, std::optional{3}});
    auto       transposed = bt::transpose(tree);
    auto       traversed  = bt::traverse(std::identity{}, tree);
    REQUIRE(transposed.has_value());
    REQUIRE(traversed.has_value());
    CHECK(transposed->flatten() == traversed->flatten());
}

TEST_CASE("Configured assignment retains the incoming monoid and destination PMR resource") {
    using Tree = ft::pmr::FingerTree<int, rt::ModSum, rt::Weight<>>;
    std::pmr::monotonic_buffer_resource first;
    std::pmr::monotonic_buffer_resource second;
    const auto                          source = Tree::from_sequence({2, 3, 7}, &first, rt::ModSum{5});
    for (auto* target : {&first, &second}) {
        for (bool move : {false, true}) {
            auto result = Tree::from_sequence({40}, target, rt::ModSum{97});
            if (move)
                result = bt::fmap(std::identity{}, source);
            else
                result = source;
            CHECK(result.get_allocator().resource() == target);
            CHECK(result.get_monoid().modulus == 5);
            CHECK(result.measure() == 2);
            CHECK(result.snoc(1).measure() == 3);
        }
    }
}

namespace {
struct AlternateCount {
    template <class T>
    auto operator()(const T&) const -> std::size_t {
        return 1;
    }
};
struct AlternatePriority : ft::PriorityMeasure<int> {};
struct AlternateInterval : ft::IntervalMeasure<int> {};
using CustomRA = ft::FingerTreeRandomAccess<int, ft::FingerTree<int, bt::sum_monoid<std::size_t>, AlternateCount>>;
using CustomPQ = ft::FingerTreePriorityQueue<int, ft::FingerTree<int, ft::PriorityMonoid<int>, AlternatePriority>>;
using CustomIndex =
    ft::FingerTreeIntervalIndex<int, ft::FingerTree<ft::Interval<int>, ft::IntervalMonoid, AlternateInterval>>;
} // namespace

TEMPLATE_TEST_CASE(
    "Custom wrapper backing families require declared type-changing rebinds", "", CustomRA, CustomPQ, CustomIndex) {
    STATIC_REQUIRE(!mappable<TestType, AsString>);
    STATIC_REQUIRE(!object_mappable<TestType, AsString>);
    STATIC_REQUIRE(!traversable<TestType, StringEffect>);
    STATIC_REQUIRE(!object_traversable<TestType, StringEffect>);
}

TEST_CASE("Random access moves fresh results while retaining its PMR family") {
    std::pmr::monotonic_buffer_resource resource;
    using Source = ft::FingerTreeRandomAccess<int, ft::pmr::FingerTree<int>>;
    const Source source{ft::pmr::FingerTree<int>::from_sequence({1, 2, 3}, &resource)};
    auto result = bt::traverse([](const int& n) { return std::optional{std::make_unique<int>(n * 10 + 1)}; }, source);
    REQUIRE(result.has_value());
    CHECK(result->backing_tree().get_allocator().resource() == &resource);
    CHECK(*result->backing_tree().head_ref() == 11);
    auto read = bt::traverse([](const auto& n) { return std::optional{*n}; }, std::move(*result));
    REQUIRE(read.has_value());
    CHECK(read->to_vector() == (std::vector<int>{11, 21, 31}));
    CHECK(*result->backing_tree().head_ref() == 11);
}

TEMPLATE_TEST_CASE("Deferred interval builders own PMR configuration and endpoints",
                   "",
                   ct::DeferredMap<false>,
                   ct::DeferredMap<true>) {
    std::pmr::monotonic_buffer_resource resource;
    using Tree  = ft::pmr::FingerTree<ft::Interval<int>, ft::IntervalMonoid, ft::IntervalMeasure<int>>;
    using Index = ft::FingerTreeIntervalIndex<int, Tree>;
    auto result = [&] {
        const Index source{Tree::from_sequence({{7, 20, 1}, {9, 30, 2}, {12, 40, 3}}, &resource)};
        return bt::traversable<Index>.traverse(
            TestType{}, [](const int& n) { return ct::Deferred<int>{[n] { return n * 10 + 1; }}; }, source);
    }();
    const auto actual = result.run();
    CHECK(actual.backing_tree().get_allocator().resource() == &resource);
    CHECK(actual.backing_tree().measure() == 40);
    CHECK(actual.query_point(13) == (std::vector<int>{11, 21, 31}));
    CHECK(actual.entries()[0].d_start == 7);
    CHECK(actual.entries()[2].d_end == 40);
}
