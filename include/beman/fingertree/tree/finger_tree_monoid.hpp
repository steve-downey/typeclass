// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_MONOID_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_MONOID_HPP

#include <beman/fingertree/tree/finger_tree.hpp>
#include <beman/transpose/monoid.hpp>

#include <concepts>
#include <utility>
#include <vector>

namespace beman::fingertree {

/** Concatenation under a selected configuration. Operands belong to the
 * prototype's measure-law domain. Both are rebuilt under its allocator and
 * monoid, so cached measures and foreign resources never leak into the
 * result. O(n + m); copying payloads is required for this explicit rebuild.
 */
template <class TREE>
class finger_tree_concat_monoid {
    typename TREE::allocator_type d_allocator;
    typename TREE::monoid_type    d_monoid;

  public:
    using value_type = TREE;

    explicit finger_tree_concat_monoid(const TREE& prototype)
        : d_allocator(prototype.get_allocator()), d_monoid(prototype.get_monoid()) {}

    auto identity() const -> TREE { return TREE(d_allocator, d_monoid); }

    auto combine(const TREE& left, const TREE& right) const -> TREE
        requires std::copy_constructible<typename TREE::value_type>
    {
        std::vector<typename TREE::value_type> values;
        auto                                   collect = [&](const auto& value) { values.push_back(value); };
        left.for_each(collect);
        right.for_each(collect);
        return TREE::from_sequence(std::move(values), d_allocator, d_monoid);
    }
};

} // namespace beman::fingertree

namespace beman::transpose {

/** The canonical monoid is limited to default count-measured, default-
 * allocated trees. Custom configurations select an explicit instance.
 */
template <class T>
struct Monoid<beman::fingertree::tree::FingerTree<T>> {
    using value_type = beman::fingertree::tree::FingerTree<T>;

    auto identity() const -> value_type { return {}; }
    auto combine(const value_type& left, const value_type& right) const -> value_type {
        return value_type::concat(left, right);
    }
};

} // namespace beman::transpose

#endif
