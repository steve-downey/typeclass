// include/beman/transpose/expected.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXPECTED_HPP
#define BEMAN_TRANSPOSE_EXPECTED_HPP

// Applicative and Monad instances for std::expected<T, E>.
//
// THREE CORES, MUTUALLY EXCLUSIVE BY CONSTRAINT.
//
// The same-error core handles operands that all declare this instance's error
// type, and deduces exactly what it always did: expected<T,E>, with no
// error_set in the carrier spelling. The same-error-plus-bare core keeps that
// spelling when ungraded operands participate. The graded core handles real
// mixing -- operands declaring different model grades -- and deduces their
// join. The constraints are complements, so the choice is never a matter of
// overload ranking (docs/transpose/decisions.md#grading-footprint).
//
// Every combination the graded core accepts was ILL-FORMED before grading:
// there was no instance that would take operands with differing error types.
// So grading claims only previously-ill-formed territory, and nothing that
// deduced a type before deduces a different one now. The golden tests in
// tests/beman/transpose/baseline_deduction.test.cpp are where that claim is
// mechanized rather than argued.
//
// The CARRIER SPELLING is lazy. `grade_of<std::expected<T,E>>` is the
// singleton grade `error_set<E>`, but operands that agree on an alternative
// carry that alternative verbatim, so unmixed pipelines never acquire a
// singleton error_set in their deduced type. A bare operand is lifted to the
// model bottom at a mixing point and leaves no trace in the carrier spelling.
// An error_set only appears where genuinely different alternatives met.
//
// Short-circuit semantics, first error wins, operands examined left to right.
// That order is observable and therefore part of the contract, per the
// Traversable rules in docs/transpose/CODING_RULES.md and the normative
// left-to-right order of docs/transpose/decisions.md#applicative-objects. Note
// that the grade is order-blind while the observed error is not: ∪ commutes, so
// operand order cannot change the deduced type, only which error you get back.
//
// THE UNIT. `std::expected<void, E>` is a context whose element is the unit,
// spelled `std::monostate` (docs/transpose/decisions.md#unit-payload-spelling).
// A callable returning `void` yields `std::expected<void, E>` from every
// object here, so the validation-pass shape `T -> expected<void, E>` composes
// and traverses like any other; the value a `void` context hands a callable
// is `std::monostate{}`, and the object over `std::expected<void, E>` maps
// `pure(std::monostate)` back to `std::expected<void, E>`, so that the
// element reading and `pure` round-trip. `void` itself stays `void`: nothing
// here holds it, and `holdable_value` is unchanged.
//
// This header stands in for <expected>, which in the proposal declares
// the instances itself (docs/transpose/decisions.md#instance-placement).
// Include it, not only <expected>, before the first use of a typeclass on a
// std::expected, in every translation unit that makes one: a use that precedes
// the instance is ill-formed, no diagnostic required. beman/transpose.hpp
// includes it.

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/error_set.hpp>
#include <beman/transpose/grade.hpp>
#include <beman/transpose/monad.hpp>

#include <expected>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace beman::transpose {

