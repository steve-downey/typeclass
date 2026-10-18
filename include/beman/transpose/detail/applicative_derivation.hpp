// include/beman/transpose/detail/applicative_derivation.hpp          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_DETAIL_APPLICATIVE_DERIVATION_HPP
#define BEMAN_TRANSPOSE_DETAIL_APPLICATIVE_DERIVATION_HPP

// Machinery for the Applicative dual-basis derivations. It lives here rather
// than in apply.hpp so that apply.hpp -- the header the wording is generated
// from -- contains only entities the specification describes.

#include <beman/transpose/detail/typeclass_base.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose {

namespace detail {

// Currying support for the invoke-from-ap derivation: wraps a plain n-ary
// function so it can be fed through one-step contextual application,
// consuming one argument at a time and invoking once all ARITY arguments
// are bound. This cost exists only in the derivation direction -- C++ is
// not natively curried -- and never at invoke-native call sites.
//
// The arity is fixed when the partial is made, from the number of
// arguments the caller of invoke passed, and never inferred from what the
// function will accept. Inferring it -- invoke as soon as the function
// accepts the bound arguments, otherwise keep binding -- turns every
// failure into another bound argument: a function that cannot take the
// arguments, or is given too few, yields a half-applied partial in the
// context instead of failing. The saturating call is constrained and
// spells its result type, so a function that cannot take the arguments
// makes the context's ap fail substitution, and with it the derived
// invoke's constraint (ap_chain_valid below).
//
// Every call copies what it binds rather than moving it: the context's ap
// may call one held partial many times (once per element of a list), and
// each call must see the same bound arguments.
template <std::size_t ARITY, class FUNCTION, class... BOUND_ARGS>
struct terminating_partial {
    static_assert(sizeof...(BOUND_ARGS) < ARITY);

    FUNCTION function;
    std::tuple<BOUND_ARGS...> bound_args;

    template <class NEXT_ARG>
        requires(sizeof...(BOUND_ARGS) + 1 == ARITY) &&
                std::invocable<FUNCTION &, BOUND_ARGS &..., NEXT_ARG>
    constexpr auto operator()(NEXT_ARG &&next_arg)
        -> std::invoke_result_t<FUNCTION &, BOUND_ARGS &..., NEXT_ARG> {
        return std::apply(
            [&](BOUND_ARGS &...bound) -> decltype(auto) {
                return std::invoke(function, bound...,
                                   std::forward<NEXT_ARG>(next_arg));
            },
            bound_args);
    }

    template <class NEXT_ARG>
        requires(sizeof...(BOUND_ARGS) + 1 == ARITY) &&
                std::invocable<const FUNCTION &, const BOUND_ARGS &...,
                               NEXT_ARG>
    constexpr auto operator()(NEXT_ARG &&next_arg) const
        -> std::invoke_result_t<const FUNCTION &, const BOUND_ARGS &...,
                                NEXT_ARG> {
        return std::apply(
            [&](const BOUND_ARGS &...bound) -> decltype(auto) {
                return std::invoke(function, bound...,
                                   std::forward<NEXT_ARG>(next_arg));
            },
            bound_args);
    }

    template <class NEXT_ARG>
        requires(sizeof...(BOUND_ARGS) + 1 < ARITY)
    constexpr auto operator()(NEXT_ARG &&next_arg) const
        -> terminating_partial<ARITY, FUNCTION, BOUND_ARGS...,
                               std::remove_cvref_t<NEXT_ARG>> {
        return {function,
                std::tuple_cat(bound_args,
                               std::tuple<std::remove_cvref_t<NEXT_ARG>>{
                                   std::forward<NEXT_ARG>(next_arg)})};
    }
};

template <std::size_t ARITY, class FUNCTION>
constexpr auto make_terminating_partial(FUNCTION &&function)
    -> terminating_partial<ARITY, std::remove_cvref_t<FUNCTION>> {
    return {std::forward<FUNCTION>(function), std::tuple<>{}};
}

// Whether ap_chain can fold ACCUMULATED through every argument: each step
// self.ap(accumulated, argument) is well-formed, and its result is the
// next step's accumulated value. The trait only names each step's type, so
// it relies on the bases' ap being SFINAE-friendly -- the same rule the
// class invariant already imposes on every Impl.
template <class SELF, class ACCUMULATED, class... ARGUMENTS>
struct ap_chain_valid : std::false_type {};

template <class SELF, class ACCUMULATED>
struct ap_chain_valid<SELF, ACCUMULATED> : std::true_type {};

template <class SELF, class ACCUMULATED, class FIRST, class... REST>
    requires requires {
        std::declval<SELF>().ap(std::declval<ACCUMULATED>(),
                                std::declval<FIRST>());
    }
struct ap_chain_valid<SELF, ACCUMULATED, FIRST, REST...>
    : ap_chain_valid<SELF,
                     decltype(std::declval<SELF>().ap(
                         std::declval<ACCUMULATED>(), std::declval<FIRST>())),
                     REST...> {};

// Whether the invoke-from-ap derivation of SELF.invoke(function,
// arguments...) is well-formed: pure can embed the curried function, and
// ap can then apply it to every argument in turn, the last application
// invoking it.
//! \expos
template <class SELF, class FUNCTION, class... ARGUMENTS>
concept invocable_through_ap =
    ap_chain_valid<SELF,
                   decltype(std::declval<SELF>().pure(
                       make_terminating_partial<sizeof...(ARGUMENTS)>(
                           std::declval<FUNCTION>()))),
                   ARGUMENTS...>::value;

// The evaluator that recovers one-step application (ap) from the n-ary
// core: ap(cf, cx) = invoke(applicative_eval, cf, cx) -- GHC's
// (<*>) = liftA2 id. The trailing return type keeps it SFINAE-visible so
// the derived ap's constraint fails cleanly -- never hard-errors -- for
// contexts that cannot hold callables.
struct applicative_eval_t {
    template <class FUNCTION, class ARGUMENT>
    constexpr auto operator()(FUNCTION &&function, ARGUMENT &&argument) const
        -> std::invoke_result_t<FUNCTION, ARGUMENT> {
        return std::invoke(std::forward<FUNCTION>(function),
                           std::forward<ARGUMENT>(argument));
    }
};
inline constexpr applicative_eval_t applicative_eval{};

// Free-function form for generic internal use.
template <class APPLICATIVE_MAP, class FUNCTIONS_IN_CONTEXT,
          class ARGUMENTS_IN_CONTEXT>
constexpr auto ap(const APPLICATIVE_MAP &map, FUNCTIONS_IN_CONTEXT &&functions,
                  ARGUMENTS_IN_CONTEXT &&arguments) {
    return map.invoke(applicative_eval,
                      std::forward<FUNCTIONS_IN_CONTEXT>(functions),
                      std::forward<ARGUMENTS_IN_CONTEXT>(arguments));
}

} // namespace detail

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_DETAIL_APPLICATIVE_DERIVATION_HPP
