// include/beman/transpose/apply.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_APPLY_HPP
#define BEMAN_TRANSPOSE_APPLY_HPP

#include <beman/transpose/detail/applicative_derivation.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/grade.hpp>

#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose {

/// Namespace-scope spelling of the evaluator named by `ap`'s constraint. A
/// second object of the `detail` type, not a reference to it: the type is
/// empty and stateless, so the two are interchangeable, and a reference's
/// declared type does not currently survive `\seebelow` masking
/// (steve-downey/specgen#33). It exists so that a declaration the
/// specification shows does not carry an implementation namespace qualifier
/// into the wording.
//! \seebelow
//! \remarks `applicative_eval` applies a callable to one argument. It is the
//! evaluator that recovers one-step application from the n-ary core:
//! `ap(f, x)` is `invoke(applicative_eval, f, x)`. Naming it is what makes
//! `ap`'s second alternative expressible, and that alternative is satisfied
//! only where the context can hold a callable.
inline constexpr detail::applicative_eval_t applicative_eval{};

namespace detail {

// A generic lambda-expression is a distinct, unrelated type at every
// occurrence in the source, so it cannot be the shared expression a
// disjunctive requires-clause needs at both the in-class declaration and
// the out-of-line definition (they must be spelled with the same tokens
// naming the same entity). discard_first_eval_t and discard_second_eval_t
// give `discard_first` and `discard_second` a named, stable callable for
// that purpose -- the same role applicative_eval_t plays for `ap`.

//! \omit
struct discard_first_eval_t {
    template <class FIRST, class SECOND>
    constexpr auto operator()(const FIRST &, SECOND &&second) const noexcept
        -> SECOND && {
        return std::forward<SECOND>(second);
    }
};

//! \omit
struct discard_second_eval_t {
    template <class FIRST, class SECOND>
    constexpr auto operator()(FIRST &&first, const SECOND &) const noexcept
        -> FIRST && {
        return std::forward<FIRST>(first);
    }
};

} // namespace detail

/// Namespace-scope spelling of the evaluator named by `discard_first`'s
/// constraint, for the same reason `applicative_eval` is named rather than
/// spelled inline: a lambda-expression cannot be repeated identically
/// between an in-class declaration and its out-of-line definition.
//! \seebelow
//! \remarks `discard_first_eval` discards its first argument and returns
//! its second. It is the evaluator `discard_first`'s second alternative
//! probes, and that alternative is satisfied exactly when `discard_first`'s
//! own `invoke`-based derivation would be.
inline constexpr detail::discard_first_eval_t discard_first_eval{};

/// Namespace-scope spelling of the evaluator named by `discard_second`'s
/// constraint; see `discard_first_eval` above.
//! \seebelow
//! \remarks `discard_second_eval` discards its second argument and returns
//! its first. It is the evaluator `discard_second`'s second alternative
//! probes, and that alternative is satisfied exactly when `discard_second`'s
//! own `invoke`-based derivation would be.
inline constexpr detail::discard_second_eval_t discard_second_eval{};

namespace detail {

// The exception specifications of the derived operations
// (docs/transpose/decisions.md#operation-noexcept). Each derived operation
// takes one of two routes -- Impl's own operation where Impl has one that
// accepts the arguments, the derivation otherwise -- and its exception
// specification is the specification of the route taken. Each trait below
// probes Impl through impl_ref_t<IMPL, SELF>, the reference the body
// addresses Impl by, with the arguments in the categories the body forwards
// them in, so the route the specification names is the route the body takes.
// The derived route promises only what the calls it makes promise.

//! \expos
//! \remarks `$applicative-invoke-nothrow$<IMPL, SELF, FUNCTION, ARGUMENTS...>`
//! is `true` when the route `derive_applicative<IMPL>::invoke` takes for
//! these arguments is non-throwing: `IMPL`'s own `invoke` where it accepts
//! them, and otherwise the embedding of the function with `pure` followed by
//! one `ap` per argument.
template <class IMPL, class SELF, class FUNCTION, class... ARGUMENTS>
inline constexpr bool applicative_invoke_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.invoke(std::declval<FUNCTION>(), std::declval<ARGUMENTS>()...);
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.invoke(std::declval<FUNCTION>(), std::declval<ARGUMENTS>()...)
        } noexcept;
    } : nothrow_invocable_through_ap<SELF, FUNCTION, ARGUMENTS...>;

//! \expos
//! \remarks `$applicative-ap-nothrow$<IMPL, SELF, FUNCTION_IN_CONTEXT,
//! ARGUMENT_IN_CONTEXT>` is `true` when the route
//! `derive_applicative<IMPL>::ap` takes is non-throwing: `IMPL`'s own `ap`
//! where it accepts the arguments, and otherwise `IMPL`'s `invoke` applied to
//! `applicative_eval` and the arguments.
template <class IMPL, class SELF, class FUNCTION_IN_CONTEXT,
          class ARGUMENT_IN_CONTEXT>
inline constexpr bool applicative_ap_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.ap(std::declval<FUNCTION_IN_CONTEXT>(),
                std::declval<ARGUMENT_IN_CONTEXT>());
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.ap(std::declval<FUNCTION_IN_CONTEXT>(),
                    std::declval<ARGUMENT_IN_CONTEXT>())
        } noexcept;
    } : requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.invoke(applicative_eval, std::declval<FUNCTION_IN_CONTEXT>(),
                        std::declval<ARGUMENT_IN_CONTEXT>())
        } noexcept;
    };

//! \expos
//! \remarks `$applicative-map-nothrow$<IMPL, SELF, FUNCTION, ARGUMENT>` is
//! `true` when the route `derive_applicative<IMPL>::map` takes is
//! non-throwing: `IMPL`'s own `map` where it accepts the arguments, and
//! otherwise the object's own `invoke` applied to them.
template <class IMPL, class SELF, class FUNCTION, class ARGUMENT>
inline constexpr bool applicative_map_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.map(std::declval<FUNCTION>(), std::declval<ARGUMENT>());
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.map(std::declval<FUNCTION>(), std::declval<ARGUMENT>())
        } noexcept;
    } : requires(SELF &self) {
        {
            self.invoke(std::declval<FUNCTION>(), std::declval<ARGUMENT>())
        } noexcept;
    };

//! \expos
//! \remarks `$applicative-lift-nothrow$<IMPL, SELF, VALUE>` is `true` when
//! the route `derive_applicative<IMPL>::lift` takes is non-throwing: `IMPL`'s
//! own `lift` where it accepts `VALUE`, and otherwise the object's own
//! `pure`.
template <class IMPL, class SELF, class VALUE>
inline constexpr bool applicative_lift_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.lift(std::declval<VALUE>());
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        { impl.lift(std::declval<VALUE>()) } noexcept;
    } : requires(SELF &self) {
        { self.pure(std::declval<VALUE>()) } noexcept;
    };

//! \expos
//! \remarks `$applicative-zip-with-nothrow$<IMPL, SELF, FUNCTION,
//! FIRST_ARGUMENT, SECOND_ARGUMENT>` is `true` when the route
//! `derive_applicative<IMPL>::zip_with` takes is non-throwing: `IMPL`'s own
//! `zip_with` where it accepts the arguments, and otherwise the object's own
//! `invoke` applied to them.
template <class IMPL, class SELF, class FUNCTION, class FIRST_ARGUMENT,
          class SECOND_ARGUMENT>