namespace detail {

/** True when EXPECTED is exactly std::expected<T, ERROR_TYPE> for some T.
 * Used to keep the bind basis SFINAE-friendly: a continuation returning a
 * different error type must fail the constraint cleanly rather than
 * hard-error inside the body when the error is rewrapped.
 */
//! \expos
template <class EXPECTED, class ERROR_TYPE>
inline constexpr bool is_expected_with_error_v = false;

template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr bool is_expected_with_error_v<
    std::expected<VALUE_TYPE, ERROR_TYPE>, ERROR_TYPE> = true;

/** True when EXPECTED is std::expected<void, E> for some E: the context
 * whose element is the unit. */
//! \expos
template <class EXPECTED>
inline constexpr bool is_void_expected_v = false;

template <class ERROR_TYPE>
inline constexpr bool is_void_expected_v<std::expected<void, ERROR_TYPE>> =
    true;

/** The value type an operand contributes; a bare operand contributes itself,
 * and a `void` expected contributes the unit.
 */
//! \expos
template <class CARRIER>
struct carrier_value {
    using type = CARRIER;
};

template <class VALUE, class ERROR>
struct carrier_value<std::expected<VALUE, ERROR>> {
    using type = VALUE;
};

template <class ERROR>
struct carrier_value<std::expected<void, ERROR>> {
    using type = std::monostate;
};

//! \expos
template <class CARRIER>
using carrier_value_t =
    typename carrier_value<std::remove_cvref_t<CARRIER>>::type;

/** The type the expected cores hand a callable for an operand of type
 * `OPERAND`: `contained_ref_t<OPERAND>` for a valued expected, and the unit
 * by value for a `void` one, which holds nothing a reference could name.
 *
 * A bare operand has no contained type, and the primary says so by having
 * no `type`. The cores name this alias in their trailing return types, and
 * a compiler that substitutes the return type before it checks the
 * requires-clause -- clang before 21, MSVC; GCC and later clang check the
 * constraint first, per CWG2369 -- reaches it for a core whose constraint
 * rejects the call. With an empty primary that is a substitution failure
 * and the core drops out, where dereferencing the bare operand would have
 * been a hard error.
 */
//! \expos
template <class OPERAND>
struct expected_contained {};

template <class OPERAND>
    requires is_expected_v<std::remove_cvref_t<OPERAND>> &&
             (!is_void_expected_v<std::remove_cvref_t<OPERAND>>)
struct expected_contained<OPERAND> {
    using type = contained_ref_t<OPERAND>;
};

template <class OPERAND>
    requires is_void_expected_v<std::remove_cvref_t<OPERAND>>
struct expected_contained<OPERAND> {
    using type = std::monostate;
};

//! \expos
template <class OPERAND>
using expected_contained_t = typename expected_contained<OPERAND>::type;

/** `forward_contained`, extended to the unit: a `void` expected yields
 * `std::monostate{}`. */
template <class OPERAND>
constexpr auto expected_forward_contained(OPERAND &&operand)
    -> expected_contained_t<OPERAND> {
    if constexpr (is_void_expected_v<std::remove_cvref_t<OPERAND>>) {
        return std::monostate{};
    } else {
        return forward_contained(std::forward<OPERAND>(operand));
    }
}

/** The expected that holds a callable's result: `std::expected<void, E>`
 * when the result is `void`, which no expected can hold as a value, and the
 * expected over the decayed result otherwise. `void` stays `void`; it is the
 * context's element that is read as the unit, never the carrier's parameter.
 */
//! \expos
template <class RESULT, class ERROR_TYPE>
struct expected_result {
    using type = std::expected<std::remove_cvref_t<RESULT>, ERROR_TYPE>;
};

template <class ERROR_TYPE>
struct expected_result<void, ERROR_TYPE> {
    using type = std::expected<void, ERROR_TYPE>;
};

//! \expos
template <class RESULT, class ERROR_TYPE>
using expected_result_t = typename expected_result<RESULT, ERROR_TYPE>::type;

/** What `pure(value)` yields on the object over
 * `std::expected<VALUE_TYPE, ERROR_TYPE>`. On the object over a `void`
 * expected, the unit is embedded as `std::expected<void, ERROR_TYPE>`, so
 * that `pure` inverts the element reading of that context; on every other
 * object, and for every other value, the expected over the decayed value.
 */
//! \expos
template <class VALUE_TYPE, class VALUE, class ERROR_TYPE>
struct expected_pure {
    using type = std::expected<std::remove_cvref_t<VALUE>, ERROR_TYPE>;
};

template <class VALUE, class ERROR_TYPE>
    requires std::is_same_v<std::remove_cvref_t<VALUE>, std::monostate>
struct expected_pure<void, VALUE, ERROR_TYPE> {
    using type = std::expected<void, ERROR_TYPE>;
};

//! \expos
template <class VALUE_TYPE, class VALUE, class ERROR_TYPE>
using expected_pure_t =
    typename expected_pure<VALUE_TYPE, VALUE, ERROR_TYPE>::type;

/** Constructs `RETURNED` from invoking `function` on `args`: when the result
 * is `void`, the call is made and a value-holding `RETURNED` is returned. */
template <class RETURNED, class FUNCTION, class... ARGS>
constexpr auto invoke_into_expected(FUNCTION &function, ARGS &&...args)
    -> RETURNED {
    if constexpr (std::is_void_v<typename RETURNED::value_type>) {
        std::invoke(function, std::forward<ARGS>(args)...);
        return RETURNED{};
    } else {
        return RETURNED{std::invoke(function, std::forward<ARGS>(args)...)};
    }
}

/** The argument `bind` hands its continuation for an element of type `A`:
 * a `const` lvalue, or the unit by value when `A` is `void`. */
//! \expos
template <class A>
struct bind_argument {
    using type = const A &;
};

template <>
struct bind_argument<void> {
    using type = std::monostate;
};

//! \expos
template <class A>
using bind_argument_t = typename bind_argument<A>::type;

//! \expos
template <class F, class A>
using bind_result_t =
    std::remove_cvref_t<std::invoke_result_t<F, bind_argument_t<A>>>;

/** The carrier `raise` yields on the object over
 * `std::expected<VALUE_TYPE, ERROR_TYPE>` for a witness of type `ERROR`: the
 * declared spelling verbatim when `ERROR` is `ERROR_TYPE` itself, which is the
 * lazy carrier spelling the cores keep, and otherwise the carrier re-indexed
 * at the join of the object's grade with the grade `ERROR` alone would carry.
 * `ERROR` may itself be an error set, which contributes its members.
 */
//! \expos
template <class VALUE_TYPE, class ERROR_TYPE, class ERROR>
struct raise_result {
    using type = rebind_grade_t<
        std::expected<VALUE_TYPE, ERROR_TYPE>,
        grade_join_t<grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     grade_of_t<std::expected<VALUE_TYPE, ERROR>>>>;
};

template <class VALUE_TYPE, class ERROR_TYPE>
struct raise_result<VALUE_TYPE, ERROR_TYPE, ERROR_TYPE> {
    using type = std::expected<VALUE_TYPE, ERROR_TYPE>;
};

//! \expos
template <class VALUE_TYPE, class ERROR_TYPE, class ERROR>
using raise_result_t = typename raise_result<VALUE_TYPE, ERROR_TYPE,
                                             std::remove_cvref_t<ERROR>>::type;

/** Whether the object over `std::expected<VALUE_TYPE, ERROR_TYPE>` can raise
 * an `ERROR`: the result's error alternative is constructible from it. */
//! \expos
template <class VALUE_TYPE, class ERROR_TYPE, class ERROR>
concept raisable = std::constructible_from<
    typename raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR>::error_type, ERROR>;

/** Builds the raised carrier. Shared by both applicative objects: raising is
 * the same for each, as `pure` is. */
template <class VALUE_TYPE, class ERROR_TYPE, class ERROR>
constexpr auto raise_into(ERROR &&error)
    -> raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR> {
    using Result = raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR>;
    return Result{std::unexpect,
                  typename Result::error_type(std::forward<ERROR>(error))};
}

/** The element `bind` hands on: the held value, or the unit for a `void`
 * expected. */
template <class A, class ERROR_TYPE>
constexpr auto bound_argument(const std::expected<A, ERROR_TYPE> &ma)
    -> bind_argument_t<A> {
    if constexpr (std::is_void_v<A>) {
        return std::monostate{};
    } else {
        return *ma;
    }
}

/** True when every operand is an expected declaring exactly ERROR_TYPE, which
 * is precisely the case the ungraded core already handles. The graded core
 * negates this, so the two are mutually exclusive by constraint rather than
 * by ranking. */
//! \expos
template <class ERROR_TYPE, class... CARRIERS>
inline constexpr bool all_declare_v =
    (is_expected_with_error_v<std::remove_cvref_t<CARRIERS>, ERROR_TYPE> &&
     ...);

/** True when every operand either declares exactly ERROR_TYPE or is truly
 * ungraded. This is not a mixing point: lazy join says ungraded values
 * contribute the model bottom and leave no trace. Foreign-model carriers are
 * not bare, even when they are not std::expected.
 */
//! \expos
template <class ERROR_TYPE, class CARRIER>
inline constexpr bool declares_or_bare_v =
    (!graded_context<std::remove_cvref_t<CARRIER>>) ||
    is_expected_with_error_v<std::remove_cvref_t<CARRIER>, ERROR_TYPE>;

//! \expos
template <class ERROR_TYPE, class... CARRIERS>
inline constexpr bool all_declare_or_bare_v =
    (declares_or_bare_v<ERROR_TYPE, CARRIERS> && ...);

/** The engaged value of an operand; a bare operand is already the value. */
template <class CARRIER>
constexpr auto operand_value(const CARRIER &operand) -> decltype(auto) {
    if constexpr (is_void_expected_v<std::remove_cvref_t<CARRIER>>) {
        return std::monostate{};
    } else if constexpr (is_expected_v<std::remove_cvref_t<CARRIER>>) {
        return *operand;
    } else {
        return (operand);
    }
}

/** Whether an operand currently carries an error. A bare value never does:
 * ∅ has no failure alternative to be in. */
template <class CARRIER>
constexpr auto operand_failed(const CARRIER &operand) -> bool {
    if constexpr (is_expected_v<std::remove_cvref_t<CARRIER>>) {
        return !operand.has_value();
    } else {
        return false;
    }
}

} // namespace detail

