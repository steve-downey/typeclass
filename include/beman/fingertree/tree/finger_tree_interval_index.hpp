// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_INTERVAL_INDEX_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_INTERVAL_INDEX_HPP

#include <beman/fingertree/tree/finger_tree_functor.hpp>
#include <beman/fingertree/tree/finger_tree_traversable.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <cassert>
#include <cstddef>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree::tree {

template <typename PAYLOAD_TYPE>
struct Interval {
    std::size_t  d_start;
    std::size_t  d_end;
    PAYLOAD_TYPE d_payload;
};

/** Measure policy: an interval measures its end point. Under
 * `max_monoid<std::size_t>` the cached measure of any subtree is its
 * maximum end point, which is what lets a query skip a subtree whole. The
 * measure value type is a plain `std::size_t`; the monoid is chosen by the
 * tree's instance type, not by wrapping the value.
 */
template <typename PAYLOAD_TYPE>
struct IntervalMeasure {
    auto operator()(const Interval<PAYLOAD_TYPE>& interval) const -> std::size_t { return interval.d_end; }
};

/** The monoid instance the interval index measures under. */
using IntervalMonoid = beman::transpose::max_monoid<std::size_t>;

/** @brief Persistent interval index supporting stabbing and overlap queries.
 *
 * @tparam PAYLOAD_TYPE Arbitrary data associated with each interval.
 * @tparam Tree         Backing finger tree type (must use IntervalMonoid over
 *                      IntervalMeasure).  Default: FingerTree — correct at all
 *                      sizes.
 *
 * Complexity:
 * - insert:        O(1) amortized
 * - query_point:   O(log n + k) where k is the number of results
 * - query_overlap: O(log n + k) where k is the number of results
 * - entries:       O(n)
 */
template <typename PAYLOAD_TYPE,
          typename Tree = FingerTree<Interval<PAYLOAD_TYPE>, IntervalMonoid, IntervalMeasure<PAYLOAD_TYPE>>>
class FingerTreeIntervalIndex {
    using Entry = Interval<PAYLOAD_TYPE>;

    Tree d_tree;

  public:
    using value_type = PAYLOAD_TYPE;

    explicit FingerTreeIntervalIndex(Tree tree) : d_tree(std::move(tree)) {}

    /** Read-only access to the configured backing representation. */
    auto backing_tree() const noexcept -> const Tree& { return d_tree; }

    template <class F>
    void for_each(F&& function) const
        requires std::invocable<F&, const PAYLOAD_TYPE&>
    {
        d_tree.for_each([&](const Interval<PAYLOAD_TYPE>& value) { std::invoke(function, value.d_payload); });
    }

    FingerTreeIntervalIndex() : d_tree(Tree{}) {}

    static auto from_intervals(std::vector<Entry> entries) -> FingerTreeIntervalIndex {
        return FingerTreeIntervalIndex{Tree::from_sequence(std::move(entries))};
    }

    auto insert(Entry entry) const -> FingerTreeIntervalIndex {
        return FingerTreeIntervalIndex{d_tree.snoc(std::move(entry))};
    }

    auto query_point(std::size_t point) const -> std::vector<PAYLOAD_TYPE> {
        std::vector<PAYLOAD_TYPE> out;
        auto                      parts = d_tree.split_at([point](std::size_t max_end) { return max_end > point; });
        parts.d_right.for_each([&](const Entry& e) {
            if (e.d_start <= point && point < e.d_end)
                out.push_back(e.d_payload);
        });
        return out;
    }

    auto query_overlap(std::size_t start, std::size_t end) const -> std::vector<PAYLOAD_TYPE> {
        std::vector<PAYLOAD_TYPE> out;
        auto                      parts = d_tree.split_at([start](std::size_t max_end) { return max_end > start; });
        parts.d_right.for_each([&](const Entry& e) {
            if (e.d_start < end && start < e.d_end)
                out.push_back(e.d_payload);
        });
        return out;
    }

    auto entries() const -> std::vector<Entry> { return d_tree.flatten(); }
};

} // namespace beman::fingertree::tree

