// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_PRIORITY_QUEUE_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_PRIORITY_QUEUE_HPP

#include <beman/fingertree/tree/finger_tree_functor.hpp>
#include <beman/fingertree/tree/finger_tree_traversable.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <algorithm>
#include <cstddef>
#include <concepts>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree::tree {

/** Minimum monoid on `std::optional<T>` with an adjoined identity: `nullopt`
 * is the identity, and two engaged operands combine to the smaller. Unlike
 * `transpose::min_monoid<T>`, which needs a saturating bound, this works for
 * any totally ordered `T`.
 */
template <typename T>
struct optional_min_monoid {
    using value_type = std::optional<T>;

    auto identity() const -> std::optional<T> { return std::nullopt; }

    auto combine(const std::optional<T>& lhs, const std::optional<T>& rhs) const -> std::optional<T> {
        if (!lhs.has_value())
            return rhs;
        if (!rhs.has_value())
            return lhs;
        return *lhs <= *rhs ? lhs : rhs;
    }
};

/** Maximum monoid on `std::optional<T>` with an adjoined identity. */
template <typename T>
struct optional_max_monoid {
    using value_type = std::optional<T>;

    auto identity() const -> std::optional<T> { return std::nullopt; }

    auto combine(const std::optional<T>& lhs, const std::optional<T>& rhs) const -> std::optional<T> {
        if (!lhs.has_value())
            return rhs;
        if (!rhs.has_value())
            return lhs;
        return *lhs >= *rhs ? lhs : rhs;
    }
};

/** The priority queue's measure value: element count, minimum, maximum. A
 * plain tuple; the monoid that combines it is `PriorityMonoid<T>`, a product
 * of three instances, so adding the count cost one more tuple element and
 * one more operand, not a new tag type and specialization.
 */
template <typename T>
using PriorityTag = std::tuple<std::size_t, std::optional<T>, std::optional<T>>;

template <typename T>
using PriorityMonoid = beman::transpose::
    tuple_monoid<beman::transpose::sum_monoid<std::size_t>, optional_min_monoid<T>, optional_max_monoid<T>>;

template <typename T>
struct PriorityMeasure {
    auto operator()(const T& value) const -> PriorityTag<T> { return PriorityTag<T>{1, value, value}; }
};

/** @brief Persistent double-ended priority queue backed by a finger tree.
 *
 * @tparam T    Element type; must be totally ordered.
 * @tparam Tree Backing finger tree type (must use PriorityMonoid<T> over
 *              PriorityMeasure<T>).  Default: FingerTree — correct at all
 *              sizes.
 *
 * Complexity:
 * - push:       O(1) amortized
 * - min / max:  O(1)
 * - pop_min / pop_max: O(log n)
 * - size:       O(1) — the count is the first component of the measure
 * - to_vector:  O(n)
 */
template <typename T, typename Tree = FingerTree<T, PriorityMonoid<T>, PriorityMeasure<T>>>
class FingerTreePriorityQueue {
    Tree d_tree;

  public:
    using value_type = T;

    explicit FingerTreePriorityQueue(Tree tree) : d_tree(std::move(tree)) {}

    /** Read-only access to the configured backing representation. */
    auto backing_tree() const noexcept -> const Tree& { return d_tree; }

    template <class F>
    void for_each(F&& function) const
        requires std::invocable<F&, const T&>
    {
        d_tree.for_each([&](const T& value) { std::invoke(function, value); });
    }

    FingerTreePriorityQueue() : d_tree(Tree{}) {}

    static auto from_values(std::vector<T> values) -> FingerTreePriorityQueue {
        return FingerTreePriorityQueue{Tree::from_sequence(std::move(values))};
    }

    auto empty() const -> bool { return d_tree.is_empty(); }

    /** Returns the number of elements; O(1), read off the cached measure. */
    auto size() const -> std::size_t { return std::get<0>(d_tree.measure()); }

    auto min() const -> std::optional<T> { return std::get<1>(d_tree.measure()); }

    auto max() const -> std::optional<T> { return std::get<2>(d_tree.measure()); }

    auto push(T value) const -> FingerTreePriorityQueue {
        return FingerTreePriorityQueue{d_tree.snoc(std::move(value))};
    }

    auto pop_min() const -> std::optional<std::pair<T, FingerTreePriorityQueue>> {
        auto current = min();
        if (!current.has_value())
            return std::nullopt;
        T    global_min = *current;
        auto sp         = d_tree.split([global_min](const PriorityTag<T>& p) {
            const auto& m = std::get<1>(p);
            return m.has_value() && *m <= global_min;
        });
        if (!sp.has_value())
            return std::nullopt;
        return std::pair<T, FingerTreePriorityQueue>{sp->d_pivot,
                                                     FingerTreePriorityQueue{Tree::concat(sp->d_left, sp->d_right)}};
    }