/** The element of `std::expected<void, ERROR>` is the unit. The framework's
 * default reads `value_type`, which is `void` here and is not a value: a
 * context's element must be something a callable can be handed and `pure`
 * can take back. See docs/transpose/decisions.md#unit-payload-spelling.
 */
//! \remarks The element type of `std::expected<void, ERROR>` is
//! `std::monostate`.
template <class ERROR>
struct applicative_value<std::expected<void, ERROR>> {
    using type = std::monostate;
};

// Namespace-scope spellings of the traits that appear in the declarations
// below. They forward to the `detail` definitions and add nothing; they exist
// so that a declaration the specification shows does not carry an
// implementation namespace qualifier into the wording. Each is omitted from
// the wording, which states the requirement it encodes in prose.

/// Applicative instance for std::expected<VALUE_TYPE, ERROR_TYPE>.
/// The n-ary invoke core: if every operand holds a value, one call; otherwise
/// the leftmost error propagates. The trailing return type keeps invoke
/// SFINAE-friendly, so an operand carrying a different error type fails
/// deduction cleanly instead of hard-erroring.
//! \expos
template <class VALUE_TYPE, class ERROR_TYPE>
struct expected_applicative_impl {
    // \ref{transpose.expected.applicative}, applicative instance for expected
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE>;