inline constexpr bool applicative_zip_with_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.zip_with(std::declval<FUNCTION>(), std::declval<FIRST_ARGUMENT>(),
                      std::declval<SECOND_ARGUMENT>());
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.zip_with(std::declval<FUNCTION>(),
                          std::declval<FIRST_ARGUMENT>(),
                          std::declval<SECOND_ARGUMENT>())
        } noexcept;
    } : requires(SELF &self) {
        {
            self.invoke(std::declval<FUNCTION>(),
                        std::declval<FIRST_ARGUMENT>(),
                        std::declval<SECOND_ARGUMENT>())
        } noexcept;
    };

//! \expos
//! \remarks `$applicative-discard-first-nothrow$<IMPL, SELF, FIRST_ARGUMENT,
//! SECOND_ARGUMENT>` is `true` when the route
//! `derive_applicative<IMPL>::discard_first` takes is non-throwing: `IMPL`'s
//! own `discard_first` where it accepts the arguments, and otherwise the
//! object's own `invoke` applied to `discard_first_eval` and them.
template <class IMPL, class SELF, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
inline constexpr bool applicative_discard_first_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.discard_first(std::declval<FIRST_ARGUMENT>(),
                           std::declval<SECOND_ARGUMENT>());
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.discard_first(std::declval<FIRST_ARGUMENT>(),
                               std::declval<SECOND_ARGUMENT>())
        } noexcept;
    } : requires(SELF &self) {
        {
            self.invoke(discard_first_eval, std::declval<FIRST_ARGUMENT>(),
                        std::declval<SECOND_ARGUMENT>())
        } noexcept;
    };

//! \expos
//! \remarks `$applicative-discard-second-nothrow$<IMPL, SELF, FIRST_ARGUMENT,
//! SECOND_ARGUMENT>` is `true` when the route
//! `derive_applicative<IMPL>::discard_second` takes is non-throwing: `IMPL`'s
//! own `discard_second` where it accepts the arguments, and otherwise the
//! object's own `invoke` applied to `discard_second_eval` and them.
template <class IMPL, class SELF, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
inline constexpr bool applicative_discard_second_nothrow =
    requires(impl_ref_t<IMPL, SELF> impl) {
        impl.discard_second(std::declval<FIRST_ARGUMENT>(),
                            std::declval<SECOND_ARGUMENT>());
    } ? requires(impl_ref_t<IMPL, SELF> impl) {
        {
            impl.discard_second(std::declval<FIRST_ARGUMENT>(),
                                std::declval<SECOND_ARGUMENT>())
        } noexcept;
    } : requires(SELF &self) {
        {
            self.invoke(discard_second_eval, std::declval<FIRST_ARGUMENT>(),
                        std::declval<SECOND_ARGUMENT>())
        } noexcept;
    };

} // namespace detail