    auto pop_max() const -> std::optional<std::pair<T, FingerTreePriorityQueue>> {
        auto current = max();
        if (!current.has_value())
            return std::nullopt;
        T    global_max = *current;
        auto sp         = d_tree.split([global_max](const PriorityTag<T>& p) {
            const auto& m = std::get<2>(p);
            return m.has_value() && *m >= global_max;
        });
        if (!sp.has_value())
            return std::nullopt;
        return std::pair<T, FingerTreePriorityQueue>{sp->d_pivot,
                                                     FingerTreePriorityQueue{Tree::concat(sp->d_left, sp->d_right)}};
    }

    auto to_vector() const -> std::vector<T> { return d_tree.flatten(); }
};

} // namespace beman::fingertree::tree

namespace beman::fingertree {

template <class T, class A, class U>
    requires(!std::same_as<T, U>) && std::totally_ordered<U> && std::copy_constructible<U> &&
            std::copy_constructible<A>
struct finger_tree_rebind<tree::FingerTree<T, tree::PriorityMonoid<T>, tree::PriorityMeasure<T>, A>, U> {
    using source_type = tree::FingerTree<T, tree::PriorityMonoid<T>, tree::PriorityMeasure<T>, A>;
    using type        = tree::FingerTree<U, tree::PriorityMonoid<U>, tree::PriorityMeasure<U>, A>;

    static auto make_builder(const source_type& source) {
        // The extrema monoids are stateless; retain the count component and
        // allocator while rebinding the two payload-dependent components.
        auto                    monoid = source.get_monoid();
        tree::PriorityMonoid<U> rebound{
            {std::get<0>(monoid.d_inners), tree::optional_min_monoid<U>{}, tree::optional_max_monoid<U>{}}};
        return detail::configured_tree_builder<type>{source.get_allocator(), std::move(rebound)};
    }
};

template <class T, class Tree, class U>
    requires detail::rebindable<Tree, U>
struct finger_tree_rebind<tree::FingerTreePriorityQueue<T, Tree>, U> {
    using source_type = tree::FingerTreePriorityQueue<T, Tree>;
    using type        = tree::FingerTreePriorityQueue<U, finger_tree_rebind_t<Tree, U>>;

    static auto make_builder(const source_type& source) {
        auto builder = finger_tree_rebind<Tree, U>::make_builder(source.backing_tree());
        return detail::wrapped_tree_builder<type, decltype(builder)>{std::move(builder)};
    }
};

template <class T, class Tree>
struct FingerTreePriorityQueueFoldableImpl {
    using element_type = T;

    // The monoid is an instance object over the result type, passed by
    // value; the Impl never chooses one (beman.transpose
    // docs/transpose/decisions.md#monoid-selection).
    template <class F, class RESULT_MONOID>
    auto fold_map(this auto&&,
                  F&&                                                              function,
                  const beman::fingertree::tree::FingerTreePriorityQueue<T, Tree>& queue,
                  RESULT_MONOID monoid) -> std::remove_cvref_t<std::invoke_result_t<F&, const T&>>
        requires std::invocable<F&, const T&> &&
                 beman::transpose::monoid_object<RESULT_MONOID,
                                                 std::remove_cvref_t<std::invoke_result_t<F&, const T&>>>
    {
        using Result       = std::remove_cvref_t<std::invoke_result_t<F&, const T&>>;
        Result accumulated = monoid.identity();
        queue.for_each([&](const T& value) {
            accumulated = monoid.combine(std::move(accumulated), std::invoke(function, value));
        });
        return accumulated;
    }
};

template <class T, class Tree>
struct FingerTreePriorityQueueFoldableMap
    : beman::transpose::derive_foldable<FingerTreePriorityQueueFoldableImpl<T, Tree>> {};

} // namespace beman::fingertree

namespace beman::transpose {

template <class T, class Tree>
inline constexpr auto functor<beman::fingertree::tree::FingerTreePriorityQueue<T, Tree>> =
    beman::fingertree::FingerTreeFunctorMap<beman::fingertree::tree::FingerTreePriorityQueue<T, Tree>>{};

template <class T, class Tree>
inline constexpr auto foldable<beman::fingertree::tree::FingerTreePriorityQueue<T, Tree>> =
    beman::fingertree::FingerTreePriorityQueueFoldableMap<T, Tree>{};

} // namespace beman::transpose

namespace beman::fingertree {

template <class T, class Tree>
using FingerTreePriorityQueueTraversableImpl =
    FingerTreeSequenceTraversableImpl<tree::FingerTreePriorityQueue<T, Tree>>;

template <class T, class Tree>
struct FingerTreePriorityQueueTraversableMap
    : beman::transpose::derive_traversable<FingerTreePriorityQueueTraversableImpl<T, Tree>> {
    using FingerTreePriorityQueueTraversableImpl<T, Tree>::traverse;
};

} // namespace beman::fingertree

namespace beman::transpose {

template <class T, class Tree>
inline constexpr auto traversable<beman::fingertree::tree::FingerTreePriorityQueue<T, Tree>> =
    beman::fingertree::FingerTreePriorityQueueTraversableMap<T, Tree>{};

} // namespace beman::transpose

#endif
