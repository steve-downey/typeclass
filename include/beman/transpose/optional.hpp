// include/beman/transpose/optional.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_OPTIONAL_HPP
#define BEMAN_TRANSPOSE_OPTIONAL_HPP

// Functor, Applicative and Monad instances for std::optional<T>.
//
// This header stands in for <optional>, which in the proposal declares the
// applicative instance itself (docs/transpose/decisions.md#instance-placement).
// Include it, not only <optional>, before the first use of a typeclass on a
// std::optional, in every translation unit that makes one: a use that precedes
// the instance is ill-formed, no diagnostic required. transpose.hpp includes
// it, and so does traverse.hpp, whose traversable_impl probes with the
// optional applicative.

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/monad.hpp>

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

namespace detail {

/** Whether `T`, ignoring cv-qualification and reference, is a
 * `std::optional` specialization. Lets an `invoke` deduce its operands
 * through forwarding references -- which is what carries the caller's value
 * category into the operation -- while still declining operands that are not
 * of the context it composes. */
//! \expos
template <class T>
struct is_optional : std::false_type {};

template <class U>
struct is_optional<std::optional<U>> : std::true_type {};

//! \expos
template <class T>
inline constexpr bool is_optional_v =
    is_optional<std::remove_cvref_t<T>>::value;

/** `holdable_value`, narrowed to what `std::optional` can hold, which
 * excludes its own tag types ([optional.optional.general]). */
//! \expos
template <class RESULT>
concept optional_holdable_value =
    holdable_value<RESULT> &&
    !std::same_as<std::remove_cvref_t<RESULT>, std::nullopt_t> &&
    !std::same_as<std::remove_cvref_t<RESULT>, std::in_place_t>;

} // namespace detail

/// Applicative instance for std::optional: the flagship of the invoke core.
/// The trailing return type keeps invoke SFINAE-friendly so availability
/// probes fail cleanly.
//! \expos
template <class VALUE_TYPE>
struct optional_applicative_impl {
    // \ref{transpose.optional.applicative}, applicative instance for optional
    template <class VALUE>
    constexpr auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>>;

    template <class FUNCTION, class FIRST, class... REST>
        requires detail::is_optional_v<FIRST> &&
                 (detail::is_optional_v<REST> && ...) &&
                 std::invocable<FUNCTION &, detail::contained_ref_t<FIRST>,
                                detail::contained_ref_t<REST>...> &&
                 detail::optional_holdable_value<std::invoke_result_t<
                     FUNCTION &, detail::contained_ref_t<FIRST>,
                     detail::contained_ref_t<REST>...>>
    constexpr auto invoke(this auto &&, FUNCTION &&function, FIRST &&first,
                          REST &&...rest)
        -> std::optional<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, detail::contained_ref_t<FIRST>,
                                 detail::contained_ref_t<REST>...>>>;
};

//! \expos
template <class VALUE_TYPE>
struct optional_applicative_map
    : derive_applicative<optional_applicative_impl<VALUE_TYPE>> {
    using optional_applicative_impl<VALUE_TYPE>::invoke;
    using optional_applicative_impl<VALUE_TYPE>::pure;
};

/// Applicative instance for `std::optional<VALUE_TYPE>`.
template <class VALUE_TYPE>
inline constexpr auto applicative<std::optional<VALUE_TYPE>> =
    optional_applicative_map<VALUE_TYPE>{};

/// Functor instance for std::optional. Functor is not proposed by this
/// paper, so this instance carries no wording.
//! \omit
template <class VALUE_TYPE>
struct OptionalFunctorImpl {
    // Constrained, not left to the body: the return type is deduced, so a
    // probe would instantiate the body and fail hard on a function that
    // cannot take the element, or one whose result an optional cannot hold
    // (a function returning void, say). The Map re-exposes this member, which
    // hides derive_functor::fmap, so its constraint is the one callers see.
    template <class F>
        requires std::invocable<F, const VALUE_TYPE &> &&
                 detail::optional_holdable_value<
                     std::invoke_result_t<F, const VALUE_TYPE &>>
    constexpr auto fmap(this auto &&, F &&function,
                        const std::optional<VALUE_TYPE> &value) {
        using Result = std::invoke_result_t<F, const VALUE_TYPE &>;
        if (!value) {
            return std::optional<std::remove_cvref_t<Result>>{};
        }
        return std::optional<std::remove_cvref_t<Result>>{
            std::invoke(std::forward<F>(function), *value)};
    }
};