/// Applicative pattern invariants:
/// - Dual BASIS, single INTERFACE. An instance opts in with pure + invoke
///   or pure + ap -- both are perfectly cromulent bases, and the base class
///   derives whichever one the instance does not supply. The user-facing
///   application verb is invoke: McBride & Paterson's canonical form
///     pure f <*> u1 <*> ... <*> un   ==   invoke(f, u1, ..., un),
///   and the GHC analogue is {-# MINIMAL pure, ((<*>) | liftA2) #-}.
/// - ap (one-step application of a callable-in-context) is the classic
///   basis and remains available as a secondary operation, but it is never
///   the lead verb: papers, examples, and teaching lead with invoke. The
///   interface choice is forced by std::simd::vec, which cannot form a
///   vec<callable> -- ap is unspellable exactly where this proposal's
///   motivation lives, while n-ary invoke works everywhere.
/// - ap-from-invoke is invoke(applicative_eval, cf, cx); it exists only
///   when the context can hold a callable. invoke-from-ap is the currying
///   derivation (detail::terminating_partial); its cost lives entirely in
///   the derivation direction.
/// - Impls keep their basis SFINAE-friendly: a trailing return type via
///   std::invoke_result_t (or an explicit requires clause), never a bare
///   auto return with the result type computed only in the body, so that
///   availability probes fail cleanly instead of hard-erroring.
/// - Every operation carries a computed exception specification: an Impl's
///   basis from the callable's invocation and the carrier's constructors, a
///   derived operation from the route it takes. A SFINAE-friendly basis is
///   what makes the latter computable without instantiating a body
///   (docs/transpose/decisions.md#operation-noexcept).
/// - Derived operations (map/lift/zip_with/discard_*) live on that object.
/// - Dispatch happens through a provided object or
///   applicative<Concrete>.
/// - Do not introduce hidden alternate semantics without a distinct map/type.
//! \remarks This concept is satisfied when `IMPL` supplies the minimal
//! complete basis the `Applicative` CRTP base needs: `pure`, together with
//! either `invoke` or `ap`. This is the `MINIMAL` pragma to
//! `applicative_object`'s class declaration -- an `IMPL` may satisfy this
//! concept and still fail `applicative_object`, which is exactly the
//! bargain the CRTP base exists to keep. `map`, `lift`, `zip_with`,
//! `discard_first`, `discard_second`, `invoke_with` and `subsume` are all
//! derived and belong to `applicative_object` alone. `pure` is checked for
//! existence only, matching `applicative_object`'s own treatment, and is
//! probed with an rvalue element for the reason `applicative_object` records.
template <class IMPL, class CONTEXT>
concept applicative_impl =
    requires(const IMPL &impl) {
        impl.pure(std::declval<applicative_value_t<CONTEXT>>());
    } &&
    (requires(const IMPL &impl, const CONTEXT &context) {
        impl.invoke(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
    } || requires(const IMPL &impl, const CONTEXT &context) {
        impl.ap(impl.pure(detail::probe_witness<applicative_value_t<CONTEXT>>{}),
                context);
    });

/// CRTP base for Applicative instances.
/// `Impl` must provide `pure(value)` and either the n-ary
/// `invoke(f, args_in_context...)` or the one-step
/// `ap(f_in_context, arg_in_context)`; the base derives the missing one and
/// every other operation. invoke is the user-facing interface; ap is the
/// classic basis.
/// A Map may re-expose `Impl::invoke` with a using-declaration, as the
/// library's own instances do, but it need not: the base's `invoke` is
/// constrained on whichever basis `Impl` supplies, so leaving the
/// using-declaration out never turns a probe of `invoke` into a hard error.
/// Re-exposing is an optimization that skips the base's dispatch.
template <class Impl>
struct derive_applicative : protected Impl {
    static_assert(
        !std::is_same_v<Impl, std::false_type>,
        "No applicative<T> specialization found. "
        "Specialize beman::transpose::applicative<T> for "
        "your type T and provide pure(...) plus invoke(...) or ap(...).");
    using Impl::pure;

    // \ref{transpose.applicative.basis}, basis operations
    template <class FUNCTION, class FIRST_ARGUMENT, class... REST_ARGUMENTS>
    [[nodiscard]] constexpr auto
    invoke(this auto &&self, FUNCTION &&function,
           FIRST_ARGUMENT &&first_argument,
           REST_ARGUMENTS
               &&...rest_arguments) noexcept(detail::
                                                 applicative_invoke_nothrow<
                                                     Impl, decltype(self),
                                                     FUNCTION, FIRST_ARGUMENT,
                                                     REST_ARGUMENTS...>)
        requires requires(const Impl &impl) {
            impl.invoke(std::forward<FUNCTION>(function),
                        std::forward<FIRST_ARGUMENT>(first_argument),
                        std::forward<REST_ARGUMENTS>(rest_arguments)...);
        } || detail::invocable_through_ap<decltype(self), FUNCTION,
                                          FIRST_ARGUMENT, REST_ARGUMENTS...>;

    template <class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT>
    [[nodiscard]] constexpr auto
    ap(this auto &&self, FUNCTION_IN_CONTEXT &&function,
       ARGUMENT_IN_CONTEXT
           &&argument) noexcept(detail::
                                    applicative_ap_nothrow<Impl, decltype(self),
                                                           FUNCTION_IN_CONTEXT,
                                                           ARGUMENT_IN_CONTEXT>)
        requires requires(const Impl &impl) {
            impl.ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                    std::forward<ARGUMENT_IN_CONTEXT>(argument));
        } || requires(const Impl &impl) {
            impl.invoke(applicative_eval,
                        std::forward<FUNCTION_IN_CONTEXT>(function),
                        std::forward<ARGUMENT_IN_CONTEXT>(argument));
        };

    // \ref{transpose.applicative.derived}, derived operations
    template <class FUNCTION, class ARGUMENT>
    [[nodiscard]] constexpr auto
    map(this auto &&self, FUNCTION &&function, ARGUMENT &&argument) noexcept(
        detail::applicative_map_nothrow<Impl, decltype(self), FUNCTION,
                                        ARGUMENT>)
        requires requires(const Impl &impl) {
            impl.map(std::forward<FUNCTION>(function),
                     std::forward<ARGUMENT>(argument));
        } || requires {
            self.invoke(std::forward<FUNCTION>(function),
                        std::forward<ARGUMENT>(argument));
        };

    template <class VALUE>
    [[nodiscard]] constexpr auto lift(this auto &&self, VALUE &&value) noexcept(
        detail::applicative_lift_nothrow<Impl, decltype(self), VALUE>)
        requires requires(const Impl &impl) {
            impl.lift(std::forward<VALUE>(value));
        } || requires { self.pure(std::forward<VALUE>(value)); };

    template <class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    [[nodiscard]] constexpr auto zip_with(
        this auto &&self, FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
        SECOND_ARGUMENT &&
            second_argument) noexcept(detail::
                                          applicative_zip_with_nothrow<
                                              Impl, decltype(self), FUNCTION,
                                              FIRST_ARGUMENT, SECOND_ARGUMENT>)
        requires requires(const Impl &impl) {
            impl.zip_with(std::forward<FUNCTION>(function),
                          std::forward<FIRST_ARGUMENT>(first_argument),
                          std::forward<SECOND_ARGUMENT>(second_argument));
        } || requires {
            self.invoke(std::forward<FUNCTION>(function),
                        std::forward<FIRST_ARGUMENT>(first_argument),
                        std::forward<SECOND_ARGUMENT>(second_argument));
        };

    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    [[nodiscard]] constexpr auto discard_first(
        this auto &&self, FIRST_ARGUMENT &&first_argument,
        SECOND_ARGUMENT &&
            second_argument) noexcept(detail::
                                          applicative_discard_first_nothrow<
                                              Impl, decltype(self),
                                              FIRST_ARGUMENT, SECOND_ARGUMENT>)
        requires requires(const Impl &impl) {
            impl.discard_first(std::forward<FIRST_ARGUMENT>(first_argument),
                               std::forward<SECOND_ARGUMENT>(second_argument));
        } || requires {
            self.invoke(discard_first_eval,
                        std::forward<FIRST_ARGUMENT>(first_argument),
                        std::forward<SECOND_ARGUMENT>(second_argument));
        };

    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    [[nodiscard]] constexpr auto discard_second(
        this auto &&self, FIRST_ARGUMENT &&first_argument,
        SECOND_ARGUMENT &&
            second_argument) noexcept(detail::
                                          applicative_discard_second_nothrow<
                                              Impl, decltype(self),
                                              FIRST_ARGUMENT, SECOND_ARGUMENT>)
        requires requires(const Impl &impl) {
            impl.discard_second(std::forward<FIRST_ARGUMENT>(first_argument),
                                std::forward<SECOND_ARGUMENT>(second_argument));
        } || requires {
            self.invoke(discard_second_eval,
                        std::forward<FIRST_ARGUMENT>(first_argument),
                        std::forward<SECOND_ARGUMENT>(second_argument));
        };

    // \ref{transpose.applicative.grade}, grade re-indexing
    template <class TARGET_GRADE, class CARRIER>
    [[nodiscard]] constexpr auto
    subsume(this auto &&, CARRIER &&value) noexcept(
        noexcept(grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value))))
        requires requires {
            grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
        };

    // \ref{transpose.applicative.delegate}, delegated application
    template <class APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
              class... REST_ARGUMENTS>
    [[nodiscard]] constexpr auto invoke_with(
        this auto &&, const APPLICATIVE_MAP &applicative_map,
        FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
        REST_ARGUMENTS
            &&...rest_arguments) noexcept(noexcept(applicative_map
                                                       .invoke(
                                                           std::forward<
                                                               FUNCTION>(
                                                               function),
                                                           std::forward<
                                                               FIRST_ARGUMENT>(
                                                               first_argument),
                                                           std::forward<
                                                               REST_ARGUMENTS>(
                                                               rest_arguments)...)))
        requires requires {
            applicative_map.invoke(
                std::forward<FUNCTION>(function),
                std::forward<FIRST_ARGUMENT>(first_argument),
                std::forward<REST_ARGUMENTS>(rest_arguments)...);
        };

    template <const auto &APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
              class... REST_ARGUMENTS>
    [[nodiscard]] constexpr auto invoke_with(
        this auto &&, FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
        REST_ARGUMENTS
            &&...rest_arguments) noexcept(noexcept(APPLICATIVE_MAP
                                                       .invoke(
                                                           std::forward<
                                                               FUNCTION>(
                                                               function),
                                                           std::forward<
                                                               FIRST_ARGUMENT>(
                                                               first_argument),
                                                           std::forward<
                                                               REST_ARGUMENTS>(
                                                               rest_arguments)...)))
        requires requires {
            APPLICATIVE_MAP.invoke(
                std::forward<FUNCTION>(function),
                std::forward<FIRST_ARGUMENT>(first_argument),
                std::forward<REST_ARGUMENTS>(rest_arguments)...);
        };

  private:
    //! \omit
    template <class SELF>
    static constexpr decltype(auto) impl_of(SELF &&self) noexcept {
        return static_cast<impl_ref_t<Impl, SELF>>(self);
    }

    //! \omit
    template <class ACCUMULATED>
    constexpr auto ap_chain(this auto &&, ACCUMULATED &&accumulated) noexcept(
        std::is_nothrow_move_constructible_v<
            std::remove_cvref_t<ACCUMULATED>>) {
        return std::forward<ACCUMULATED>(accumulated);
    }

    //! \omit
    template <class ACCUMULATED, class NEXT_ARGUMENT, class... REST_ARGUMENTS>
    constexpr auto ap_chain(
        this auto &&self, ACCUMULATED &&accumulated,
        NEXT_ARGUMENT &&next_argument,
        REST_ARGUMENTS
            &&...rest_arguments) noexcept(detail::
                                              ap_chain_nothrow<
                                                  decltype(self), ACCUMULATED,
                                                  NEXT_ARGUMENT,
                                                  REST_ARGUMENTS...>::value) {
        auto next = self.ap(std::forward<ACCUMULATED>(accumulated),
                            std::forward<NEXT_ARGUMENT>(next_argument));
        if constexpr (sizeof...(REST_ARGUMENTS) == 0) {
            return next;
        } else {
            return self.ap_chain(std::move(next), std::forward<REST_ARGUMENTS>(
                                                      rest_arguments)...);
        }
    }
};

