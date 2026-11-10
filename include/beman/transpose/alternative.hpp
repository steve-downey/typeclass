// include/beman/transpose/alternative.hpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_ALTERNATIVE_HPP
#define BEMAN_TRANSPOSE_ALTERNATIVE_HPP

#include <beman/transpose/detail/canonical_set.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/eliminate.hpp>
#include <beman/transpose/grade.hpp>

#include <concepts>
#include <expected>
#include <optional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

// EVIDENCE, NOT PROPOSED WORDING. D3200R0 does not propose an Alternative
// abstraction -- deferred, not rejected, on the same footing as Monad. The
// disjunction's semantics were decided before the typeclass was written
// (docs/transpose/decisions.md#alternative-disjunction-semantics), and this
// header is the proof that the mechanism carries them: `alt(a, b, ...)`
// yields the leftmost operand that holds a value; the value side is the
// common type or the `variant` join of the operands' value types, in operand
// order, never read as a grade; the error side fails only when every operand
// fails and then carries every operand's evidence at the join of their
// grades; an operand that cannot fail makes the disjunction total; and
// `zero<C>()` is the identity where the context has one. It is unproposed
// only because nothing in the coordinated set calls `alt`.
//
// The basis is `alt`, n-ary. It is not derived from a binary `alt` by
// folding, because folding nests the value join -- `variant<variant<A, B>,
// C>` where `variant<A, B, C>` is meant -- and the instance is the only
// place that knows all of its operands at once. `zero` is a basis operation
// where the context's failure state carries no witness (`optional`), and
// absent where it needs one (`expected`): a context without `zero` is a
// disjunction without an identity, which is a semigroup, and the laws that
// mention `zero` are vacuous for it rather than false.

namespace detail {

/** The join of the value types a disjunction's operands contribute: the one
 * type when they agree, otherwise `std::variant` of the distinct types in
 * operand order, deduplicated by identity. The same join `eliminate` computes
 * for its arms, and like that one a value type and nothing more: no operation
 * reads it as a grade (docs/transpose/decisions.md#value-side-grading). */
//! \omit
template <class... VALUES>
using disjunction_join_t =
    join_of_arms_t<typename list_dedupe<type_list<VALUES...>>::type>;

/** Whether a contribution of type `REF` can be placed into `JOIN` the way
 * `into_join` places it: directly when it is the join, by its exact
 * alternative otherwise. */
//! \omit
template <class JOIN, class REF>
concept joins_into =
    (std::same_as<std::remove_cvref_t<REF>, JOIN> &&
     std::constructible_from<JOIN, REF>) ||
    (!std::same_as<std::remove_cvref_t<REF>, JOIN> &&
     std::constructible_from<
         JOIN, std::in_place_type_t<std::remove_cvref_t<REF>>, REF>);

/** Whether `T`, ignoring cv-qualification and reference, is a carrier this
 * library knows can fail. A value of such a carrier is never read as a bare
 * operand of a disjunction over another carrier: a disjunction is over one
 * carrier, and a mistaken mixture is a constraint that does not match rather
 * than a silent annihilation of the error channel. */
//! \omit
template <class T>
struct is_failure_carrier : std::false_type {};

template <class VALUE>
struct is_failure_carrier<std::optional<VALUE>> : std::true_type {};

template <class VALUE, class ERROR>
struct is_failure_carrier<std::expected<VALUE, ERROR>> : std::true_type {};

//! \omit
template <class T>
inline constexpr bool is_failure_carrier_v =
    is_failure_carrier<std::remove_cvref_t<T>>::value;

/** An operand of a disjunction that cannot fail: a bare value, which lifts to
 * the model bottom. Its presence makes the disjunction total. */
//! \omit
template <class OPERAND>
concept infallible_operand =
    !graded_context<std::remove_cvref_t<OPERAND>> &&
    !is_failure_carrier_v<OPERAND> && holdable_value<OPERAND>;

/** A bare operand's contribution, with the operand's own value category: an
 * rvalue is handed over, an lvalue is a `const` lvalue whether or not the
 * operand itself is `const`, as `contained_ref_t` reads a held value. */
//! \omit
template <class OPERAND>
using bare_ref_t =
    std::conditional_t<std::is_lvalue_reference_v<OPERAND>,
                       const std::remove_reference_t<OPERAND> &, OPERAND &&>;

//! \omit
template <class OPERAND>
constexpr auto bare_ref(OPERAND &&operand) noexcept -> bare_ref_t<OPERAND> {
    return static_cast<bare_ref_t<OPERAND>>(operand);
}

} // namespace detail

