// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_INTERVAL_INDEX_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_INTERVAL_INDEX_HPP

#include <beman/fingertree/tree/finger_tree.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

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

    explicit FingerTreeIntervalIndex(Tree tree) : d_tree(std::move(tree)) {}

  public:
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
                  RESULT_MONOID monoid) -> std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>> {
        using Result = std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>>;
        return std::ranges::fold_left(index.entries(), Result(monoid.identity()), [&](Result acc, const auto& value) {
            return monoid.combine(std::move(acc), std::invoke(function, value.d_payload));
        });
    }
};

template <class PAYLOAD_TYPE, class Tree>
struct FingerTreeIntervalIndexFoldableMap
    : beman::transpose::derive_foldable<FingerTreeIntervalIndexFoldableImpl<PAYLOAD_TYPE, Tree>> {};

} // namespace beman::fingertree

namespace beman::transpose {

template <class PAYLOAD_TYPE, class Tree>
inline constexpr auto foldable<beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>> =
    beman::fingertree::FingerTreeIntervalIndexFoldableMap<PAYLOAD_TYPE, Tree>{};

} // namespace beman::transpose

namespace beman::fingertree {

template <class PAYLOAD_TYPE, class Tree>
struct FingerTreeIntervalIndexTraversableImpl {
    using element_type = PAYLOAD_TYPE;

    template <class APPLICATIVE, class F>
    auto traverse(this auto&&,
                  const APPLICATIVE&                                                          applicative,
                  F&&                                                                         function,
                  const beman::fingertree::tree::FingerTreeIntervalIndex<PAYLOAD_TYPE, Tree>& index)
        requires std::invocable<F&, const PAYLOAD_TYPE&> &&
                 beman::transpose::applicative_object_for<
                     APPLICATIVE,
                     std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>>>
    {
        using Context = std::remove_cvref_t<std::invoke_result_t<F&, const PAYLOAD_TYPE&>>;
        using U       = beman::transpose::applicative_value_t<Context>;

        using Entry = beman::fingertree::tree::Interval<PAYLOAD_TYPE>;

        const auto entries = index.entries();

        std::vector<std::pair<std::size_t, std::size_t>> bounds;
        bounds.reserve(entries.size());
        for (const auto& entry : entries) {
            bounds.emplace_back(entry.d_start, entry.d_end);
        }
        auto accumulated = beman::transpose::traversable<std::vector<Entry>>.traverse(
            applicative, [&](const Entry& entry) { return std::invoke(function, entry.d_payload); }, entries);

        return applicative.invoke(
            [bounds = std::move(bounds)](std::vector<U> payloads) {
                // Own endpoints independently of the source and deferred payload effects.
                std::vector<beman::fingertree::tree::Interval<U>> values;
                values.reserve(payloads.size());
                for (std::size_t i = 0; i < payloads.size(); ++i) {
                    values.push_back({bounds[i].first, bounds[i].second, std::move(payloads[i])});
                }
                return beman::fingertree::tree::FingerTreeIntervalIndex<U>::from_intervals(std::move(values));
            },
            std::move(accumulated));
    }
};

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
