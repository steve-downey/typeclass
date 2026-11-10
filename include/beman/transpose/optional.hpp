// include/beman/transpose/optional.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_OPTIONAL_HPP
#define BEMAN_TRANSPOSE_OPTIONAL_HPP

// Functor, Applicative, Monad and Alternative instances for std::optional<T>.
//
// This header stands in for <optional>, which in the proposal declares the
// applicative instance itself (docs/transpose/decisions.md#instance-placement).
// Include it, not only <optional>, before the first use of a typeclass on a
// std::optional, in every translation unit that makes one: a use that precedes
// the instance is ill-formed, no diagnostic required. beman/transpose.hpp
// includes it, and so does traverse.hpp, whose traversable_impl probes with the
// optional applicative.

#include <beman/transpose/alternative.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/monad.hpp>

#include <concepts>
#include <cstddef>
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

//! \expos
//! \remarks `$optional-invoke-nothrow$<FUNCTION, FIRST, REST...>` is `true`
//! when invoking `FUNCTION` on the values the operands hold, each in its
//! operand's value category, and constructing an `optional` from the result
//! are both non-throwing.
template <class FUNCTION, class FIRST, class... REST>
inline constexpr bool optional_invoke_nothrow = invokes_nothrow_into<
    std::optional<std::remove_cvref_t<std::invoke_result_t<
        FUNCTION &, contained_ref_t<FIRST>, contained_ref_t<REST>...>>>,
    FUNCTION &, contained_ref_t<FIRST>, contained_ref_t<REST>...>;

} // namespace detail

/// Applicative instance for std::optional: the flagship of the invoke core.
/// The trailing return type keeps invoke SFINAE-friendly so availability
/// probes fail cleanly.
//! \expos
template <class VALUE_TYPE>
struct optional_applicative_impl {
    // \ref{transpose.optional.applicative}, applicative instance for optional
    template <class VALUE>
    [[nodiscard]] constexpr auto pure(this auto &&, VALUE &&value) noexcept(
        std::is_nothrow_constructible_v<
            std::optional<std::remove_cvref_t<VALUE>>, VALUE>)
        -> std::optional<std::remove_cvref_t<VALUE>>;

    [[nodiscard]] constexpr auto zero(this auto &&) noexcept
        -> std::optional<VALUE_TYPE>;

    template <class FUNCTION, class FIRST, class... REST>
        requires detail::is_optional_v<FIRST> &&
                 (detail::is_optional_v<REST> && ...) &&
                 std::invocable<FUNCTION &, detail::contained_ref_t<FIRST>,
                                detail::contained_ref_t<REST>...> &&
                 detail::optional_holdable_value<std::invoke_result_t<
                     FUNCTION &, detail::contained_ref_t<FIRST>,
                     detail::contained_ref_t<REST>...>>
    [[nodiscard]] constexpr auto
    invoke(this auto &&, FUNCTION &&function, FIRST &&first,
           REST &&...rest) noexcept(detail::optional_invoke_nothrow<FUNCTION,
                                                                    FIRST,
                                                                    REST...>)
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
    using optional_applicative_impl<VALUE_TYPE>::zero;
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
    [[nodiscard]] constexpr auto
    fmap(this auto &&, F &&function,
         const std::optional<VALUE_TYPE>
             &value) noexcept(detail::
                                  invokes_nothrow_into<
                                      std::optional<std::remove_cvref_t<
                                          std::invoke_result_t<
                                              F, const VALUE_TYPE &>>>,
                                      F, const VALUE_TYPE &>) {
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
    [[nodiscard]] constexpr auto pure(this auto &&, VALUE &&value) noexcept(
        noexcept(applicative<std::optional<VALUE_TYPE>>.pure(
            std::forward<VALUE>(value))))
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return applicative<std::optional<VALUE_TYPE>>.pure(
            std::forward<VALUE>(value));
    }