//! \omit
template <class VALUE_TYPE>
struct OptionalFunctorMap : derive_functor<OptionalFunctorImpl<VALUE_TYPE>> {
    using OptionalFunctorImpl<VALUE_TYPE>::fmap;
};

/** Functor instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto functor<std::optional<VALUE_TYPE>> =
    OptionalFunctorMap<VALUE_TYPE>{};

/// Monad instance for std::optional. Monad is not proposed by this paper,
/// so this instance carries no wording. `pure` delegates to the applicative
/// instance above.
//! \omit
template <class VALUE_TYPE>
struct OptionalMonadImpl {
    using element_type = VALUE_TYPE;

    template <class VALUE>
    constexpr auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return applicative<std::optional<VALUE_TYPE>>.pure(
            std::forward<VALUE>(value));
    }

    // The continuation must return an optional: a -> m b, not a -> b. A
    // plain result would be returned as-is, with a value-initialized one
    // standing in for the empty case, which breaks the monad laws.
    template <class A, class F>
    constexpr auto bind(this auto &&, const std::optional<A> &ma, F &&f)
        -> std::remove_cvref_t<std::invoke_result_t<F, const A &>>
        requires detail::is_optional_v<std::invoke_result_t<F, const A &>>
    {
        using Result = std::remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma)
            return Result{};
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }
};

//! \omit
template <class VALUE_TYPE>
struct OptionalMonadMap : derive_monad<OptionalMonadImpl<VALUE_TYPE>> {
    using OptionalMonadImpl<VALUE_TYPE>::bind;
    using OptionalMonadImpl<VALUE_TYPE>::pure;
};

/** Monad instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto monad<std::optional<VALUE_TYPE>> =
    OptionalMonadMap<VALUE_TYPE>{};

// \rSec3[transpose.optional.applicative]{Applicative instance for optional}

//! \returns An engaged `optional` holding `value`.
template <class VALUE_TYPE>
template <class VALUE>
constexpr auto optional_applicative_impl<VALUE_TYPE>::pure(this auto &&,
                                                           VALUE &&value)
    -> std::optional<std::remove_cvref_t<VALUE>> {
    return std::optional<std::remove_cvref_t<VALUE>>{
        std::forward<VALUE>(value)};
}

//! \returns If every operand is engaged, an engaged `optional` holding the
//! result of invoking `function` with the contained values, in the order
//! written; otherwise a disengaged `optional`.
//! \remarks `function` is invoked at most once. Each operand's held value is
//! passed on with that operand's own value category, so a caller that hands
//! over an rvalue operand has its value moved from rather than copied. This
//! is what lets a traversal carry its accumulated result forward without
//! duplicating it at every step.
template <class VALUE_TYPE>
template <class FUNCTION, class FIRST, class... REST>
    requires detail::is_optional_v<FIRST> &&
             (detail::is_optional_v<REST> && ...) &&
             std::invocable<FUNCTION &, detail::contained_ref_t<FIRST>,
                            detail::contained_ref_t<REST>...> &&
             detail::optional_holdable_value<std::invoke_result_t<
                 FUNCTION &, detail::contained_ref_t<FIRST>,
                 detail::contained_ref_t<REST>...>>
constexpr auto
optional_applicative_impl<VALUE_TYPE>::invoke(this auto &&, FUNCTION &&function,
                                              FIRST &&first, REST &&...rest)
    -> std::optional<std::remove_cvref_t<
        std::invoke_result_t<FUNCTION &, detail::contained_ref_t<FIRST>,
                             detail::contained_ref_t<REST>...>>> {
    using Result = std::remove_cvref_t<
        std::invoke_result_t<FUNCTION &, detail::contained_ref_t<FIRST>,
                             detail::contained_ref_t<REST>...>>;
    if (first.has_value() && (... && rest.has_value())) {
        return std::optional<Result>{std::invoke(
            function, detail::forward_contained(std::forward<FIRST>(first)),
            detail::forward_contained(std::forward<REST>(rest))...)};
    }
    return std::optional<Result>{};
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_OPTIONAL_HPP
