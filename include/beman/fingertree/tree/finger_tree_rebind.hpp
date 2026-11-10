// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_REBIND_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_REBIND_HPP

#include <beman/fingertree/tree/finger_tree.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree {

/** Declares a result representation and an owned reconstruction callable.
 * A specialization supplies type and make_builder(const SourceTree&).
 * The builder accepts vector<U>&& and returns exactly type. Unsupported
 * rebindings leave this primary undefined; no measure law is inferred.
 */
template <class SourceTree, class U>
struct finger_tree_rebind;

template <class SourceTree, class U>
using finger_tree_rebind_t = typename finger_tree_rebind<std::remove_cvref_t<SourceTree>, U>::type;

namespace detail {

template <class T, class M, class P>
concept measurable_payload =
    std::move_constructible<T> && std::default_initializable<P> && std::copy_constructible<M> &&
    std::copy_constructible<transpose::monoid_value_t<M>> &&
    transpose::monoid_object<M, transpose::monoid_value_t<M>> && requires(P& measure, const T& value) {
        { std::invoke(measure, value) } -> std::convertible_to<transpose::monoid_value_t<M>>;
    };

template <class TREE>
struct configured_tree_builder {
    typename TREE::allocator_type allocator;
    typename TREE::monoid_type    monoid;

    auto operator()(std::vector<typename TREE::value_type>&& values) const -> TREE {
        return TREE::from_sequence(std::move(values), allocator, monoid);
    }
};

template <class SOURCE, class U>
using rebind_builder_t = decltype(finger_tree_rebind<std::remove_cvref_t<SOURCE>, U>::make_builder(
    std::declval<const std::remove_cvref_t<SOURCE>&>()));

template <class SOURCE, class U>
concept rebindable = transpose::detail::holdable_value<U> && std::move_constructible<U> &&
                     requires(const std::remove_cvref_t<SOURCE>& source) {
                         typename finger_tree_rebind_t<SOURCE, U>;
                         finger_tree_rebind<std::remove_cvref_t<SOURCE>, U>::make_builder(source);
                     } && std::move_constructible<rebind_builder_t<SOURCE, U>> &&
                     requires(rebind_builder_t<SOURCE, U>& builder, std::vector<U>& values) {
                         { std::invoke(builder, std::move(values)) } -> std::same_as<finger_tree_rebind_t<SOURCE, U>>;
                     };

template <class WRAPPER, class BUILDER>
struct wrapped_tree_builder {
    BUILDER builder;

    auto operator()(std::vector<typename WRAPPER::value_type>&& values) -> WRAPPER {
        return WRAPPER{std::invoke(builder, std::move(values))};
    }
};

} // namespace detail

/** Same-payload reconstruction retains the complete configuration. */
template <class T, class M, class P, class A>
    requires detail::measurable_payload<T, M, P> && std::copy_constructible<A>
struct finger_tree_rebind<tree::FingerTree<T, M, P, A>, T> {
    using type = tree::FingerTree<T, M, P, A>;

    static auto make_builder(const type& source) {
        return detail::configured_tree_builder<type>{source.get_allocator(), source.get_monoid()};
    }
};

/** The count policy has a declared payload-independent measure law. */
template <class T, class M, class TAG, class A, class U>
    requires(!std::same_as<T, U>) && detail::measurable_payload<U, M, tree::UnitMeasure<U, TAG>> &&
            std::copy_constructible<A>
struct finger_tree_rebind<tree::FingerTree<T, M, tree::UnitMeasure<T, TAG>, A>, U> {
    using source_type = tree::FingerTree<T, M, tree::UnitMeasure<T, TAG>, A>;
    using type        = tree::FingerTree<U, M, tree::UnitMeasure<U, TAG>, A>;

    static auto make_builder(const source_type& source) {
        return detail::configured_tree_builder<type>{source.get_allocator(), source.get_monoid()};
    }
};

} // namespace beman::fingertree

#endif
