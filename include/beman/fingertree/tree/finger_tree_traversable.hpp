// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_TRAVERSABLE_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_TRAVERSABLE_HPP

// Persistent sources are walked by reference. Transpose owns effect
// sequencing; the declared rebind owns configured reconstruction.
#include <beman/fingertree/tree/finger_tree_rebind.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree {

/** Reference-reading traversal shared by the sequence and its wrappers. */
template <class STRUCTURE>
struct FingerTreeSequenceTraversableImpl {
    using element_type = typename STRUCTURE::value_type;

    template <class APPLICATIVE, class F>
    constexpr auto traverse(this auto&&, const APPLICATIVE& applicative, F&& function, const STRUCTURE& source)
        requires std::invocable<F&, const element_type&> &&
                 transpose::applicative_object_for<
                     APPLICATIVE,
                     std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>> &&
                 std::move_constructible<std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>,
                                         std::invoke_result_t<F&, const element_type&>> &&
                 detail::rebindable<STRUCTURE,
                                    transpose::applicative_value_t<
                                        std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>>>
    {
        using Context = std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>;
        using U       = transpose::applicative_value_t<Context>;
        auto builder  = finger_tree_rebind<STRUCTURE, U>::make_builder(source);

        // The pointers exist only while the vector primitive invokes the
        // mapper. Deferred effects and reconstruction own their own data.
        std::vector<const element_type*> positions;
        source.for_each([&](const element_type& value) { positions.push_back(std::addressof(value)); });
        auto accumulated = transpose::traversable<decltype(positions)>.traverse(
            applicative,
            [&](const element_type* value) -> Context { return std::invoke(function, *value); },
            positions);
        return applicative.invoke(
            [builder = std::move(builder)](std::vector<U> values) mutable {
                return std::invoke(builder, std::move(values));
            },
            std::move(accumulated));
    }
};

template <class T, class MONOID, class MEASURE_POLICY, class ALLOCATOR>
using FingerTreeTraversableImpl =
    FingerTreeSequenceTraversableImpl<tree::FingerTree<T, MONOID, MEASURE_POLICY, ALLOCATOR>>;

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