    template <class ERROR>
        requires detail::raisable<VALUE_TYPE, ERROR_TYPE, ERROR>
    auto raise(this auto &&, ERROR &&error)
        -> detail::raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR>;

    /** N-ary core: all operands share ERROR_TYPE; leftmost error wins.
     *
     * Operands are deduced through forwarding references rather than matched
     * as `const expected<FIRST, ERROR_TYPE>&`, so that a caller handing over
     * an rvalue has its held value moved from. `all_declare_v` restates in a
     * constraint the shape the old signature matched by pattern, and remains
     * exactly complementary to the two cores below.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                detail::all_declare_v<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto &&, FUNCTION &&function, CARRIERS &&...operands)
        -> detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 detail::expected_contained_t<CARRIERS>...>,
            ERROR_TYPE>;

    /** N-ary core: expected operands share ERROR_TYPE, with any number of
     * bare operands. This is not a mixing point: the bare operands lift to
     * the model bottom and leave no trace in the deduced type.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>) &&
                detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 const detail::carrier_value_t<CARRIERS> &...>,
            ERROR_TYPE>;

    /** Graded n-ary core: the mixing point.
     *
     * Reached exactly when the operands do NOT all declare this instance's
     * error type -- the ungraded core above handles that case, and the two
     * constraints are complementary, so the choice is never a matter of
     * overload ranking (docs/transpose/decisions.md#grading-footprint).
     *
     * The result carries the join of the operands' semantic grades. Carrier
     * spelling remains lazy: identical alternatives are carried verbatim,
     * only genuinely different ones become an error_set, and bare operands
     * lift to the model bottom without leaving a trace in the deduced type.
     *
     * This is the ONLY behavioural change grading makes to existing code, and
     * it makes previously-ill-formed combinations well-formed. Nothing that
     * deduced a type before deduces a different one now.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
                (detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     CARRIERS> &&
                 ...)
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            detail::expected_result_t<
                std::invoke_result_t<
                    FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>,
                ERROR_TYPE>,
            CARRIERS...>;
};

//! \expos
template <class VALUE_TYPE, class ERROR_TYPE>
struct expected_applicative_map
    : derive_applicative<expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>> {
    using expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke;
    using expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::pure;
    using expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::raise;
};

/** Applicative instance for `std::expected<VALUE_TYPE, ERROR_TYPE>`. */
template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr auto applicative<std::expected<VALUE_TYPE, ERROR_TYPE>> =
    expected_applicative_map<VALUE_TYPE, ERROR_TYPE>{};

// -- The accumulating applicative instance ---------------------------------
//
// The second NTTP-pinned object over the SAME carrier and grade algebra as
// expected_applicative_map above, per
// docs/transpose/decisions.md#applicative-objects. Stage accumulating-object of
// docs/transpose/transpose-grading-plan.md.
//
// Where the short-circuit object above keeps only the LEFTMOST failing
// operand's witness, this object COMBINES every failing operand's witness
// through the grade model's evidence-combine hook. Same-type collisions keep
// the leftmost witness; model evidence with multiple slots can preserve
// failures of genuinely different types. That is the whole difference:
// examining every operand instead of stopping at the first.
//
// Structurally this mirrors expected_applicative_impl's ungraded/graded split
// exactly -- same two constraints, same complementary coverage, same
// bare-operand promotion. Only the failure-collecting step differs.

namespace detail {

/** Folds model evidence over every failing operand's error, left to right.
 * `EXTRACT` maps an operand to `std::optional<ERROR>` -- present exactly
 * when that operand failed -- so this one loop serves both the ungraded core
 * and the graded core.
 */
template <class ERROR, class EXTRACT, class... OPERANDS>
constexpr auto accumulate_failures(EXTRACT &&extract,
                                   const OPERANDS &...operands)
    -> std::optional<ERROR> {
    std::optional<ERROR> accumulated;
    auto fold_in = [&accumulated, &extract](const auto &operand) {
        auto this_one = extract(operand);
        if (!this_one.has_value()) {
            return;
        }
        if (accumulated.has_value()) {
            accumulated = combine_grade_evidence(*accumulated, *this_one);
        } else {
            accumulated = std::move(this_one);
        }
    };
    (fold_in(operands), ...);
    return accumulated;
}

} // namespace detail

/** Accumulating applicative instance for std::expected<VALUE_TYPE,
 * ERROR_TYPE>. Same carrier as expected_applicative_impl; see the section
 * comment above for how the two differ.
 */
//! \expos
template <class VALUE_TYPE, class ERROR_TYPE>
struct accumulating_expected_applicative_impl {
    // \ref{transpose.expected.accumulating}, accumulating instance
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE>;

