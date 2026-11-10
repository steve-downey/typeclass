// include/beman/transpose/eliminate.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_ELIMINATE_HPP
#define BEMAN_TRANSPOSE_ELIMINATE_HPP

// Elimination of a one-of sum with heterogeneous arms.
//
// `std::visit` requires every arm to return one type, so an algebra over a
// variant-shaped layer that rebuilds the layer at a new element type spells
// `-> Layer<B>` on every arm, and a visitor whose arms naturally produce
// different things names their union by hand. `eliminate` removes that: its
// result type is the JOIN OF THE ARMS -- the one type every arm returns when
// they agree, and otherwise `std::variant` of the distinct result types, in
// alternative order. The join is a value type and nothing more. No operation
// in this library reads it as a grade; `fmap` over such a result receives the
// whole variant (docs/transpose/decisions.md#value-side-grading).
//
// Deduplication is by identity, in alternative order, and NOT canonical
// (sorted) order: a variant's alternatives are positional, and the join of
// arms that rebuild a `variant<Zero, Succ<B>>` must be exactly that type,
// which a sorted set would not be. That is the one way this differs from the
// error_set's canonical set, and it is why the two share only the dedupe
// (docs/transpose/decisions.md#heterogeneous-arm-elimination).
//
// `eliminate_indexed` passes the alternative's index tag to the arm as well,
// for arms that must tell two alternatives of one type apart, or that want
// the position as a template argument.
//
// Proposed beside `overloaded` (docs/transpose/decisions.md#overloaded-home),
// whose consteval catch-all is the exhaustiveness diagnostic: an alternative
// no arm accepts is a compile error naming the alternative. Wording for both
// is pending, with `overloaded`'s.

#include <beman/transpose/detail/canonical_set.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace beman::transpose {

