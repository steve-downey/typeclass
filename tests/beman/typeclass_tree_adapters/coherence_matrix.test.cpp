// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// The cross-component coherence matrix.
//
// One row per structure that registers typeclass instances, across all three
// libraries: std::vector as the reference instance, the finger tree and its
// payload-polymorphic wrappers, and the tree algorithms' payload trees. One
// column per property the libraries promise in common. Every cell is the
// same code, instantiated for the row: a structure that stops honouring a
// contract the others keep fails here, whichever library it lives in.
//
// The columns:
//   registered objects   every instance is found and satisfies the deep
//                        object concept of its typeclass
//   order                folds and effects follow the documented order
//   result types         optional, expected, graded expected, the unit,
//                        a lanewise context, and a collecting context whose
//                        composition changes carrier type
//   native collection    a policy's collect is used once when it has one
//   laws                 identity and composition of fmap, traverse with a
//                        pure lift, transpose as identity traversal
//   structure            the structural fold agrees with the Foldable
//
// docs/typeclass_tree_adapters/README.md carries the matrix as a table.

#include <beman/typeclass_tree_adapters/finger_tree.hpp>

#include <beman/fingertree.hpp>
#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include "../collection_test_support.hpp"

#include <concepts>
#include <cstddef>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// Error alternatives of a graded expected. Named types with external linkage
// and a stable spelling, as the interim type ordering requires
// (beman/transpose/error_set.hpp).
namespace beman_coherence_matrix_errors {

struct too_small {
    int         value{};
    friend auto operator==(const too_small&, const too_small&) -> bool = default;
};
struct too_large {
    int         value{};
    friend auto operator==(const too_large&, const too_large&) -> bool = default;
};

} // namespace beman_coherence_matrix_errors