//! \remarks This variable template is the lookup point for the Applicative
//! object of a context type. A program may specialize it for a
//! program-defined context. The primary template names no applicative
//! object.
//! The type of each object named by a specialization this document declares
//! is a class type that is not final and models `semiregular`, so a program
//! may derive from it to combine several objects in one algorithm object.
template <class T>
inline constexpr auto applicative = std::false_type{};

//! \remarks This variable template is a second lookup point, over the same
//! carrier and grade algebra as `applicative`, naming the
//! accumulating Applicative object. Where the object named by
//! `applicative` stops at the first failing operand, this object
//! combines the evidence of every failing operand. Neither object is
//! selected automatically for a carrier: the context type alone does not
//! determine which composition discipline a caller wants. This object has
//! no Monad instance, because sequencing requires a value from a
//! computation that accumulation admits may have failed.
//! The type of each object named by a specialization this document declares
//! is a class type that is not final and models `semiregular`, so a program
//! may derive from it to combine several objects in one algorithm object.
template <class T>
inline constexpr auto accumulating_applicative = std::false_type{};

// -- Operation objects --
//
// See functor.hpp for the shape and the motivation. Applicative is where the
// deducing form meets its one structural limit: `pure` has no in-context
// argument, so nothing at the call site names the context to look up. It is
// spelled as a variable template of operation objects, taking the context --
// `pure<std::optional<int>>(42)` -- and it is the only operation in this
// header that does not deduce.

/** Operation object for Applicative's `invoke`, the user-facing
 * application verb.
 *
 * The context keys the lookup and is deduced from the first in-context
 * argument; every operand is expected to share that context.
 */
//! \omit
struct invoke_fn {
    /** Applies the plain callable `function` to N in-context arguments,
     * producing one in-context result.
     *
     * @tparam TC  The Applicative instance, from the lookup for the keying
     * argument. Not for callers to supply; pin with mode 2.
     */
    template <class FUNCTION, class FIRST_ARGUMENT, class... REST_ARGUMENTS,
              const auto &TC = applicative<std::remove_cvref_t<FIRST_ARGUMENT>>>
        requires requires {
            TC.invoke(std::declval<FUNCTION>(), std::declval<FIRST_ARGUMENT>(),
                      std::declval<REST_ARGUMENTS>()...);
        }
    [[nodiscard]] constexpr auto
    operator()(FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
               REST_ARGUMENTS &&...rest_arguments) const
        noexcept(noexcept(
            TC.invoke(std::forward<FUNCTION>(function),
                      std::forward<FIRST_ARGUMENT>(first_argument),
                      std::forward<REST_ARGUMENTS>(rest_arguments)...))) {
        return TC.invoke(std::forward<FUNCTION>(function),
                         std::forward<FIRST_ARGUMENT>(first_argument),
                         std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }
};

/** Operation object for Applicative's `ap`, the classic one-step
 * application; secondary to `invoke`, and available only where the context
 * can hold a callable.
 *
 * The context keys the lookup and is deduced from the argument-in-context,
 * not from the callable-in-context.
 */
//! \omit
struct ap_fn {
    /** Applies a callable held in context to one argument in context. */
    template <
        class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT,
        const auto &TC = applicative<std::remove_cvref_t<ARGUMENT_IN_CONTEXT>>>
        requires requires {
            TC.ap(std::declval<FUNCTION_IN_CONTEXT>(),
                  std::declval<ARGUMENT_IN_CONTEXT>());
        }
    [[nodiscard]] constexpr auto
    operator()(FUNCTION_IN_CONTEXT &&function,
               ARGUMENT_IN_CONTEXT &&argument) const
        noexcept(noexcept(TC.ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                                std::forward<ARGUMENT_IN_CONTEXT>(argument)))) {
        return TC.ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                     std::forward<ARGUMENT_IN_CONTEXT>(argument));
    }
};

/** Operation object for Applicative's `pure`, parameterized on the
 * context because no argument can name it.
 *
 * `CONTEXT` is the instantiated context type that keys the lookup, e.g.
 * `pure<std::optional<int>>(42)`. This is the one place the deducing
 * pattern does not reach: `pure` builds the context rather than consuming
 * one, so the context is in the return type and never in an argument.
 */
//! \omit
template <class CONTEXT,
          const auto &TC = applicative<std::remove_cvref_t<CONTEXT>>>
struct pure_fn {
    /** Embeds a plain value into the applicative context. */
    template <class VALUE>
        requires requires { TC.pure(std::declval<VALUE>()); }
    [[nodiscard]] constexpr auto operator()(VALUE &&value) const
        noexcept(noexcept(TC.pure(std::forward<VALUE>(value)))) {
        return TC.pure(std::forward<VALUE>(value));
    }
};

/** Operation object for `raise`, the dual of `pure`: it builds a context in
 * its failure state, so like `pure` it must name the context. Dispatches to
 * the applicative object's own `raise`; an object that has none, because its
 * context's failure state carries no witness or because it cannot fail, is a
 * constraint that does not match. */
//! \omit
template <class CONTEXT,
          const auto &TC = applicative<std::remove_cvref_t<CONTEXT>>>
struct raise_fn {
    /** Enters the failure state of the context with `error` as its witness. */
    template <class ERROR>
        requires requires { TC.raise(std::declval<ERROR>()); }
    [[nodiscard]] constexpr auto operator()(ERROR &&error) const
        noexcept(noexcept(TC.raise(std::forward<ERROR>(error)))) {
        return TC.raise(std::forward<ERROR>(error));
    }
};

/** Operation object for `zero`, the failure state that carries no witness:
 * the identity of Alternative's disjunction, decided under the same rule as
 * `raise` (docs/transpose/decisions.md#failure-injection-spelling). Only a
 * context whose failure state needs nothing has one. */
//! \omit
template <class CONTEXT,
          const auto &TC = applicative<std::remove_cvref_t<CONTEXT>>>