namespace detail {

/** Whether `HANDLER` has an arm for `ARGS`: invocable, and not returning
 * `void`. `overloaded`'s consteval catch-all is invocable and returns `void`,
 * so this is what reports it as the missing arm it stands for, before its
 * own diagnostic fires. */
//! \expos
template <class HANDLER, class... ARGS>
concept sum_arm_for = std::invocable<HANDLER &, ARGS...> &&
                      !std::is_void_v<std::invoke_result_t<HANDLER &, ARGS...>>;

/** One arm's result for one alternative, decayed. One instantiation per
 * alternative, so that the failing assertion names the alternative in its
 * instantiation context. */
//! \expos
template <class HANDLER, class... ARGS>
struct arm_result {
    static_assert(sum_arm_for<HANDLER, ARGS...>,
                  "eliminate: no arm accepts this alternative -- add a case");
    using type = std::remove_cvref_t<std::invoke_result_t<HANDLER &, ARGS...>>;
};

//! \expos
template <class HANDLER, class... ARGS>
using arm_result_t = typename arm_result<HANDLER, ARGS...>::type;

/** The join of a deduplicated list of arm results: the one type when the
 * arms agree, otherwise the variant of them in the order given. */
//! \expos
template <class LIST>
struct join_of_arms;

template <class ONLY>
struct join_of_arms<type_list<ONLY>> {
    using type = ONLY;
};

template <class FIRST, class SECOND, class... REST>
struct join_of_arms<type_list<FIRST, SECOND, REST...>> {
    using type = std::variant<FIRST, SECOND, REST...>;
};

//! \expos
template <class LIST>
using join_of_arms_t = typename join_of_arms<LIST>::type;

/** The alternatives of a `std::variant`, as a type list. Undefined for any
 * other type, which is what makes `variant_specialization` a constraint
 * rather than a diagnostic. */
//! \expos
template <class VARIANT>
struct alternatives_of;

template <class... ALTERNATIVES>
struct alternatives_of<std::variant<ALTERNATIVES...>> {
    using type = type_list<ALTERNATIVES...>;
};

//! \expos
template <class VARIANT>
concept variant_specialization =
    requires { typename alternatives_of<std::remove_cvref_t<VARIANT>>::type; };

/** How an alternative of `VARIANT` is presented to an arm: with `VARIANT`'s
 * own value category, as `std::visit` presents it. Spelled through
 * `forward_like` rather than `get<ALTERNATIVE>`, which is ill-formed when
 * the alternative's type occurs more than once -- the case
 * `eliminate_indexed` exists for -- and which a library whose `get` deduces
 * its return type rejects even in an unevaluated operand. */
//! \expos
template <class VARIANT, class ALTERNATIVE>
using presented_t =
    decltype(std::forward_like<VARIANT>(std::declval<ALTERNATIVE &>()));

//! \expos
template <class VARIANT, class HANDLER, class ALTERNATIVES>
struct eliminate_result;

template <class VARIANT, class HANDLER, class... ALTERNATIVES>
struct eliminate_result<VARIANT, HANDLER, type_list<ALTERNATIVES...>> {
    using type = join_of_arms_t<typename list_dedupe<type_list<
        arm_result_t<HANDLER, presented_t<VARIANT, ALTERNATIVES>>...>>::type>;
};

//! \expos
template <class VARIANT, class HANDLER>
using eliminate_result_t = typename eliminate_result<
    VARIANT, HANDLER,
    typename alternatives_of<std::remove_cvref_t<VARIANT>>::type>::type;

//! \expos
template <class VARIANT, class HANDLER, class INDICES, class ALTERNATIVES>
struct eliminate_indexed_result;

template <class VARIANT, class HANDLER, std::size_t... INDICES,
          class... ALTERNATIVES>
struct eliminate_indexed_result<VARIANT, HANDLER,
                                std::index_sequence<INDICES...>,
                                type_list<ALTERNATIVES...>> {
    using type = join_of_arms_t<typename list_dedupe<
        type_list<arm_result_t<HANDLER, std::in_place_index_t<INDICES>,
                               presented_t<VARIANT, ALTERNATIVES>>...>>::type>;
};

//! \expos
template <class VARIANT, class HANDLER>
using eliminate_indexed_result_t = typename eliminate_indexed_result<
    VARIANT, HANDLER,
    std::make_index_sequence<std::variant_size_v<std::remove_cvref_t<VARIANT>>>,
    typename alternatives_of<std::remove_cvref_t<VARIANT>>::type>::type;

/** Places one arm's result into the joined type: directly when the join is
 * that type, and by its exact alternative otherwise, so that two result
 * types convertible to each other are never confused. */
template <class JOINED, class RESULT>
constexpr auto into_join(RESULT &&result) noexcept(
    std::is_same_v<std::remove_cvref_t<RESULT>, JOINED>
        ? std::is_nothrow_constructible_v<JOINED, RESULT>
        : std::is_nothrow_constructible_v<
              JOINED, std::in_place_type_t<std::remove_cvref_t<RESULT>>,
              RESULT>) -> JOINED {
    using Result = std::remove_cvref_t<RESULT>;
    if constexpr (std::is_same_v<Result, JOINED>) {
        return std::forward<RESULT>(result);
    } else {
        return JOINED(std::in_place_type<Result>, std::forward<RESULT>(result));
    }
}

/** Whether eliminating `VARIANT` with `HANDLER` is non-throwing: for every
 * alternative, invoking the arm on it as presented and placing the result
 * into the join. A variant that is valueless by exception holds no
 * alternative and is outside both eliminators' preconditions, so neither
 * specification accounts for it. */
//! \expos
template <class VARIANT, class HANDLER, class ALTERNATIVES>
struct eliminate_nothrow;

template <class VARIANT, class HANDLER, class... ALTERNATIVES>
struct eliminate_nothrow<VARIANT, HANDLER, type_list<ALTERNATIVES...>>
    : std::bool_constant<(
          noexcept(into_join<eliminate_result_t<VARIANT, HANDLER>>(std::invoke(
              std::declval<HANDLER &>(),
              std::declval<presented_t<VARIANT, ALTERNATIVES>>()))) &&
          ...)> {};

//! \expos
//! \remarks `$eliminate-nothrow-v$<VARIANT, HANDLER>` is `true` when, for
//! every alternative of `VARIANT`, invoking `HANDLER` on it as presented and
//! placing the result into the join are non-throwing.
template <class VARIANT, class HANDLER>
inline constexpr bool eliminate_nothrow_v = eliminate_nothrow<
    VARIANT, HANDLER,
    typename alternatives_of<std::remove_cvref_t<VARIANT>>::type>::value;

//! \expos
template <class VARIANT, class HANDLER, class INDICES, class ALTERNATIVES>
struct eliminate_indexed_nothrow;

template <class VARIANT, class HANDLER, std::size_t... INDICES,
          class... ALTERNATIVES>
struct eliminate_indexed_nothrow<VARIANT, HANDLER,
                                 std::index_sequence<INDICES...>,
                                 type_list<ALTERNATIVES...>>
    : std::bool_constant<(
          noexcept(into_join<eliminate_indexed_result_t<VARIANT, HANDLER>>(
              std::invoke(
                  std::declval<HANDLER &>(), std::in_place_index<INDICES>,
                  std::declval<presented_t<VARIANT, ALTERNATIVES>>()))) &&
          ...)> {};

//! \expos
//! \remarks `$eliminate-indexed-nothrow-v$<VARIANT, HANDLER>` is as
//! `$eliminate-nothrow-v$`, with each arm invoked with its index tag.
template <class VARIANT, class HANDLER>
inline constexpr bool eliminate_indexed_nothrow_v = eliminate_indexed_nothrow<
    VARIANT, HANDLER,
    std::make_index_sequence<std::variant_size_v<std::remove_cvref_t<VARIANT>>>,
    typename alternatives_of<std::remove_cvref_t<VARIANT>>::type>::value;

} // namespace detail