namespace beman::fingertree {

template <class T, class A, class U>
    requires(!std::same_as<T, U>) &&
            detail::measurable_payload<tree::Interval<U>, tree::IntervalMonoid, tree::IntervalMeasure<U>> &&
            std::copy_constructible<A>
struct finger_tree_rebind<tree::FingerTree<tree::Interval<T>, tree::IntervalMonoid, tree::IntervalMeasure<T>, A>,
                          tree::Interval<U>> {
    using source_type = tree::FingerTree<tree::Interval<T>, tree::IntervalMonoid, tree::IntervalMeasure<T>, A>;
    using type        = tree::FingerTree<tree::Interval<U>, tree::IntervalMonoid, tree::IntervalMeasure<U>, A>;

    static auto make_builder(const source_type& source) {
        return detail::configured_tree_builder<type>{source.get_allocator(), source.get_monoid()};
    }
};

namespace detail {
template <class INDEX, class BUILDER>
struct interval_index_builder {
    BUILDER                                          builder;
    std::vector<std::pair<std::size_t, std::size_t>> bounds;

    auto operator()(std::vector<typename INDEX::value_type>&& payloads) -> INDEX {
        assert(payloads.size() == bounds.size());
        std::vector<tree::Interval<typename INDEX::value_type>> entries;
        entries.reserve(payloads.size());
        for (std::size_t i = 0; i < payloads.size(); ++i)
            entries.push_back({bounds[i].first, bounds[i].second, std::move(payloads[i])});
        return INDEX{std::invoke(builder, std::move(entries))};
    }
};
} // namespace detail

template <class T, class Tree, class U>
    requires detail::rebindable<Tree, tree::Interval<U>>
struct finger_tree_rebind<tree::FingerTreeIntervalIndex<T, Tree>, U> {
    using source_type = tree::FingerTreeIntervalIndex<T, Tree>;
    using type        = tree::FingerTreeIntervalIndex<U, finger_tree_rebind_t<Tree, tree::Interval<U>>>;

    static auto make_builder(const source_type& source) {
        auto builder = finger_tree_rebind<Tree, tree::Interval<U>>::make_builder(source.backing_tree());
        std::vector<std::pair<std::size_t, std::size_t>> bounds;
        source.backing_tree().for_each(
            [&](const tree::Interval<T>& entry) { bounds.emplace_back(entry.d_start, entry.d_end); });
        return detail::interval_index_builder<type, decltype(builder)>{std::move(builder), std::move(bounds)};
    }
};

template <class PAYLOAD_TYPE, class Tree>
struct FingerTreeIntervalIndexFoldableImpl {
    using element_type = PAYLOAD_TYPE;

    // The monoid is an instance object over the result type, passed by
    // value; the Impl never chooses one (beman.transpose
    // docs/transpose/decisions.md#monoid-selection).
    template <class F, class RESULT_MONOID>
    auto fold_map(this auto&&,
                  F&&                                                                         function,
                  const beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>& index,
                  RESULT_MONOID monoid) -> std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>>
        requires std::invocable<F&, const PAYLOAD_TYPE&> &&
                 beman::transpose::monoid_object<RESULT_MONOID,
                                                 std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>>>
    {
        using Result       = std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>>;
        Result accumulated = monoid.identity();
        index.for_each([&](const PAYLOAD_TYPE& value) {
            accumulated = monoid.combine(std::move(accumulated), std::invoke(function, value));
        });
        return accumulated;
    }
};

template <class PAYLOAD_TYPE, class Tree>
struct FingerTreeIntervalIndexFoldableMap
    : beman::transpose::derive_foldable<FingerTreeIntervalIndexFoldableImpl<PAYLOAD_TYPE, Tree>> {};

} // namespace beman::fingertree

namespace beman::transpose {

template <class PAYLOAD_TYPE, class Tree>
inline constexpr auto functor<beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>> =
    beman::fingertree::FingerTreeFunctorMap<beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>>{};

template <class PAYLOAD_TYPE, class Tree>
inline constexpr auto foldable<beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>> =
    beman::fingertree::FingerTreeIntervalIndexFoldableMap<PAYLOAD_TYPE, Tree>{};

} // namespace beman::transpose

namespace beman::fingertree {

template <class PAYLOAD_TYPE, class Tree>
using FingerTreeIntervalIndexTraversableImpl =
    FingerTreeSequenceTraversableImpl<tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>>;

template <class PAYLOAD_TYPE, class Tree>
struct FingerTreeIntervalIndexTraversableMap
    : beman::transpose::derive_traversable<FingerTreeIntervalIndexTraversableImpl<PAYLOAD_TYPE, Tree>> {
    using FingerTreeIntervalIndexTraversableImpl<PAYLOAD_TYPE, Tree>::traverse;
};

} // namespace beman::fingertree

namespace beman::transpose {

template <class PAYLOAD_TYPE, class Tree>
inline constexpr auto traversable<beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>> =
    beman::fingertree::FingerTreeIntervalIndexTraversableMap<PAYLOAD_TYPE, Tree>{};

} // namespace beman::transpose

#endif