namespace {

namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
namespace ct = collection_test;

using beman_coherence_matrix_errors::too_large;
using beman_coherence_matrix_errors::too_small;

const std::vector<int>         k_payloads{1, 2, 3};
const std::vector<std::string> k_spelled{"1", "2", "3"};

inline constexpr auto spell  = [](const int& value) { return std::to_string(value); };
inline constexpr auto concat = [](const std::string& a, const std::string& b) { return a + b; };

// ---------------------------------------------------------------------
// The rows.
// ---------------------------------------------------------------------

// A row supplies an instance holding the payloads 1, 2, 3 in its documented
// order, and the text its structural fold gives -- nullopt where the
// structure has no structural route.
template <class S>
struct row;

template <>
struct row<std::vector<int>> {
    static auto make() -> std::vector<int> { return k_payloads; }
    static auto structure(const std::vector<int>&) -> std::optional<std::string> { return std::nullopt; }
};

template <>
struct row<ft::FingerTree<int>> {
    static auto make() -> ft::FingerTree<int> { return ft::FingerTree<int>::from_sequence(k_payloads); }
    // Registered by the optional adapter.
    static auto structure(const ft::FingerTree<int>& tree) -> std::optional<std::string> {
        return ta::fold_map(spell, tree, bt::monoid<std::string>);
    }
};

template <>
struct row<ft::FingerTreeRandomAccess<int>> {
    static auto make() { return ft::FingerTreeRandomAccess<int>::from_sequence(k_payloads); }
    // A wrapper's structure is its backing tree's, named explicitly.
    static auto structure(const ft::FingerTreeRandomAccess<int>& sequence) -> std::optional<std::string> {
        return ta::fold_map<std::string>(spell, concat, std::string{}, sequence.backing_tree());
    }
};

template <>
struct row<ft::FingerTreePriorityQueue<int>> {
    static auto make() { return ft::FingerTreePriorityQueue<int>::from_values(k_payloads); }
    static auto structure(const ft::FingerTreePriorityQueue<int>& queue) -> std::optional<std::string> {
        return ta::fold_map<std::string>(spell, concat, std::string{}, queue.backing_tree());
    }
};

template <>
struct row<ft::FingerTreeIntervalIndex<int>> {
    static auto make() {
        return ft::FingerTreeIntervalIndex<int>::from_intervals({{7, 20, 1}, {9, 30, 2}, {12, 40, 3}});
    }
    // The backing payload is the whole interval; its value is one member.
    static auto structure(const ft::FingerTreeIntervalIndex<int>& index) -> std::optional<std::string> {
        return ta::fold_map<std::string>(
            [](const ft::Interval<int>& entry) { return std::to_string(entry.d_payload); },
            concat,
            std::string{},
            index.backing_tree());
    }
};

template <>
struct row<ta::BinaryTree<int>> {
    // In-order.
    static auto make() {
        using Tree = ta::BinaryTree<int>;
        return Tree::node(2, Tree::leaf(1), Tree::leaf(3));
    }
    static auto structure(const ta::BinaryTree<int>& tree) -> std::optional<std::string> {
        return ta::fold_map(spell, tree, bt::monoid<std::string>);
    }
};

template <>
struct row<ta::BinaryTreeFix<int>> {
    static auto make() { return ta::to_fix(row<ta::BinaryTree<int>>::make()); }
    static auto structure(const ta::BinaryTreeFix<int>& tree) -> std::optional<std::string> {
        return ta::fold_map(spell, tree, bt::monoid<std::string>);
    }
};

template <>
struct row<ta::FringeTree<int>> {
    static auto make() { return ta::FringeTree<int>::from_sequence(k_payloads); }
    static auto structure(const ta::FringeTree<int>& tree) -> std::optional<std::string> {
        return ta::fold_map(spell, tree, bt::monoid<std::string>);
    }
};

template <>
struct row<ta::RoseTreeFix<int>> {
    // Pre-order.
    static auto make() { return ta::rose<int>(1, {ta::rose<int>(2), ta::rose<int>(3)}); }
    static auto structure(const ta::RoseTreeFix<int>& tree) -> std::optional<std::string> {
        return ta::fold_map(spell, tree, bt::monoid<std::string>);
    }
};

template <class S>
using FunctorOf = std::remove_cvref_t<decltype(bt::functor<S>)>;
template <class S>
using FoldableOf = std::remove_cvref_t<decltype(bt::foldable<S>)>;
template <class S>
using TraversableOf = std::remove_cvref_t<decltype(bt::traversable<S>)>;

// Whether the monoid-object bridge of the tree algorithms accepts S itself.
template <class S>
concept bridged = requires(const S& s) { ta::fold_map(spell, s, bt::monoid<std::string>); };

// The payloads of any structure in the matrix, in its documented order.
template <class S>
auto payloads(const S& structure) {
    return bt::to_vector(structure);
}

// The start and end of every interval, when the row has intervals.
template <class S>
auto endpoints(const S&) -> std::vector<std::size_t> {
    return {};
}
template <class T>
auto endpoints(const ft::FingerTreeIntervalIndex<T>& index) -> std::vector<std::size_t> {
    std::vector<std::size_t> out;
    for (const auto& entry : index.entries()) {
        out.push_back(entry.d_start);
        out.push_back(entry.d_end);
    }
    return out;
}

using Plain  = std::expected<long, std::errc>;
using Graded = std::expected<int, bt::error_set<too_small, too_large>>;
using Unit   = std::expected<void, std::errc>;

// A policy's writer effect for one payload: the payload held, its text
// logged. The carrier is labelled when the policy widens.
template <class POLICY>
inline constexpr auto log_effect = [](const int& value) {
    return ct::Writer<int, std::same_as<POLICY, ct::WriterMap<true, true>>>{value, std::to_string(value)};
};

} // namespace

#define MATRIX_ROWS                                                                                           \
    std::vector<int>, ft::FingerTree<int>, ft::FingerTreeRandomAccess<int>, ft::FingerTreePriorityQueue<int>, \
        ft::FingerTreeIntervalIndex<int>, ta::BinaryTree<int>, ta::BinaryTreeFix<int>, ta::FringeTree<int>,   \
        ta::RoseTreeFix<int>

TEMPLATE_TEST_CASE("CoherenceMatrix - RegisteredObjectsSatisfyTheirDeepConcepts", "[coherence::matrix]", MATRIX_ROWS) {
    STATIC_REQUIRE(bt::has_instance_v<decltype(bt::functor<TestType>)>);
    STATIC_REQUIRE(bt::has_instance_v<decltype(bt::foldable<TestType>)>);
    STATIC_REQUIRE(bt::has_instance_v<decltype(bt::traversable<TestType>)>);
    STATIC_REQUIRE(std::same_as<bt::applicative_value_t<TestType>, int>);
    STATIC_REQUIRE(bt::functor_object<FunctorOf<TestType>, TestType>);
    STATIC_REQUIRE(bt::foldable_object<FoldableOf<TestType>, TestType>);
    STATIC_REQUIRE(bt::traversable_object<TraversableOf<TestType>, TestType>);
}