    // The continuation must return an optional: a -> m b, not a -> b. A
    // plain result would be returned as-is, with a value-initialized one
    // standing in for the empty case, which breaks the monad laws.
    template <class A, class F>
    [[nodiscard]] constexpr auto
    bind(this auto &&, const std::optional<A> &ma, F &&f) noexcept(
        detail::invokes_nothrow_into<
            std::remove_cvref_t<std::invoke_result_t<F, const A &>>, F,
            const A &>)
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

// -- The Alternative instance ----------------------------------------------
//
// Evidence, not proposed: see alternative.hpp. The disjunction over optional
// is the leftmost engaged operand, at the join of the operands' value types;
// a bare last operand cannot fail and makes the result total; `zero` is the
// disengaged optional the applicative object declares first.

namespace detail {

/** The value type an operand of a disjunction over `optional` contributes:
 * the contained type of an `optional`, the operand itself otherwise. */
//! \omit
template <class OPERAND>
struct optional_disjunction_value {
    using type = std::remove_cvref_t<OPERAND>;
};

//! \omit
template <class VALUE>
struct optional_disjunction_value<std::optional<VALUE>> {
    using type = VALUE;
};

//! \omit
template <class OPERAND>
using optional_disjunction_value_t =
    typename optional_disjunction_value<std::remove_cvref_t<OPERAND>>::type;

/** An operand's contribution with its own value category: the held value of
 * an `optional`, the operand itself otherwise. A specialization rather than
 * a conditional, so that `contained_ref_t` is never formed for a bare
 * operand. */
//! \omit
template <class OPERAND>
struct optional_disjunction_ref {
    using type = bare_ref_t<OPERAND>;
};

//! \omit
template <class OPERAND>
    requires is_optional_v<OPERAND>
struct optional_disjunction_ref<OPERAND> {
    using type = contained_ref_t<OPERAND>;
};

//! \omit
template <class OPERAND>
using optional_disjunction_ref_t =
    typename optional_disjunction_ref<OPERAND>::type;

/** The shape of a disjunction over `optional`: every operand but the last is
 * an `optional`, and the last is an `optional` or a bare value. An operand
 * after one that cannot fail would never be consulted while its type still
 * entered the join; the shape rules it out. */
//! \omit
template <class... OPERANDS>
inline constexpr bool optional_disjunction_shape_v =
    []<std::size_t... INDICES>(std::index_sequence<INDICES...>) {
        return (
            (is_optional_v<OPERANDS> || (INDICES + 1 == sizeof...(OPERANDS) &&
                                         infallible_operand<OPERANDS>)) &&
            ...);
    }(std::index_sequence_for<OPERANDS...>{});

//! \omit
template <class... OPERANDS>
using optional_disjunction_join_t =
    disjunction_join_t<optional_disjunction_value_t<OPERANDS>...>;

/** The result of a disjunction over `optional`: the join in an `optional`
 * when every operand is one, the join itself when the last cannot fail. */
//! \omit
template <class... OPERANDS>
using optional_alt_result_t =
    std::conditional_t<(is_optional_v<OPERANDS> && ...),
                       std::optional<optional_disjunction_join_t<OPERANDS...>>,
                       optional_disjunction_join_t<OPERANDS...>>;

//! \omit
template <class... OPERANDS>
concept optional_disjunction_joinable =
    (joins_into<optional_disjunction_join_t<OPERANDS...>,
                optional_disjunction_ref_t<OPERANDS>> &&
     ...);

/** Whether an operand already is the result: an `optional` of the result's
 * own type. Such an operand is passed on whole, by its copy or move
 * constructor, rather than unwrapped and re-wrapped through the
 * `in_place_t` constructor. The value is the same either way; the
 * difference is the exception specification. The standard gives
 * `optional`'s move constructor a conditional `noexcept` and leaves its
 * `in_place_t` constructor without one, which a library may add or not, so
 * only the first lets `alt(a, a)` be non-throwing on every implementation.
 */
//! \omit
template <class RESULT, class OPERAND>
inline constexpr bool optional_alt_passes_whole_v =
    is_optional_v<OPERAND> &&
    std::is_same_v<std::remove_cvref_t<OPERAND>, RESULT>;

/** Whether one operand's contribution reaches the result without throwing:
 * passed on whole, or placed into the join and the join into the result. */
//! \omit
template <class RESULT, class JOIN, class OPERAND>
inline constexpr bool optional_alt_operand_nothrow_v = [] {
    if constexpr (optional_alt_passes_whole_v<RESULT, OPERAND>) {
        return std::is_nothrow_constructible_v<RESULT, bare_ref_t<OPERAND>>;
    } else if constexpr (std::is_same_v<RESULT, JOIN>) {
        return noexcept(into_join<JOIN>(
                   std::declval<optional_disjunction_ref_t<OPERAND>>())) &&
               std::is_nothrow_move_constructible_v<JOIN>;
    } else {
        return noexcept(into_join<JOIN>(
                   std::declval<optional_disjunction_ref_t<OPERAND>>())) &&
               std::is_nothrow_constructible_v<RESULT, std::in_place_t, JOIN>;
    }
}();

/** Whether the disjunction is non-throwing: every operand's contribution
 * reaches the result without throwing. */
//! \omit
template <class... OPERANDS>
inline constexpr bool optional_alt_nothrow =
    (optional_alt_operand_nothrow_v<optional_alt_result_t<OPERANDS...>,
                                    optional_disjunction_join_t<OPERANDS...>,
                                    OPERANDS> &&
     ...);

/** Places a contribution: into the join, and the join into the `optional`
 * when the result is one. */
//! \omit
template <class RESULT, class JOIN, class REF>
constexpr auto optional_alt_place(REF &&contribution) -> RESULT {
    if constexpr (std::is_same_v<RESULT, JOIN>) {
        return into_join<JOIN>(std::forward<REF>(contribution));
    } else {
        return RESULT{std::in_place,
                      into_join<JOIN>(std::forward<REF>(contribution))};
    }
}

/** The leftmost engaged operand's contribution; the disengaged result when
 * every operand is a disengaged `optional`; a bare operand, reached only
 * last, as it is. */
//! \omit
template <class RESULT, class JOIN, class FIRST, class... REST>
constexpr auto optional_alt_select(FIRST &&first, REST &&...rest) -> RESULT {
    if constexpr (is_optional_v<FIRST>) {
        if (first.has_value()) {
            if constexpr (optional_alt_passes_whole_v<RESULT, FIRST>) {
                return RESULT(bare_ref(std::forward<FIRST>(first)));
            } else {
                return optional_alt_place<RESULT, JOIN>(
                    forward_contained(std::forward<FIRST>(first)));
            }
        }
        if constexpr (sizeof...(REST) == 0) {
            return RESULT{};
        } else {
            return optional_alt_select<RESULT, JOIN>(
                std::forward<REST>(rest)...);
        }
    } else {
        return optional_alt_place<RESULT, JOIN>(
            bare_ref(std::forward<FIRST>(first)));
    }
}

} // namespace detail

/// Alternative instance for std::optional. Alternative is not proposed by
/// this paper, so this instance carries no wording. `zero` delegates to the
/// applicative instance above, which declares it first.
//! \omit
template <class VALUE_TYPE>
struct optional_alternative_impl {
    /** The leftmost engaged operand, at the join of the operands' value
     * types. Each operand's held value is passed on with that operand's own
     * value category, as `invoke` passes it. */
    template <class FIRST, class... REST>
        requires(sizeof...(REST) > 0) &&
                detail::optional_disjunction_shape_v<FIRST, REST...> &&
                detail::optional_disjunction_joinable<FIRST, REST...>
    [[nodiscard]] constexpr auto
    alt(this auto &&, FIRST &&first,
        REST &&...rest) noexcept(detail::optional_alt_nothrow<FIRST, REST...>)
        -> detail::optional_alt_result_t<FIRST, REST...> {
        return detail::optional_alt_select<
            detail::optional_alt_result_t<FIRST, REST...>,
            detail::optional_disjunction_join_t<FIRST, REST...>>(
            std::forward<FIRST>(first), std::forward<REST>(rest)...);
    }

