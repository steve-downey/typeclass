// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_TRAVERSABLE_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_TRAVERSABLE_HPP

// Traversable constraint for FingerTree:
// - Materialization: traverse materializes via flatten() into a vector.
// - Reconstruction: results are rebuilt via FingerTree<U>::from_sequence(),
//   applying the default UnitMeasure, monoid, and allocator to the result type.
// - Applicative semantics: traversals run left-to-right independent of tree
//   structure.
//
// Traversal is not a primary performance path, so O(n) materialization is
// acceptable. Wrappers that need structure-preserving traversal can override.

#include <beman/fingertree/tree/finger_tree.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree {

/** Traversable typeclass implementation for FingerTree; materialises via
 * flatten() then reconstructs with from_sequence(); O(n).
 */
template <class T, class MONOID, class MEASURE_POLICY, class ALLOCATOR>
struct FingerTreeTraversableImpl {
    using element_type = T;

    template <class APPLICATIVE, class F>
    auto traverse(this auto&&,
                  const APPLICATIVE&                                                               applicative,
                  F&&                                                                              function,
                  const beman::fingertree::tree::FingerTree<T, MONOID, MEASURE_POLICY, ALLOCATOR>& tree)
        requires std::invocable<F&, const T&> &&
                 beman::transpose::applicative_object_for<APPLICATIVE,
                                                          std::remove_cvref_t<std::invoke_result_t<F&, const T&>>>
    {
        using Context = std::remove_cvref_t<std::invoke_result_t<F&, const T&>>;
        using U       = beman::transpose::applicative_value_t<Context>;

        // Keep the persistent input's const-reference element convention.
        const auto values = tree.flatten();
        auto       accumulated =
            beman::transpose::traversable<std::vector<T>>.traverse(applicative, std::forward<F>(function), values);

        return applicative.invoke(
            [](std::vector<U> values) {
                return beman::fingertree::tree::FingerTree<U>::from_sequence(std::move(values));
            },
            std::move(accumulated));
    }
};

/** Traversable typeclass map entry for FingerTree. */
template <class T, class MONOID, class MEASURE_POLICY, class ALLOCATOR>
struct FingerTreeTraversableMap
    : beman::transpose::derive_traversable<FingerTreeTraversableImpl<T, MONOID, MEASURE_POLICY, ALLOCATOR>> {
    using FingerTreeTraversableImpl<T, MONOID, MEASURE_POLICY, ALLOCATOR>::traverse;
};

} // namespace beman::fingertree

namespace beman::transpose {

/** Registers FingerTree as a Traversable for all monoid and measure
 * combinations.
 */
template <class T, class MONOID, class MEASURE_POLICY, class ALLOCATOR>
inline constexpr auto traversable<beman::fingertree::tree::FingerTree<T, MONOID, MEASURE_POLICY, ALLOCATOR>> =
    beman::fingertree::FingerTreeTraversableMap<T, MONOID, MEASURE_POLICY, ALLOCATOR>{};

} // namespace beman::transpose

#endif