    template <class ERROR>
        requires detail::raisable<VALUE_TYPE, ERROR_TYPE, ERROR>
    auto raise(this auto &&, ERROR &&error)
        -> detail::raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR>;

    /** N-ary core: all operands share ERROR_TYPE. When ERROR_TYPE is itself
     * a witnessed subset (the traverse case: one function, one declared
     * error, possibly several members), failures of distinct members all
     * survive; otherwise there is one slot and this degrades to
     * leftmost-wins, same as the short-circuit object.
     *
     * Operands are deduced through forwarding references, as
     * expected_applicative_impl's counterpart core explains.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                detail::all_declare_v<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto &&, FUNCTION &&function, CARRIERS &&...operands)
        -> detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 detail::expected_contained_t<CARRIERS>...>,
            ERROR_TYPE>;

    /** N-ary core: expected operands share ERROR_TYPE, with any number of
     * bare operands. This is not a mixing point, so the result carries the
     * declared error alternative verbatim.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>) &&
                detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 const detail::carrier_value_t<CARRIERS> &...>,
            ERROR_TYPE>;

    /** Graded n-ary core: the mixing point, mirroring
     * expected_applicative_impl's, but accumulating every failing operand's
     * witness into the joined grade instead of keeping only the first.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
                (detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     CARRIERS> &&
                 ...)
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            detail::expected_result_t<
                std::invoke_result_t<
                    FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>,
                ERROR_TYPE>,
            CARRIERS...>;
};

//! \expos
template <class VALUE_TYPE, class ERROR_TYPE>
struct accumulating_expected_applicative_map
    : derive_applicative<
          accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>> {
    using accumulating_expected_applicative_impl<VALUE_TYPE,
                                                 ERROR_TYPE>::invoke;
    using accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::pure;
    using accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::raise;

    /// Bind is refused, not merely absent: sequencing needs a value from
    /// the first computation to feed the continuation, and when that
    /// computation failed there is none -- a value-flow obstruction, not
    /// something grade bookkeeping can fix. The framework never derives bind
    /// for this object.
    ///
    /// The refusal is a deleted function, not a `static_assert` in a body.
    /// A call to a deleted function is an invalid expression in the
    /// immediate context, so asking whether `bind` exists -- as
    /// `monad_object` does, and as an algorithm dispatching between a
    /// sequential and an applicative route does -- answers false. A
    /// `static_assert` in a deduced body fires when the probe instantiates
    /// the body, which makes the question a hard error. The message on the
    /// deletion is C++26 (P2573); `BEMAN_TRANSPOSE_DELETE_WITH_REASON`
    /// drops it where it is unavailable, and a call is still rejected, as a
    /// call to a deleted function.
    //! \omit
    template <class... ARGS>
    void bind(ARGS &&...) const = BEMAN_TRANSPOSE_DELETE_WITH_REASON(
        "The accumulating applicative object has no bind: sequencing "
        "needs a value from the first computation to feed the "
        "continuation, and there is none when that computation failed "
        "-- a value-flow obstruction, not something grade bookkeeping "
        "can fix. Use the short-circuit (monad-derived) applicative "
        "object (applicative) for sequential composition. "
        "See docs/transpose/decisions.md#applicative-objects.");
};

/** Accumulating applicative instance for `std::expected<VALUE_TYPE,
 * ERROR_TYPE>`. */
template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr auto
    accumulating_applicative<std::expected<VALUE_TYPE, ERROR_TYPE>> =
        accumulating_expected_applicative_map<VALUE_TYPE, ERROR_TYPE>{};

/** Monad instance for std::expected<VALUE_TYPE, ERROR_TYPE>.
 * bind short-circuits on the error alternative. The continuation must return
 * an expected carrying the same ERROR_TYPE; the constraint says so directly
 * rather than letting the mismatch surface as a rewrapping failure in the
 * body.
 */
//! \omit
template <class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedMonadImpl {
    using element_type =
        detail::carrier_value_t<std::expected<VALUE_TYPE, ERROR_TYPE>>;

    //! \omit
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE>;

    //! \omit
    template <class A, class F>
    auto bind(this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
        -> detail::bind_result_t<F, A>
        requires detail::is_expected_with_error_v<detail::bind_result_t<F, A>,
                                                  ERROR_TYPE>;

    /** Graded bind: sequencing joins grades.
     *
     * Reached when the continuation returns an expected declaring a DIFFERENT
     * error alternative; the same-alternative case is the ungraded bind
     * above, and the constraints are complementary. The result carries the
     * join, and each side's error is widened into it by the alternative's own
     * conversion.
     */
    //! \omit
    template <class A, class F>
        requires is_expected_v<detail::bind_result_t<F, A>> &&
                 (!detail::is_expected_with_error_v<detail::bind_result_t<F, A>,
                                                    ERROR_TYPE>) &&
                 detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     std::expected<A, ERROR_TYPE>> &&
                 detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     detail::bind_result_t<F, A>>
    auto bind(this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
        -> detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<typename detail::bind_result_t<F, A>::value_type,
                          ERROR_TYPE>,
            std::expected<A, ERROR_TYPE>, detail::bind_result_t<F, A>>;
};

//! \omit
template <class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedMonadMap
    : derive_monad<ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>> {
    using ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::bind;
    using ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::pure;
};

/** Monad instance for `std::expected<VALUE_TYPE, ERROR_TYPE>`. */
template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr auto monad<std::expected<VALUE_TYPE, ERROR_TYPE>> =
    ExpectedMonadMap<VALUE_TYPE, ERROR_TYPE>{};

// \rSec3[transpose.expected.applicative]{Applicative instance for expected}

//! \returns An `expected` holding `value`.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class VALUE>
auto expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::pure(this auto &&,
                                                             VALUE &&value)
    -> detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE> {
    using Returned = detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE>;
    if constexpr (std::is_void_v<typename Returned::value_type>) {
        return Returned{};
    } else {
        return Returned{std::forward<VALUE>(value)};
    }
}

//! \constraints The error alternative of the result is constructible from
//! `error`.
//! \returns An `expected` holding `error` as its error, at the grade that is
//! the join of this object's grade with the grade `error`'s type alone would
//! carry. When that type is `ERROR_TYPE` itself the result is
//! `expected<VALUE_TYPE, ERROR_TYPE>`, carried verbatim.
//! \remarks This is the dual of `pure`: the generic entry into the failure
//! state, so that generic graded code can fail without naming the carrier.
//! Carrier spelling stays lazy, as for the cores: raising the declared
//! alternative leaves the spelling alone, and only a genuinely different
//! alternative widens it into an error set. `error` may itself be an error
//! set, whose members join.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class ERROR>
    requires detail::raisable<VALUE_TYPE, ERROR_TYPE, ERROR>
auto expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::raise(this auto &&,
                                                              ERROR &&error)
    -> detail::raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR> {
    return detail::raise_into<VALUE_TYPE, ERROR_TYPE>(
        std::forward<ERROR>(error));
}

//! \returns If every operand holds a value, an `expected` holding the result
//! of invoking `function` with those values, in the order written; otherwise
//! an `expected` holding the error of the first operand, in that same order,
//! that does not hold a value.
//! \remarks `function` is invoked at most once. Composition short-circuits:
//! only the first error is observed. Each operand's held value is passed on
//! with that operand's own value category, so a caller that hands over an
//! rvalue operand has its value moved from rather than copied.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            detail::all_declare_v<ERROR_TYPE, CARRIERS...>
auto expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke(
    this auto &&, FUNCTION &&function, CARRIERS &&...operands)
    -> detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             detail::expected_contained_t<CARRIERS>...>,
        ERROR_TYPE> {
    using Returned = detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             detail::expected_contained_t<CARRIERS>...>,
        ERROR_TYPE>;

    std::optional<ERROR_TYPE> failure;
    auto record_first_failure = [&failure](const auto &operand) {
        if (!failure.has_value() && !operand.has_value()) {
            failure = operand.error();
        }
    };
    (record_first_failure(operands), ...);

    if (failure.has_value()) {
        return Returned{std::unexpect, std::move(*failure)};
    }
    return detail::invoke_into_expected<Returned>(
        function, detail::expected_forward_contained(
                      std::forward<CARRIERS>(operands))...);
}

//! \constraints At least one operand is an `expected`; the operands do not
//! all declare `ERROR_TYPE`; and every operand either declares `ERROR_TYPE`
//! or is a bare value.
//! \returns As for the preceding overload, treating a bare operand as
//! holding itself.
//! \remarks This is not a mixing point. A bare operand is ungraded, lifts to
//! the model bottom, and leaves no trace in the deduced type.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
            (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>) &&
            detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
auto expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke(
    this auto &&, FUNCTION &&function, const CARRIERS &...operands)
    -> detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             const detail::carrier_value_t<CARRIERS> &...>,
        ERROR_TYPE> {
    using Returned = detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             const detail::carrier_value_t<CARRIERS> &...>,
        ERROR_TYPE>;

    std::optional<ERROR_TYPE> failure;
    auto record_first_failure = [&failure](const auto &operand) {
        if constexpr (is_expected_v<std::remove_cvref_t<decltype(operand)>>) {
            if (!failure.has_value() && !operand.has_value()) {
                failure = operand.error();
            }
        }
    };
    (record_first_failure(operands), ...);

    if (failure.has_value()) {
        return Returned{std::unexpect, std::move(*failure)};
    }
    return detail::invoke_into_expected<Returned>(
        function, detail::operand_value(operands)...);
}

//! \constraints At least one operand is an `expected`; the operands do not
//! all declare `ERROR_TYPE` or a bare value; and every operand's grade
//! belongs to this instance's grade model.
//! \returns As for the preceding overloads, at the grade that is the join of
//! the operands' grades.
//! \remarks This is the mixing point, reached exactly when the preceding
//! overload's constraint does not hold; the two are complementary, so the
//! choice is never a matter of overload ranking. Carrier spelling stays
//! lazy: operands that declare the same error alternative keep that
//! spelling, only genuinely different ones join into an error set, and a
//! bare operand lifts to the model bottom without trace. Combinations that
//! were previously ill-formed become well-formed; nothing that deduced a
//! type before deduces a different one now.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
            (!detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
            (detail::mixes_with_model<
                 grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>, CARRIERS> &&
             ...)
auto expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke(
    this auto &&, FUNCTION &&function, const CARRIERS &...operands)
    -> detail::mixed_result_t<
        grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
        detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 const detail::carrier_value_t<CARRIERS> &...>,
            ERROR_TYPE>,
        CARRIERS...> {
    using Returned = detail::mixed_result_t<
        grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
        detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 const detail::carrier_value_t<CARRIERS> &...>,
            ERROR_TYPE>,
        CARRIERS...>;
    using Joined = typename Returned::error_type;

    std::optional<Joined> failure;
    auto record_first_failure = [&failure](const auto &operand) {
        if (!failure.has_value() && detail::operand_failed(operand)) {
            if constexpr (is_expected_v<
                              std::remove_cvref_t<decltype(operand)>>) {
                failure.emplace(Joined(operand.error()));
            }
        }
    };
    (record_first_failure(operands), ...);

    if (failure.has_value()) {
        return Returned{std::unexpect, std::move(*failure)};
    }
    return detail::invoke_into_expected<Returned>(
        function, detail::operand_value(operands)...);
}

// \rSec3[transpose.expected.accumulating]{Accumulating instance}

//! \returns An `expected` holding `value`.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class VALUE>
auto accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::pure(
    this auto &&, VALUE &&value)
    -> detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE> {
    return applicative<std::expected<VALUE_TYPE, ERROR_TYPE>>.pure(
        std::forward<VALUE>(value));
}

//! \constraints As for the short-circuit object's `raise`.
//! \returns As for the short-circuit object's `raise`: the two objects share
//! the carrier, so they share the entry into its failure state.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class ERROR>
    requires detail::raisable<VALUE_TYPE, ERROR_TYPE, ERROR>
auto accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::raise(
    this auto &&, ERROR &&error)
    -> detail::raise_result_t<VALUE_TYPE, ERROR_TYPE, ERROR> {
    return detail::raise_into<VALUE_TYPE, ERROR_TYPE>(
        std::forward<ERROR>(error));
}

//! \returns If every operand holds a value, an `expected` holding the result
//! of invoking `function` with those values, in the order written; otherwise
//! an `expected` holding the combination of every operand's error, combined
//! in that same order.
//! \remarks `function` is invoked at most once. Composition accumulates:
//! every error is observed, not only the first. Each operand's held value is
//! passed on with that operand's own value category, so a caller that hands
//! over an rvalue operand has its value moved from rather than copied.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            detail::all_declare_v<ERROR_TYPE, CARRIERS...>
auto accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke(
    this auto &&, FUNCTION &&function, CARRIERS &&...operands)
    -> detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             detail::expected_contained_t<CARRIERS>...>,
        ERROR_TYPE> {
    using Returned = detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             detail::expected_contained_t<CARRIERS>...>,
        ERROR_TYPE>;

    auto extract_failure =
        [](const auto &operand) -> std::optional<ERROR_TYPE> {
        if (operand.has_value()) {
            return std::nullopt;
        }
        return operand.error();
    };
    auto accumulated =
        detail::accumulate_failures<ERROR_TYPE>(extract_failure, operands...);

    if (accumulated.has_value()) {
        return Returned{std::unexpect, std::move(*accumulated)};
    }
    return detail::invoke_into_expected<Returned>(
        function, detail::expected_forward_contained(
                      std::forward<CARRIERS>(operands))...);
}

//! \constraints At least one operand is an `expected`; the operands do not
//! all declare `ERROR_TYPE`; and every operand either declares `ERROR_TYPE`
//! or is a bare value.
//! \returns As for the preceding overload, treating a bare operand as
//! holding itself.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
            (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>) &&
            detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
auto accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke(
    this auto &&, FUNCTION &&function, const CARRIERS &...operands)
    -> detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             const detail::carrier_value_t<CARRIERS> &...>,
        ERROR_TYPE> {
    using Returned = detail::expected_result_t<
        std::invoke_result_t<FUNCTION &,
                             const detail::carrier_value_t<CARRIERS> &...>,
        ERROR_TYPE>;

    auto extract_failure =
        [](const auto &operand) -> std::optional<ERROR_TYPE> {
        if constexpr (is_expected_v<std::remove_cvref_t<decltype(operand)>>) {
            if (!operand.has_value()) {
                return operand.error();
            }
        }
        return std::nullopt;
    };
    auto accumulated =
        detail::accumulate_failures<ERROR_TYPE>(extract_failure, operands...);

    if (accumulated.has_value()) {
        return Returned{std::unexpect, std::move(*accumulated)};
    }
    return detail::invoke_into_expected<Returned>(
        function, detail::operand_value(operands)...);
}

//! \constraints At least one operand is an `expected`; the operands do not
//! all declare `ERROR_TYPE` or a bare value; and every operand's grade
//! belongs to this instance's grade model.
//! \returns As for the preceding overloads, at the grade that is the join of
//! the operands' grades, holding the combination of every failing operand's
//! evidence.
//! \remarks Errors are combined in the order the operands are written.
template <class VALUE_TYPE, class ERROR_TYPE>
template <class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<std::remove_cvref_t<CARRIERS>> || ...) &&
            (!detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
            (detail::mixes_with_model<
                 grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>, CARRIERS> &&
             ...)
auto accumulating_expected_applicative_impl<VALUE_TYPE, ERROR_TYPE>::invoke(
    this auto &&, FUNCTION &&function, const CARRIERS &...operands)
    -> detail::mixed_result_t<
        grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
        detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 const detail::carrier_value_t<CARRIERS> &...>,
            ERROR_TYPE>,
        CARRIERS...> {
    using Returned = detail::mixed_result_t<
        grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
        detail::expected_result_t<
            std::invoke_result_t<FUNCTION &,
                                 const detail::carrier_value_t<CARRIERS> &...>,
            ERROR_TYPE>,
        CARRIERS...>;
    using Joined = typename Returned::error_type;

    auto extract_failure = [](const auto &operand) -> std::optional<Joined> {
        if constexpr (is_expected_v<std::remove_cvref_t<decltype(operand)>>) {
            if (detail::operand_failed(operand)) {
                return Joined(operand.error());
            }
        }
        return std::nullopt;
    };
    auto accumulated =
        detail::accumulate_failures<Joined>(extract_failure, operands...);

    if (accumulated.has_value()) {
        return Returned{std::unexpect, std::move(*accumulated)};
    }
    return detail::invoke_into_expected<Returned>(
        function, detail::operand_value(operands)...);
}

//! \omit
template <class VALUE_TYPE, class ERROR_TYPE>
template <class VALUE>
auto ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::pure(this auto &&,
                                                     VALUE &&value)
    -> detail::expected_pure_t<VALUE_TYPE, VALUE, ERROR_TYPE> {
    return applicative<std::expected<VALUE_TYPE, ERROR_TYPE>>.pure(
        std::forward<VALUE>(value));
}

//! \omit
template <class VALUE_TYPE, class ERROR_TYPE>
template <class A, class F>
auto ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::bind(
    this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
    -> detail::bind_result_t<F, A>
    requires detail::is_expected_with_error_v<detail::bind_result_t<F, A>,
                                              ERROR_TYPE>
{
    using Result = detail::bind_result_t<F, A>;
    if (!ma.has_value()) {
        return Result{std::unexpect, ma.error()};
    }
    return Result{std::invoke(std::forward<F>(f), detail::bound_argument(ma))};
}

//! \omit
template <class VALUE_TYPE, class ERROR_TYPE>
template <class A, class F>
    requires is_expected_v<detail::bind_result_t<F, A>> &&
             (!detail::is_expected_with_error_v<detail::bind_result_t<F, A>,
                                                ERROR_TYPE>) &&
             detail::mixes_with_model<
                 grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                 std::expected<A, ERROR_TYPE>> &&
             detail::mixes_with_model<
                 grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                 detail::bind_result_t<F, A>>
auto ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::bind(
    this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
    -> detail::mixed_result_t<
        grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
        std::expected<typename detail::bind_result_t<F, A>::value_type,
                      ERROR_TYPE>,
        std::expected<A, ERROR_TYPE>, detail::bind_result_t<F, A>> {
    using Continuation = detail::bind_result_t<F, A>;
    using Returned = detail::mixed_result_t<
        grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
        std::expected<typename Continuation::value_type, ERROR_TYPE>,
        std::expected<A, ERROR_TYPE>, Continuation>;
    using Joined = typename Returned::error_type;

    if (!ma.has_value()) {
        return Returned{std::unexpect, Joined(ma.error())};
    }
    auto produced = std::invoke(std::forward<F>(f), detail::bound_argument(ma));
    if (!produced.has_value()) {
        return Returned{std::unexpect, Joined(produced.error())};
    }
    if constexpr (std::is_void_v<typename Continuation::value_type>) {
        return Returned{};
    } else {
        return Returned{std::move(*produced)};
    }
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_EXPECTED_HPP