    [[nodiscard]] constexpr auto zero(this auto &&) noexcept
        -> std::optional<VALUE_TYPE> {
        return applicative<std::optional<VALUE_TYPE>>.zero();
    }
};

//! \omit
template <class VALUE_TYPE>
struct optional_alternative_map
    : derive_alternative<optional_alternative_impl<VALUE_TYPE>> {
    using optional_alternative_impl<VALUE_TYPE>::alt;
    using optional_alternative_impl<VALUE_TYPE>::zero;
};

/** Alternative instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto alternative<std::optional<VALUE_TYPE>> =
    optional_alternative_map<VALUE_TYPE>{};

// \rSec3[transpose.optional.applicative]{Applicative instance for optional}

//! \returns An engaged `optional` holding `value`.
template <class VALUE_TYPE>
template <class VALUE>
[[nodiscard]] constexpr auto optional_applicative_impl<VALUE_TYPE>::
    pure(this auto &&, VALUE &&value) noexcept(
        std::is_nothrow_constructible_v<
            std::optional<std::remove_cvref_t<VALUE>>, VALUE>)
        -> std::optional<std::remove_cvref_t<VALUE>> {
    return std::optional<std::remove_cvref_t<VALUE>>{
        std::forward<VALUE>(value)};
}

//! \returns A disengaged `optional<VALUE_TYPE>`.
//! \remarks The failure state of `optional` carries no witness, so `optional`
//! has a `zero`; it is the identity of the first-engaged disjunction that
//! `first_monoid` already folds with.
template <class VALUE_TYPE>
[[nodiscard]] constexpr auto
optional_applicative_impl<VALUE_TYPE>::zero(this auto &&) noexcept
    -> std::optional<VALUE_TYPE> {
    return std::optional<VALUE_TYPE>{};
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
[[nodiscard]] constexpr auto optional_applicative_impl<VALUE_TYPE>::invoke(
    this auto &&, FUNCTION &&function, FIRST &&first,
    REST &&...rest) noexcept(detail::optional_invoke_nothrow<FUNCTION, FIRST,
                                                             REST...>)
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