struct zero_fn {
    /** The context in its witness-free failure state. */
    [[nodiscard]] constexpr auto operator()() const
        noexcept(noexcept(TC.zero()))
        requires requires { TC.zero(); }
    {
        return TC.zero();
    }
};

/** Operation object for the derived `zip_with`. */
//! \omit
struct zip_with_fn {
    /** Lifts a binary function over two in-context arguments. */
    template <class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT,
              const auto &TC = applicative<std::remove_cvref_t<FIRST_ARGUMENT>>>
        requires requires {
            TC.zip_with(std::declval<FUNCTION>(),
                        std::declval<FIRST_ARGUMENT>(),
                        std::declval<SECOND_ARGUMENT>());
        }
    [[nodiscard]] constexpr auto
    operator()(FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
               SECOND_ARGUMENT &&second_argument) const
        noexcept(noexcept(
            TC.zip_with(std::forward<FUNCTION>(function),
                        std::forward<FIRST_ARGUMENT>(first_argument),
                        std::forward<SECOND_ARGUMENT>(second_argument)))) {
        return TC.zip_with(std::forward<FUNCTION>(function),
                           std::forward<FIRST_ARGUMENT>(first_argument),
                           std::forward<SECOND_ARGUMENT>(second_argument));
    }
};

/** Operation object for the derived `discard_first`. */
//! \omit
struct discard_first_fn {
    /** Sequences two in-context values, keeping the second. */
    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT,
              const auto &TC = applicative<std::remove_cvref_t<FIRST_ARGUMENT>>>
        requires requires {
            TC.discard_first(std::declval<FIRST_ARGUMENT>(),
                             std::declval<SECOND_ARGUMENT>());
        }
    [[nodiscard]] constexpr auto
    operator()(FIRST_ARGUMENT &&first_argument,
               SECOND_ARGUMENT &&second_argument) const
        noexcept(noexcept(
            TC.discard_first(std::forward<FIRST_ARGUMENT>(first_argument),
                             std::forward<SECOND_ARGUMENT>(second_argument)))) {
        return TC.discard_first(std::forward<FIRST_ARGUMENT>(first_argument),
                                std::forward<SECOND_ARGUMENT>(second_argument));
    }
};

/** Operation object for the derived `discard_second`. */
//! \omit
struct discard_second_fn {
    /** Sequences two in-context values, keeping the first. */
    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT,
              const auto &TC = applicative<std::remove_cvref_t<FIRST_ARGUMENT>>>
        requires requires {
            TC.discard_second(std::declval<FIRST_ARGUMENT>(),
                              std::declval<SECOND_ARGUMENT>());
        }
    [[nodiscard]] constexpr auto
    operator()(FIRST_ARGUMENT &&first_argument,
               SECOND_ARGUMENT &&second_argument) const
        noexcept(noexcept(TC.discard_second(
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument)))) {
        return TC.discard_second(
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument));
    }
};