TEMPLATE_TEST_CASE("CoherenceMatrix - FoldsFollowTheDocumentedOrder", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject = row<TestType>::make();
    CHECK(payloads(subject) == k_payloads);
    CHECK(bt::length(subject) == 3);
    CHECK_FALSE(bt::empty(subject));
    // A noncommutative monoid: the order is in the result.
    CHECK(bt::fold_map(spell, subject, bt::monoid<std::string>) == "123");
    CHECK(bt::fold_left(subject, std::string{}, [](std::string acc, const int& value) {
              return std::move(acc) + std::to_string(value);
          }) == "123");
    CHECK(bt::fold_right(subject, std::string{}, [](const int& value, std::string acc) {
              return std::move(acc) + std::to_string(value);
          }) == "321");
}

TEMPLATE_TEST_CASE("CoherenceMatrix - FunctorLawsHold", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject = row<TestType>::make();

    const auto same = bt::fmap([](const int& value) { return value; }, subject);
    STATIC_REQUIRE(std::same_as<decltype(same), const TestType>);
    CHECK(payloads(same) == k_payloads);
    CHECK(endpoints(same) == endpoints(subject));

    const auto tenfold  = [](const int& value) { return value * 10; };
    const auto composed = bt::fmap([&](const int& value) { return spell(tenfold(value)); }, subject);
    const auto stepwise = bt::fmap(spell, bt::fmap(tenfold, subject));
    STATIC_REQUIRE(std::same_as<decltype(composed), decltype(stepwise)>);
    CHECK(payloads(composed) == std::vector<std::string>{"10", "20", "30"});
    CHECK(payloads(stepwise) == payloads(composed));
    CHECK(endpoints(composed) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - TraverseOptional", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject = row<TestType>::make();

    const auto all = bt::traverse([](const int& value) { return std::optional{std::to_string(value)}; }, subject);
    REQUIRE(all.has_value());
    CHECK(payloads(*all) == k_spelled);
    CHECK(endpoints(*all) == endpoints(subject));

    const auto none = bt::traverse(
        [](const int& value) -> std::optional<std::string> {
            if (value == 2)
                return std::nullopt;
            return std::to_string(value);
        },
        subject);
    CHECK_FALSE(none.has_value());

    // Traversing with a pure lift is mapping.
    const auto lifted = bt::traverse([](const int& value) { return std::optional{value * 10}; }, subject);
    const auto mapped = bt::fmap([](const int& value) { return value * 10; }, subject);
    STATIC_REQUIRE(std::same_as<decltype(lifted), const std::optional<std::remove_const_t<decltype(mapped)>>>);
    REQUIRE(lifted.has_value());
    CHECK(payloads(*lifted) == payloads(mapped));

    // transpose is the identity traversal.
    const auto wrapped    = bt::fmap([](const int& value) { return std::optional{value}; }, subject);
    const auto transposed = bt::transpose(wrapped);
    STATIC_REQUIRE(std::same_as<decltype(transposed), const std::optional<TestType>>);
    REQUIRE(transposed.has_value());
    CHECK(payloads(*transposed) == k_payloads);
    CHECK(endpoints(*transposed) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - TraverseExpected", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject = row<TestType>::make();
    const auto widen   = [](const int& value) -> Plain {
        if (value > 2)
            return std::unexpected(std::errc::result_out_of_range);
        return static_cast<long>(value) * 100;
    };

    // The first failure in the documented order is the one reported.
    const auto failed = bt::traverse(
        [](const int& value) -> Plain {
            if (value == 2)
                return std::unexpected(std::errc::invalid_argument);
            if (value == 3)
                return std::unexpected(std::errc::result_out_of_range);
            return static_cast<long>(value);
        },
        subject);
    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error() == std::errc::invalid_argument);

    const auto last = bt::traverse(widen, subject);
    REQUIRE_FALSE(last.has_value());
    CHECK(last.error() == std::errc::result_out_of_range);

    const auto ok = bt::traverse([](const int& value) -> Plain { return static_cast<long>(value) * 100; }, subject);
    REQUIRE(ok.has_value());
    CHECK(payloads(*ok) == std::vector<long>{100, 200, 300});
    CHECK(endpoints(*ok) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - TraverseGradedExpected", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject  = row<TestType>::make();
    const auto classify = [](const int& value) -> Graded {
        if (value == 1)
            return Graded{std::unexpect, too_small{value}};
        if (value == 3)
            return Graded{std::unexpect, too_large{value}};
        return value;
    };

    // Short-circuit: the first failure in order, and only that one.
    const auto first = bt::traverse(classify, subject);
    REQUIRE_FALSE(first.has_value());
    CHECK(first.error().template holds<too_small>());
    CHECK_FALSE(first.error().template holds<too_large>());

    // Accumulating: every distinct failure, at the joined grade.
    const auto every = bt::traverse(classify, subject, bt::accumulating_applicative<Graded>);
    STATIC_REQUIRE(std::same_as<decltype(every), decltype(first)>);
    REQUIRE_FALSE(every.has_value());
    CHECK(every.error().template witness<too_small>() == std::optional{too_small{1}});
    CHECK(every.error().template witness<too_large>() == std::optional{too_large{3}});

    const auto ok = bt::traverse([](const int& value) -> Graded { return value + 1; }, subject);
    REQUIRE(ok.has_value());
    CHECK(payloads(*ok) == std::vector<int>{2, 3, 4});
    CHECK(endpoints(*ok) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - TraverseUnitEffects", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject  = row<TestType>::make();
    int        visited  = 0;
    const auto positive = [&visited](const int& value) -> Unit {
        ++visited;
        if (value > 0)
            return {};
        return std::unexpected(std::errc::invalid_argument);
    };

    // A void-valued effect yields the same shape, holding units.
    const auto ok = bt::traverse(positive, subject);
    REQUIRE(ok.has_value());
    STATIC_REQUIRE(std::same_as<bt::applicative_value_t<std::remove_cvref_t<decltype(*ok)>>, std::monostate>);
    CHECK(bt::length(*ok) == 3);
    CHECK(endpoints(*ok) == endpoints(subject));
    CHECK(visited == 3);

    const auto bad = bt::traverse(
        [](const int& value) -> Unit {
            if (value != 2)
                return {};
            return std::unexpected(std::errc::invalid_argument);
        },
        subject);
    REQUIRE_FALSE(bad.has_value());
    CHECK(bad.error() == std::errc::invalid_argument);
}

TEMPLATE_TEST_CASE("CoherenceMatrix - TraverseLanewise", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject = row<TestType>::make();
    // Each payload fans out into two lanes; the result is one structure
    // per lane, each with the source's shape.
    const auto lanes = bt::traverse([](const int& value) { return bt::zip_list<int>{{value, value * 10}}; }, subject);
    REQUIRE(lanes.data.size() == 2);
    CHECK(payloads(lanes.data[0]) == k_payloads);
    CHECK(payloads(lanes.data[1]) == std::vector<int>{10, 20, 30});
    // Reconstruction runs once per lane and must not be used up by the
    // first: the second lane has its endpoints too.
    CHECK(endpoints(lanes.data[0]) == endpoints(subject));
    CHECK(endpoints(lanes.data[1]) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - NativeCollectionIsUsedOnce", "[coherence::matrix]", MATRIX_ROWS) {
    using Policy       = ct::WriterMap<true>;
    const auto subject = row<TestType>::make();
    Policy::collects   = 0;
    const auto result  = bt::traverse(log_effect<Policy>, subject, Policy{});
    CHECK(Policy::collects == 1);
    // Each effect composed once, in the documented order.
    CHECK(result.log == "123");
    CHECK(payloads(result.held) == k_payloads);
    CHECK(endpoints(result.held) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - PairwiseCompositionWithoutCollect", "[coherence::matrix]", MATRIX_ROWS) {
    using Policy       = ct::WriterMap<false>;
    const auto subject = row<TestType>::make();
    Policy::collects   = 0;
    const auto result  = bt::traverse(log_effect<Policy>, subject, Policy{});
    CHECK(Policy::collects == 0);
    CHECK(result.log == "123");
    CHECK(payloads(result.held) == k_payloads);
    CHECK(endpoints(result.held) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - CollectionMayChangeCarrier", "[coherence::matrix]", MATRIX_ROWS) {
    // pure yields one carrier, composition another: only native collection
    // can sequence this policy, and every row must reach it.
    using Policy = ct::WriterMap<true, true>;
    STATIC_REQUIRE(bt::applicative_object_for<Policy, ct::Writer<int, true>>);
    const auto subject = row<TestType>::make();
    Policy::collects   = 0;
    const auto result  = bt::traverse(log_effect<Policy>, subject, Policy{});
    STATIC_REQUIRE(std::same_as<decltype(result), const ct::Writer<TestType, true>>);
    CHECK(Policy::collects == 1);
    CHECK(result.log == "123");
    CHECK(payloads(result.held) == k_payloads);
    CHECK(endpoints(result.held) == endpoints(subject));
}

TEMPLATE_TEST_CASE("CoherenceMatrix - StructuralFoldAgreesWithFoldable", "[coherence::matrix]", MATRIX_ROWS) {
    const auto subject    = row<TestType>::make();
    const auto structural = row<TestType>::structure(subject);

    // Which rows the monoid bridge accepts directly is part of the
    // contract: the trees do, the wrappers and std::vector do not.
    constexpr bool is_wrapper = std::same_as<TestType, ft::FingerTreeRandomAccess<int>> ||
                                std::same_as<TestType, ft::FingerTreePriorityQueue<int>> ||
                                std::same_as<TestType, ft::FingerTreeIntervalIndex<int>>;
    constexpr bool is_vector  = std::same_as<TestType, std::vector<int>>;
    STATIC_REQUIRE(bridged<TestType> == (!is_wrapper && !is_vector));

    if (is_vector) {
        CHECK_FALSE(structural.has_value());
    } else {
        REQUIRE(structural.has_value());
        CHECK(*structural == bt::fold_map(spell, subject, bt::monoid<std::string>));
    }
}

// ---------------------------------------------------------------------
// Configuration: what a tree was built with survives every context.
// ---------------------------------------------------------------------

TEST_CASE("CoherenceMatrix - ConfigurationSurvivesEveryContext", "[coherence::matrix]") {
    using Tree = ft::pmr::FingerTree<int>;
    std::pmr::monotonic_buffer_resource resource;
    const auto                          subject = Tree::from_sequence(k_payloads, &resource);

    const auto mapped = bt::fmap(spell, subject);
    CHECK(mapped.get_allocator().resource() == &resource);

    const auto optional = bt::traverse([](const int& value) { return std::optional{value}; }, subject);
    REQUIRE(optional.has_value());
    CHECK(optional->get_allocator().resource() == &resource);

    const auto expected = bt::traverse([](const int& value) -> Plain { return value; }, subject);
    REQUIRE(expected.has_value());
    CHECK(expected->get_allocator().resource() == &resource);

    const auto graded =
        bt::traverse([](const int& value) -> Graded { return value; }, subject, bt::accumulating_applicative<Graded>);
    REQUIRE(graded.has_value());
    CHECK(graded->get_allocator().resource() == &resource);

    const auto unit = bt::traverse([](const int&) -> Unit { return {}; }, subject);
    REQUIRE(unit.has_value());
    CHECK(unit->get_allocator().resource() == &resource);

    const auto lanes = bt::traverse([](const int& value) { return bt::zip_list<int>{{value, -value}}; }, subject);
    REQUIRE(lanes.data.size() == 2);
    CHECK(lanes.data[0].get_allocator().resource() == &resource);
    CHECK(lanes.data[1].get_allocator().resource() == &resource);

    using Policy       = ct::WriterMap<true, true>;
    const auto written = bt::traverse(log_effect<Policy>, subject, Policy{});
    CHECK(written.held.get_allocator().resource() == &resource);

    // The structural round trip keeps it as well.
    namespace tt = beman::typeclass_tree_adapters;
    using Part   = tt::FingerTreePart<Tree>;
    const auto rebuilt =
        ta::unfold_with<Part>(tt::finger_tree_project, bt::fmap, tt::FingerTreeEmbed<Tree>(subject), Part(subject))
            .sequence();
    CHECK(rebuilt.get_allocator().resource() == &resource);
    CHECK(rebuilt == subject);
}