/** CRTP base for Alternative instances.
 * `Impl` must provide the n-ary `alt(first, rest...)`, and provides `zero()`
 * where the context's failure state carries no witness. Nothing is derived
 * from the basis today; the base exists so that the lookup, the object
 * concept and the operation objects have the shape every other typeclass
 * has, and so that an instance's `alt` is reached the same way its `bind` or
 * `invoke` is. Derived operations -- a guard, a fold of a range of contexts
 * under `alt` -- wait for the client that calls them.
 */
template <class Impl>
struct derive_alternative : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No alternative<T> specialization found. "
                  "Specialize beman::transpose::alternative<T> for your "
                  "type T and provide an alt(...) operation.");

    using Impl::alt;

    /** zero: the witness-free failure state, the identity of `alt`. A basis
     * operation of the instance that has one; the member is absent, not
     * ill-formed, for an instance that does not, so asking whether a context
     * has a `zero` is a constraint that does not match. */
    [[nodiscard]] constexpr auto
    zero(this auto &&self) noexcept(noexcept(impl_of(self).zero()))
        requires requires(const Impl &impl) { impl.zero(); }
    {
        return impl_of(self).zero();
    }

    // alt_with: explicit alternative object override, the shape `invoke_with`
    // and `bind_with` have.
    template <class ALTERNATIVE_MAP, class FIRST, class... REST>
    [[nodiscard]] constexpr auto alt_with(
        this auto &&, const ALTERNATIVE_MAP &alternative_map, FIRST &&first,
        REST &&...rest) noexcept(noexcept(alternative_map
                                              .alt(
                                                  std::forward<FIRST>(first),
                                                  std::forward<REST>(rest)...)))
        requires requires {
            alternative_map.alt(std::forward<FIRST>(first),
                                std::forward<REST>(rest)...);
        }
    {
        return alternative_map.alt(std::forward<FIRST>(first),
                                   std::forward<REST>(rest)...);
    }

  private:
    //! \omit
    template <class SELF>
    static constexpr decltype(auto) impl_of(SELF &&self) noexcept {
        return static_cast<impl_ref_t<Impl, SELF>>(self);
    }
};

/** Typeclass lookup variable for Alternative; specialize for each type. */
template <class T>
inline constexpr auto alternative = std::false_type{};

/** Restricted `Impl` concept for Alternative: satisfied when `IMPL` supplies
 * the one basis the CRTP base admits, the n-ary `alt`, probed at arity two
 * over the context. `zero` is not required: an instance without one is a
 * disjunction without an identity, and is admitted as such. */
template <class IMPL, class CONTEXT>
concept alternative_impl = requires(const IMPL &impl, const CONTEXT &context) {
    impl.alt(context, context);
};

/** Deep object concept for an Alternative object over `CONTEXT`: `alt` over
 * two contexts, `alt` with a bare last operand -- the annihilation form, whose
 * result is total -- and `alt_with`. `zero`, where the object has one, is the
 * context in its failure state, so its type is `CONTEXT`. */
template <class OBJ, class CONTEXT>
concept alternative_object =
    requires(const OBJ &obj, const CONTEXT &context) {
        obj.alt(context, context);
        obj.alt(context, std::declval<applicative_value_t<CONTEXT>>());
        obj.alt_with(obj, context, context);
    } &&
    (!requires(const OBJ &obj) { obj.zero(); } ||
     requires(const OBJ &obj) {
         { obj.zero() } -> std::same_as<CONTEXT>;
     });

// -- Operation objects --
//
// See functor.hpp for the shape and the motivation. `alt` deduces its context
// from the first operand, as `invoke` does; `zero<C>()` is spelled in
// apply.hpp beside `pure<C>` and `raise<C>`, because it builds a context and
// must name it (docs/transpose/decisions.md#failure-injection-spelling), and
// dispatches to the applicative object, which is where an instance's `zero`
// is declared first.

/** Operation object for Alternative's `alt`, the disjunction.
 *
 * The context keys the lookup and is deduced from the first operand; every
 * operand but the last is expected to share that context, and the last may
 * be a bare value, which makes the result total.
 */
struct alt_fn {
    /** The leftmost operand that holds a value, at the join of the operands'
     * value types.
     *
     * @tparam TC  The Alternative instance, from the lookup for the keying
     * operand. Not for callers to supply; pin with mode 2.
     */
    template <class FIRST, class... REST,
              const auto &TC = alternative<std::remove_cvref_t<FIRST>>>
        requires requires {
            TC.alt(std::declval<FIRST>(), std::declval<REST>()...);
        }
    [[nodiscard]] constexpr auto operator()(FIRST &&first, REST &&...rest) const
        noexcept(noexcept(TC.alt(std::forward<FIRST>(first),
                                 std::forward<REST>(rest)...))) {
        return TC.alt(std::forward<FIRST>(first), std::forward<REST>(rest)...);
    }
};

/** Disjunction: `alt(a, b, ...)`, the leftmost operand holding a value. */
inline constexpr alt_fn alt{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_ALTERNATIVE_HPP