//! \remarks This concept is satisfied when `OBJ` provides the full
//! Applicative object surface over `CONTEXT`: `pure`, the `invoke` basis, and
//! the derived `map`, `lift`, `zip_with`, `discard_first`, `discard_second`
//! and `invoke_with`. `ap` and `subsume` are required only where their own
//! condition -- the same one their own declarations carry, not a second
//! spelling of it -- licenses them: `ap` where `CONTEXT` can hold a
//! callable (probed by lifting a witness callable through `OBJ`'s own
//! `pure`, the same mechanism the library's own ap-from-invoke derivation
//! uses), `subsume` where `CONTEXT` participates in grading. Operations
//! templated over an arbitrary callable are probed with one representative
//! witness ($probe-witness$/$probe-witness2$): this checks that the
//! operation exists, not that it holds for every callable. Conformance here
//! is structural, so a hand-implemented object that never derives from
//! `derive_applicative<Impl>` can satisfy this concept.
//!
//! Every probe is written in the value category the operation is used in.
//! `pure` and `lift` take a value *into* the context and are probed with an
//! rvalue element; `discard_first` and `discard_second` return a value *out*
//! of one of their operands and are probed with rvalue operands. Probing
//! either with a `const` lvalue asks the context to copy, which no
//! operation's specification requires, and which for an element type that
//! can be moved but not copied is not merely a stricter test but an
//! ill-formed one: these operations have deduced return types, so a probe
//! instantiates the body, and a body that cannot copy is a diagnostic rather
//! than an unsatisfied constraint. A concept that diagnoses cannot be used
//! to detect anything.
template <class OBJ, class CONTEXT>
concept applicative_object =
    requires(const OBJ &obj, const CONTEXT &context) {
        obj.pure(std::declval<applicative_value_t<CONTEXT>>());
        obj.invoke(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
        obj.map(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
        obj.lift(std::declval<applicative_value_t<CONTEXT>>());
        obj.zip_with(detail::probe_witness2<applicative_value_t<CONTEXT>>{}, context,
                     context);
        obj.discard_first(std::declval<CONTEXT>(), std::declval<CONTEXT>());
        obj.discard_second(std::declval<CONTEXT>(), std::declval<CONTEXT>());
        obj.invoke_with(obj, detail::probe_witness<applicative_value_t<CONTEXT>>{},
                        context);
    } &&
    (!requires(const OBJ &obj) {
        obj.pure(detail::probe_witness<applicative_value_t<CONTEXT>>{});
    } || requires(const OBJ &obj, const CONTEXT &context) {
        obj.ap(obj.pure(detail::probe_witness<applicative_value_t<CONTEXT>>{}),
               context);
    }) &&
    (!graded_context<CONTEXT> ||
     requires(const OBJ &obj, const CONTEXT &context) {
         obj.template subsume<grade_of_t<CONTEXT>>(context);
     });

// \rSec3[transpose.applicative.basis]{Basis operations}

//! \constraints Either `Impl` provides an `invoke` accepting `function` and
//! the arguments, or `Impl` provides an `ap` and the application described
//! in the *Effects* element below is well-formed, which requires that
//! `function` be invocable with the values held by all of the arguments.
//! \effects Lifts `function` into the context and applies it to the
//! arguments, left to right. If `Impl` provides `invoke`, the effect is that
//! of `Impl`'s own `invoke`; otherwise `function` is embedded with `pure` and
//! applied one argument at a time using `ap`.
//! \returns The single value in context holding the result of applying
//! `function` to the values held by the arguments.
//! \remarks The arguments are evaluated in the order written. No argument's
//! context depends on another argument's value. When `invoke` is expressed
//! using `ap`, `function` is invoked only once it has been applied to every
//! argument; its arity is the number of arguments, never inferred from what
//! `function` accepts.
template <class Impl>
template <class FUNCTION, class FIRST_ARGUMENT, class... REST_ARGUMENTS>
[[nodiscard]] constexpr auto derive_applicative<Impl>::invoke(
    this auto &&self, FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
    REST_ARGUMENTS
        &&...rest_arguments) noexcept(detail::
                                          applicative_invoke_nothrow<
                                              Impl, decltype(self), FUNCTION,
                                              FIRST_ARGUMENT,
                                              REST_ARGUMENTS...>)
    requires requires(const Impl &impl) {
        impl.invoke(std::forward<FUNCTION>(function),
                    std::forward<FIRST_ARGUMENT>(first_argument),
                    std::forward<REST_ARGUMENTS>(rest_arguments)...);
    } || detail::invocable_through_ap<decltype(self), FUNCTION, FIRST_ARGUMENT,
                                      REST_ARGUMENTS...>
{
    if constexpr (requires {
                      impl_of(self).invoke(
                          std::forward<FUNCTION>(function),
                          std::forward<FIRST_ARGUMENT>(first_argument),
                          std::forward<REST_ARGUMENTS>(rest_arguments)...);
                  }) {
        return impl_of(self).invoke(
            std::forward<FUNCTION>(function),
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<REST_ARGUMENTS>(rest_arguments)...);
    } else {
        // Derivation from the ap basis:
        //   invoke(f, x1, ..., xn) = pure(curried f) `ap` x1 `ap` ... xn
        // This static_assert is provably redundant: the declaration's
        // constraint already established that the ap chain is well-formed,
        // which needs an ap basis. It is the GHC-MINIMAL-style last-resort
        // message fold_map also keeps, not load-bearing SFINAE.
        static_assert(
            applicative_impl<Impl, std::remove_cvref_t<FIRST_ARGUMENT>>,
            "Applicative Impl must provide pure and at least one basis: "
            "invoke(f, args_in_context...) or "
            "ap(f_in_context, arg_in_context).");
        auto lifted_function = self.pure(
            detail::make_terminating_partial<1 + sizeof...(REST_ARGUMENTS)>(
                std::forward<FUNCTION>(function)));
        return self.ap_chain(
            self.ap(std::move(lifted_function),
                    std::forward<FIRST_ARGUMENT>(first_argument)),
            std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }
}

//! \constraints `Impl` provides either an `ap` accepting `function` and
//! `argument`, or an `invoke` able to apply a callable held in the context to
//! one argument in the context. The second alternative is satisfied only
//! where the context can hold a callable, so `ap` does not participate in
//! overload resolution for a context that cannot.
//! \effects Applies the callable held by `function` to the value held by
//! `argument`. If `Impl` provides `ap`, the effect is that of `Impl`'s own
//! `ap`; otherwise the application is expressed through `Impl`'s `invoke`.
//! \returns The single value in context holding the result of that
//! application.
//! \remarks This is the classic one-step application, retained as a secondary
//! operation and as an instance basis. Both alternatives address `Impl`
//! directly, so no derivation cycle with `invoke` arises.
template <class Impl>
template <class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT>
[[nodiscard]] constexpr auto derive_applicative<Impl>::ap(
    this auto &&self, FUNCTION_IN_CONTEXT &&function,
    ARGUMENT_IN_CONTEXT &&
        argument) noexcept(detail::applicative_ap_nothrow<Impl, decltype(self),
                                                          FUNCTION_IN_CONTEXT,
                                                          ARGUMENT_IN_CONTEXT>)
    requires requires(const Impl &impl) {
        impl.ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                std::forward<ARGUMENT_IN_CONTEXT>(argument));
    } || requires(const Impl &impl) {
        impl.invoke(applicative_eval,
                    std::forward<FUNCTION_IN_CONTEXT>(function),
                    std::forward<ARGUMENT_IN_CONTEXT>(argument));
    }
{
    if constexpr (requires {
                      impl_of(self).ap(
                          std::forward<FUNCTION_IN_CONTEXT>(function),
                          std::forward<ARGUMENT_IN_CONTEXT>(argument));
                  }) {
        return impl_of(self).ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                                std::forward<ARGUMENT_IN_CONTEXT>(argument));
    } else {
        return impl_of(self).invoke(
            detail::applicative_eval,
            std::forward<FUNCTION_IN_CONTEXT>(function),
            std::forward<ARGUMENT_IN_CONTEXT>(argument));
    }
}

// \rSec3[transpose.applicative.derived]{Derived operations}

//! \constraints `Impl` provides either a `map` accepting `function` and
//! `argument`, or an `invoke` accepting `function` and `argument`, whether
//! native to `Impl` or derived from `Impl`'s basis.
//! \effects If `Impl` provides `map`, the effect is that of `Impl`'s own
//! `map`; otherwise the application is expressed through the object's own
//! `invoke`, available for either half of the dual basis.
//! \returns The single value in context holding the result of applying
//! `function` to the value held by `argument`.
template <class Impl>
template <class FUNCTION, class ARGUMENT>
[[nodiscard]] constexpr auto derive_applicative<Impl>::map(
    this auto &&self, FUNCTION &&function,
    ARGUMENT &&
        argument) noexcept(detail::applicative_map_nothrow<Impl, decltype(self),
                                                           FUNCTION, ARGUMENT>)
    requires requires(const Impl &impl) {
        impl.map(std::forward<FUNCTION>(function),
                 std::forward<ARGUMENT>(argument));
    } || requires {
        self.invoke(std::forward<FUNCTION>(function),
                    std::forward<ARGUMENT>(argument));
    }
{
    if constexpr (requires {
                      impl_of(self).map(std::forward<FUNCTION>(function),
                                        std::forward<ARGUMENT>(argument));
                  }) {
        return impl_of(self).map(std::forward<FUNCTION>(function),
                                 std::forward<ARGUMENT>(argument));
    } else {
        return self.invoke(std::forward<FUNCTION>(function),
                           std::forward<ARGUMENT>(argument));
    }
}

//! \constraints `Impl` provides either a `lift` accepting `value`, or a
//! `pure` accepting `value`.
//! \effects If `Impl` provides `lift`, the effect is that of `Impl`'s own
//! `lift`; otherwise `value` is lifted into the context using the object's
//! own `pure`, which `Impl` is required to provide directly.
//! \returns The single value in context holding `value`.
template <class Impl>
template <class VALUE>
[[nodiscard]] constexpr auto
derive_applicative<Impl>::lift(this auto &&self, VALUE &&value) noexcept(
    detail::applicative_lift_nothrow<Impl, decltype(self), VALUE>)
    requires requires(const Impl &impl) {
        impl.lift(std::forward<VALUE>(value));
    } || requires { self.pure(std::forward<VALUE>(value)); }
{
    if constexpr (requires {
                      impl_of(self).lift(std::forward<VALUE>(value));
                  }) {
        return impl_of(self).lift(std::forward<VALUE>(value));
    } else {
        return self.pure(std::forward<VALUE>(value));
    }
}

//! \constraints `Impl` provides either a `zip_with` accepting `function` and
//! the two arguments, or an `invoke` accepting `function` and the two
//! arguments, whether native to `Impl` or derived from `Impl`'s basis.
//! \effects If `Impl` provides `zip_with`, the effect is that of `Impl`'s
//! own `zip_with`; otherwise the application is expressed through the
//! object's own `invoke`, available for either half of the dual basis.
//! \returns The single value in context holding the result of applying
//! `function` to the values held by `first_argument` and `second_argument`.
template <class Impl>
template <class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
[[nodiscard]] constexpr auto derive_applicative<Impl>::zip_with(
    this auto &&self, FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
    SECOND_ARGUMENT
        &&second_argument) noexcept(detail::
                                        applicative_zip_with_nothrow<
                                            Impl, decltype(self), FUNCTION,
                                            FIRST_ARGUMENT, SECOND_ARGUMENT>)
    requires requires(const Impl &impl) {
        impl.zip_with(std::forward<FUNCTION>(function),
                      std::forward<FIRST_ARGUMENT>(first_argument),
                      std::forward<SECOND_ARGUMENT>(second_argument));
    } || requires {
        self.invoke(std::forward<FUNCTION>(function),
                    std::forward<FIRST_ARGUMENT>(first_argument),
                    std::forward<SECOND_ARGUMENT>(second_argument));
    }
{
    if constexpr (requires {
                      impl_of(self).zip_with(
                          std::forward<FUNCTION>(function),
                          std::forward<FIRST_ARGUMENT>(first_argument),
                          std::forward<SECOND_ARGUMENT>(second_argument));
                  }) {
        return impl_of(self).zip_with(
            std::forward<FUNCTION>(function),
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument));
    } else {
        return self.invoke(std::forward<FUNCTION>(function),
                           std::forward<FIRST_ARGUMENT>(first_argument),
                           std::forward<SECOND_ARGUMENT>(second_argument));
    }
}

