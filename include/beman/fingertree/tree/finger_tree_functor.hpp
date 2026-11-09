// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_FUNCTOR_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_FUNCTOR_HPP

#include <beman/fingertree/tree/finger_tree_rebind.hpp>
#include <beman/transpose/functor.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree {

/** Payload mapping shared by the measured sequence and its capability wrappers. */
template <class STRUCTURE>
struct FingerTreeFunctorImpl {
    using element_type = typename STRUCTURE::value_type;

    template <class F>
    constexpr auto fmap(this auto&&, F&& function, const STRUCTURE& source)
        requires std::invocable<F&, const element_type&> &&
                 detail::rebindable<STRUCTURE, std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>,
                                         std::invoke_result_t<F&, const element_type&>>
    {
        using U                = std::remove_cvref_t<std::invoke_result_t<F&, const element_type&>>;
        auto           builder = finger_tree_rebind<STRUCTURE, U>::make_builder(source);
        std::vector<U> values;
        source.for_each([&](const element_type& value) { values.emplace_back(std::invoke(function, value)); });
        return std::invoke(builder, std::move(values));
    }
};

template <class STRUCTURE>
struct FingerTreeFunctorMap : transpose::derive_functor<FingerTreeFunctorImpl<STRUCTURE>> {};

} // namespace beman::fingertree

namespace beman::transpose {

template <class T, class M, class P, class A>
inline constexpr auto functor<beman::fingertree::tree::FingerTree<T, M, P, A>> =
    beman::fingertree::FingerTreeFunctorMap<beman::fingertree::tree::FingerTree<T, M, P, A>>{};

} // namespace beman::transpose

#endif