//! \constraints `remove_cvref_t<VARIANT>` is a specialization of `variant`,
//! and for every alternative `A` of it `handler` is invocable with `A`
//! presented in `variant`'s value category, with a result that is not
//! `void`.
//! \expects `variant.valueless_by_exception()` is `false`.
//! \effects Invokes `handler` with the alternative `variant` holds, presented
//! in `variant`'s value category.
//! \returns The result of that invocation, as the join of the arms: when
//! every arm's decayed result is one type, that type; otherwise `variant` of
//! the distinct decayed results in alternative order, holding the result in
//! the position of its type.
//! \remarks The join is a value and is read as a grade by nothing. Results of
//! one type from different arms share one position: the join records what
//! was produced, not which arm produced it; `eliminate_indexed` is for arms
//! that need the position. The exception specification is
//! `$eliminate-nothrow-v$<VARIANT, HANDLER>`: the arms' own, and the join's
//! construction from what they return.
template <class VARIANT, class HANDLER>
    requires detail::variant_specialization<VARIANT>
[[nodiscard]] constexpr auto eliminate(
    VARIANT &&variant,
    HANDLER &&handler) noexcept(detail::eliminate_nothrow_v<VARIANT, HANDLER>)
    -> detail::eliminate_result_t<VARIANT, HANDLER> {
    using Joined = detail::eliminate_result_t<VARIANT, HANDLER>;
    return std::visit(
        [&handler]<class ALTERNATIVE>(ALTERNATIVE &&alternative) -> Joined {
            return detail::into_join<Joined>(
                std::invoke(handler, std::forward<ALTERNATIVE>(alternative)));
        },
        std::forward<VARIANT>(variant));
}

//! \constraints As for `eliminate`, with `handler` invocable with
//! `in_place_index_t<I>` followed by the alternative, for each index `I`.
//! \expects `variant.valueless_by_exception()` is `false`.
//! \effects Invokes `handler` with `in_place_index<I>` and the alternative
//! `variant` holds at index `I`, presented in `variant`'s value category.
//! \returns As for `eliminate`.
//! \remarks The index tag is the injection into the sum: an arm that receives
//! it can tell two alternatives of one type apart, or use the position as a
//! template argument. The exception specification is
//! `$eliminate-indexed-nothrow-v$<VARIANT, HANDLER>`.
template <class VARIANT, class HANDLER>
    requires detail::variant_specialization<VARIANT>
[[nodiscard]] constexpr auto
eliminate_indexed(VARIANT &&variant, HANDLER &&handler) noexcept(
    detail::eliminate_indexed_nothrow_v<VARIANT, HANDLER>)
    -> detail::eliminate_indexed_result_t<VARIANT, HANDLER> {
    using Joined = detail::eliminate_indexed_result_t<VARIANT, HANDLER>;
    using Variant = std::remove_cvref_t<VARIANT>;
    return [&]<std::size_t... INDICES>(
               std::index_sequence<INDICES...>) -> Joined {
        Joined (*const arms[])(VARIANT &&, HANDLER &) = {
            [](VARIANT &&v, HANDLER &h) -> Joined {
                return detail::into_join<Joined>(
                    std::invoke(h, std::in_place_index<INDICES>,
                                std::get<INDICES>(std::forward<VARIANT>(v))));
            }...};
        return arms[variant.index()](std::forward<VARIANT>(variant), handler);
    }(std::make_index_sequence<std::variant_size_v<Variant>>{});
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_ELIMINATE_HPP