//! \constraints `Impl` provides either a `discard_first` accepting
//! `first_argument` and `second_argument`, or an `invoke` accepting a
//! callable that ignores its first parameter and returns its second,
//! together with `first_argument` and `second_argument` -- an `invoke`
//! native to `Impl` or derived from `Impl`'s basis.
//! \effects If `Impl` provides `discard_first`, the effect is that of
//! `Impl`'s own `discard_first`; otherwise the application is expressed
//! through the object's own `invoke`, applying a callable that discards the
//! value held by `first_argument` and returns the value held by
//! `second_argument`.
//! \returns The single value in context holding the value that
//! `second_argument` holds.
template <class Impl>
template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
[[nodiscard]] constexpr auto derive_applicative<Impl>::discard_first(
    this auto &&self, FIRST_ARGUMENT &&first_argument,
    SECOND_ARGUMENT
        &&second_argument) noexcept(detail::
                                        applicative_discard_first_nothrow<
                                            Impl, decltype(self),
                                            FIRST_ARGUMENT, SECOND_ARGUMENT>)
    requires requires(const Impl &impl) {
        impl.discard_first(std::forward<FIRST_ARGUMENT>(first_argument),
                           std::forward<SECOND_ARGUMENT>(second_argument));
    } || requires {
        self.invoke(discard_first_eval,
                    std::forward<FIRST_ARGUMENT>(first_argument),
                    std::forward<SECOND_ARGUMENT>(second_argument));
    }
{
    if constexpr (requires {
                      impl_of(self).discard_first(
                          std::forward<FIRST_ARGUMENT>(first_argument),
                          std::forward<SECOND_ARGUMENT>(second_argument));
                  }) {
        return impl_of(self).discard_first(
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument));
    } else {
        return self.invoke(discard_first_eval,
                           std::forward<FIRST_ARGUMENT>(first_argument),
                           std::forward<SECOND_ARGUMENT>(second_argument));
    }
}

//! \constraints `Impl` provides either a `discard_second` accepting
//! `first_argument` and `second_argument`, or an `invoke` accepting a
//! callable that returns its first parameter and ignores its second,
//! together with `first_argument` and `second_argument` -- an `invoke`
//! native to `Impl` or derived from `Impl`'s basis.
//! \effects If `Impl` provides `discard_second`, the effect is that of
//! `Impl`'s own `discard_second`; otherwise the application is expressed
//! through the object's own `invoke`, applying a callable that returns the
//! value held by `first_argument` and discards the value held by
//! `second_argument`.
//! \returns The single value in context holding the value that
//! `first_argument` holds.
template <class Impl>
template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
[[nodiscard]] constexpr auto derive_applicative<Impl>::discard_second(
    this auto &&self, FIRST_ARGUMENT &&first_argument,
    SECOND_ARGUMENT
        &&second_argument) noexcept(detail::
                                        applicative_discard_second_nothrow<
                                            Impl, decltype(self),
                                            FIRST_ARGUMENT, SECOND_ARGUMENT>)
    requires requires(const Impl &impl) {
        impl.discard_second(std::forward<FIRST_ARGUMENT>(first_argument),
                            std::forward<SECOND_ARGUMENT>(second_argument));
    } || requires {
        self.invoke(discard_second_eval,
                    std::forward<FIRST_ARGUMENT>(first_argument),
                    std::forward<SECOND_ARGUMENT>(second_argument));
    }
{
    if constexpr (requires {
                      impl_of(self).discard_second(
                          std::forward<FIRST_ARGUMENT>(first_argument),
                          std::forward<SECOND_ARGUMENT>(second_argument));
                  }) {
        return impl_of(self).discard_second(
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument));
    } else {
        return self.invoke(discard_second_eval,
                           std::forward<FIRST_ARGUMENT>(first_argument),
                           std::forward<SECOND_ARGUMENT>(second_argument));
    }
}

// \rSec3[transpose.applicative.grade]{Grade re-indexing}

//! \constraints `value` can be re-indexed to `TARGET_GRADE`.
//! \effects-equiv
//! \remarks An instance that does not participate in grading still provides
//! this member. Such a carrier is graded by the empty set, the only licensed
//! target is the empty set, and the re-indexing is the identity. Grade
//! participation is therefore never something an instance declares.
template <class Impl>
template <class TARGET_GRADE, class CARRIER>
[[nodiscard]] constexpr auto
derive_applicative<Impl>::subsume(this auto &&, CARRIER &&value) noexcept(
    noexcept(grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value))))
    requires requires {
        grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
    }
{
    return grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
}

// \rSec3[transpose.applicative.delegate]{Delegated application}

//! \constraints `applicative_map.invoke(function, first_argument,
//! rest_arguments...)`, with each argument forwarded in its own value
//! category, is a valid expression.
//! \effects-equiv
template <class Impl>
template <class APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
          class... REST_ARGUMENTS>
[[nodiscard]] constexpr auto derive_applicative<Impl>::invoke_with(
    this auto &&, const APPLICATIVE_MAP &applicative_map, FUNCTION &&function,
    FIRST_ARGUMENT &&first_argument,
    REST_ARGUMENTS
        &&...rest_arguments) noexcept(noexcept(applicative_map
                                                   .invoke(
                                                       std::forward<FUNCTION>(
                                                           function),
                                                       std::forward<
                                                           FIRST_ARGUMENT>(
                                                           first_argument),
                                                       std::forward<
                                                           REST_ARGUMENTS>(
                                                           rest_arguments)...)))
    requires requires {
        applicative_map.invoke(std::forward<FUNCTION>(function),
                               std::forward<FIRST_ARGUMENT>(first_argument),
                               std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }
{
    return applicative_map.invoke(
        std::forward<FUNCTION>(function),
        std::forward<FIRST_ARGUMENT>(first_argument),
        std::forward<REST_ARGUMENTS>(rest_arguments)...);
}

//! \constraints `APPLICATIVE_MAP.invoke(function, first_argument,
//! rest_arguments...)`, with each argument forwarded in its own value
//! category, is a valid expression.
//! \effects-equiv
template <class Impl>
template <const auto &APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
          class... REST_ARGUMENTS>
[[nodiscard]] constexpr auto derive_applicative<Impl>::invoke_with(
    this auto &&, FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
    REST_ARGUMENTS
        &&...rest_arguments) noexcept(noexcept(APPLICATIVE_MAP
                                                   .invoke(
                                                       std::forward<FUNCTION>(
                                                           function),
                                                       std::forward<
                                                           FIRST_ARGUMENT>(
                                                           first_argument),
                                                       std::forward<
                                                           REST_ARGUMENTS>(
                                                           rest_arguments)...)))
    requires requires {
        APPLICATIVE_MAP.invoke(std::forward<FUNCTION>(function),
                               std::forward<FIRST_ARGUMENT>(first_argument),
                               std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }
{
    return APPLICATIVE_MAP.invoke(
        std::forward<FUNCTION>(function),
        std::forward<FIRST_ARGUMENT>(first_argument),
        std::forward<REST_ARGUMENTS>(rest_arguments)...);
}

// \rSec3[transpose.applicative.objects]{Operation objects}

//! \seebelow
//! \constraints For a call `invoke(function, first_argument,
//! rest_arguments...)`, let `C` be
//! `remove_cvref_t<decltype((first_argument))>`.
//! `applicative<C>.invoke(function, first_argument, rest_arguments...)` is a
//! valid expression.
//! \effects Equivalent to `applicative<C>.invoke(function, first_argument,
//! rest_arguments...)`, with each argument forwarded in its own value
//! category.
//! \remarks The name `invoke` denotes an operation object: it is not found by
//! argument-dependent lookup, and no explicit template argument list may be
//! given in a call to it. The context is deduced from the first in-context
//! argument; every operand shares it. The exception specification is
//! equivalent to `noexcept(applicative<C>.invoke(function, first_argument,
//! rest_arguments...))`.
inline constexpr invoke_fn invoke{};

//! \seebelow
//! \constraints For a call `ap(function, argument)`, let `C` be
//! `remove_cvref_t<decltype((argument))>`. `applicative<C>.ap(function,
//! argument)` is a valid expression.
//! \effects Equivalent to `applicative<C>.ap(function, argument)`, with each
//! argument forwarded in its own value category.
//! \remarks The name `ap` denotes an operation object: it is not found by
//! argument-dependent lookup, and no explicit template argument list may be
//! given in a call to it. The context is deduced from the argument in context,
//! not from the callable in context. The exception specification is
//! equivalent to `noexcept(applicative<C>.ap(function, argument))`.
inline constexpr ap_fn ap{};

//! \seebelow
//! \constraints For a call `pure<CONTEXT>(value)`, let `C` be
//! `remove_cvref_t<CONTEXT>`. `applicative<C>.pure(value)` is a valid
//! expression.
//! \effects Equivalent to `applicative<C>.pure(value)`, with `value`
//! forwarded in its own value category.
//! \remarks The name `pure` denotes a family of operation objects, one per
//! context. None is found by argument-dependent lookup, and `CONTEXT` is the
//! only template argument a call gives. The context is named rather than
//! deduced: `pure` builds a context instead of consuming one, so no argument
//! carries it. The exception specification is equivalent to
//! `noexcept(applicative<C>.pure(value))`.
template <class CONTEXT>
inline constexpr pure_fn<CONTEXT> pure{};

//! \seebelow
//! \constraints For a call `raise<CONTEXT>(error)`, let `C` be
//! `remove_cvref_t<CONTEXT>`. `applicative<C>.raise(error)` is a valid
//! expression.
//! \effects Equivalent to `applicative<C>.raise(error)`, with `error`
//! forwarded in its own value category.
//! \remarks The name `raise` denotes a family of operation objects, one per
//! context, the dual of `pure`: where `pure` enters the context's success
//! state with a value, `raise` enters its failure state with a witness. The
//! context is named for the reason `pure`'s is: the operation builds a
//! context rather than consuming one. The grade of the result is the
//! object's business; for the error-set model it is the join of the context's
//! grade with the grade `error`'s type alone would carry, spelled lazily. The
//! exception specification is equivalent to
//! `noexcept(applicative<C>.raise(error))`.
template <class CONTEXT>
inline constexpr raise_fn<CONTEXT> raise{};

//! \seebelow
//! \constraints For a call `zero<CONTEXT>()`, let `C` be
//! `remove_cvref_t<CONTEXT>`. `applicative<C>.zero()` is a valid expression.
//! \effects Equivalent to `applicative<C>.zero()`.
//! \remarks The name `zero` denotes a family of operation objects, one per
//! context: the failure state that carries no witness, which only a context
//! whose failure state needs nothing has. It is the identity of the
//! disjunction an Alternative object would supply, and is spelled now so that
//! `raise` and `zero` are decided under one rule. `std::expected` has no
//! `zero`: its failure state needs a witness, which is what `raise` supplies.
//! The exception specification is equivalent to
//! `noexcept(applicative<C>.zero())`.
template <class CONTEXT>
inline constexpr zero_fn<CONTEXT> zero{};

//! \seebelow
//! \constraints For a call `zip_with(function, first_argument,
//! second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`.
//! `applicative<C>.zip_with(function, first_argument, second_argument)` is a
//! valid expression.
//! \effects Equivalent to `applicative<C>.zip_with(function, first_argument,
//! second_argument)`, with each argument forwarded in its own value category.
//! \remarks The name `zip_with` denotes an operation object: it is not found by
//! argument-dependent lookup, and no explicit template argument list may be
//! given in a call to it. The exception specification is equivalent to
//! `noexcept(applicative<C>.zip_with(function, first_argument,
//! second_argument))`.
inline constexpr zip_with_fn zip_with{};

//! \seebelow
//! \constraints For a call `discard_first(first_argument, second_argument)`,
//! let `C` be `remove_cvref_t<decltype((first_argument))>`.
//! `applicative<C>.discard_first(first_argument, second_argument)` is a valid
//! expression.
//! \effects Equivalent to `applicative<C>.discard_first(first_argument,
//! second_argument)`, with each argument forwarded in its own value category.
//! \remarks The name `discard_first` denotes an operation object: it is not
//! found by argument-dependent lookup, and no explicit template argument list
//! may be given in a call to it. The exception specification is equivalent to
//! `noexcept(applicative<C>.discard_first(first_argument, second_argument))`.
inline constexpr discard_first_fn discard_first{};

//! \seebelow
//! \constraints For a call `discard_second(first_argument, second_argument)`,
//! let `C` be `remove_cvref_t<decltype((first_argument))>`.
//! `applicative<C>.discard_second(first_argument, second_argument)` is a valid
//! expression.
//! \effects Equivalent to `applicative<C>.discard_second(first_argument,
//! second_argument)`, with each argument forwarded in its own value category.
//! \remarks The name `discard_second` denotes an operation object: it is not
//! found by argument-dependent lookup, and no explicit template argument list
//! may be given in a call to it. The exception specification is equivalent to
//! `noexcept(applicative<C>.discard_second(first_argument, second_argument))`.
inline constexpr discard_second_fn discard_second{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_APPLY_HPP
