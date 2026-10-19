// include/beman/transpose/monad.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_MONAD_HPP
#define BEMAN_TRANSPOSE_MONAD_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/grade.hpp>

#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose {

// EVIDENCE, NOT PROPOSED WORDING. D3200R0 does not propose a Monad
// abstraction — deferred, not rejected. Everything about the typeclass
// design leads to a consistent generic name for sequential composition
// eventually (today's and_then/transform/let_value are per-type members,
// not generic), and this header is the proof the mechanism carries Monad
// without strain. It is unproposed only because nothing in the coordinated
// set currently needs bind; the slot stays open for the paper that does.
// See D3200R0 "Why not Monad".

namespace detail {

// Stand-ins for the continuations the bind-basis derivations hand to bind.
// A derivation's continuation is a lambda over an element whose type only
// bind knows, so its validity cannot be spelled at the derived member's
// declaration directly. These name it instead: each call operator is
// constrained on what the matching continuation does with the element, and
// spells the continuation's result type, so a bind that checks its
// continuation -- the shipped instances spell their result through
// invoke_result_t on it -- fails the probe softly for a function the
// derivation could not have applied. Like probe_witness, each body
// establishes only the result type.

/** The continuation `fmap` binds with: `pure(std::invoke(function, a))`.
 * `FUNCTION` is invoked as the lvalue the derivation captures.
 */
template <class IMPL, class FUNCTION>
struct fmap_continuation_probe {
    template <class ARGUMENT>
        requires std::invocable<FUNCTION &, ARGUMENT> &&
                 requires(const IMPL &impl) {
                     impl.pure(std::invoke(std::declval<FUNCTION &>(),
                                           std::declval<ARGUMENT>()));
                 }
    constexpr auto operator()(ARGUMENT &&) const
        -> decltype(std::declval<const IMPL &>().pure(std::invoke(
            std::declval<FUNCTION &>(), std::declval<ARGUMENT>()))) {
        std::unreachable();
    }
};

/** The continuation the derived n-ary `invoke` binds its next argument
 * with. `BOUND` holds the elements bound so far, as the lvalues the
 * derivation's lambdas capture them by; `REST` are the arguments still to
 * bind. With none left, the continuation applies the function to the bound
 * elements and this one, and embeds the result with `pure`. Otherwise it
 * binds the next argument with the continuation one step further on.
 */
template <class SELF, class FUNCTION, class BOUND, class... REST>
struct invoke_continuation_probe;

template <class SELF, class FUNCTION, class... BOUND>
struct invoke_continuation_probe<SELF, FUNCTION, std::tuple<BOUND...>> {
    template <class HEAD>
        requires std::invocable<FUNCTION &, BOUND..., HEAD> && requires {
            std::declval<SELF>().pure(std::invoke(std::declval<FUNCTION &>(),
                                                  std::declval<BOUND>()...,
                                                  std::declval<HEAD>()));
        }
    constexpr auto operator()(HEAD &&) const
        -> decltype(std::declval<SELF>().pure(
            std::invoke(std::declval<FUNCTION &>(), std::declval<BOUND>()...,
                        std::declval<HEAD>()))) {
        std::unreachable();
    }
};

template <class SELF, class FUNCTION, class... BOUND, class NEXT, class... REST>
struct invoke_continuation_probe<SELF, FUNCTION, std::tuple<BOUND...>, NEXT,
                                 REST...> {
    template <class HEAD>
        requires requires {
            std::declval<SELF>().bind(
                std::declval<NEXT>(),
                invoke_continuation_probe<
                    SELF, FUNCTION, std::tuple<BOUND..., HEAD &>, REST...>{});
        }
    constexpr auto
    operator()(HEAD &&) const -> decltype(std::declval<SELF>().bind(
        std::declval<NEXT>(),
        invoke_continuation_probe<SELF, FUNCTION, std::tuple<BOUND..., HEAD &>,
                                  REST...>{})) {
        std::unreachable();
    }
};

/** The function the derived `kleisli` returns: `(f >=> g)(a) = bind(f(a),
 * g)`. A named type rather than a lambda so that its call operator can be
 * constrained on that expression; the composition's validity depends on
 * the argument's type, which is known only at the call.
 */
template <class SELF, class F, class G>
struct kleisli_composition {
    SELF &self;
    F f;
    G g;

    template <class ARGUMENT>
        requires requires(SELF &monad, const F &first, const G &second) {
            monad.bind(first(std::declval<ARGUMENT>()), second);
        }
    constexpr auto operator()(ARGUMENT &&argument) const {
        return self.bind(f(std::forward<ARGUMENT>(argument)), g);
    }
};

} // namespace detail

/** CRTP base for Monad instances.
 * `Impl` must provide `pure(value)` and `bind(ma, f)`.
 * The n-ary `invoke` is synthesized; `join` and `kleisli` are derived.
 * Monad does not inherit from Applicative, but provides the equivalent
 * applicative operation once `invoke` is synthesized from `bind` + `pure`.
 */
template <class Impl>
struct derive_monad : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No monad<T> specialization found. "
                  "Specialize beman::transpose::monad<T> for your "
                  "type T and provide pure(...) and bind(...) operations.");

    using Impl::bind;
    using Impl::pure;

    /** fmap: the Functor basis, grounded in bind + pure.
     *
     *   fmap(f, ma) = ma >>= (pure . f)
     *
     * A monad is a functor, and this is that theorem spelled as an operation
     * rather than as a superclass constraint. Prefers a native Impl::fmap
     * when the instance supplies one -- an instance that can map without
     * sequencing usually should. The full Functor instance is spelled at the
     * registration site as derive_functor<ThisMap>{}; Monad grows the Functor
     * BASIS only, so Functor's derived surface stays Functor's.
     */
    template <class FUNCTION, class MA>
    constexpr auto fmap(this auto &&self, FUNCTION &&function, MA &&ma)
        requires requires(const Impl &impl) {
            impl.fmap(std::forward<FUNCTION>(function), std::forward<MA>(ma));
        } || requires(const Impl &impl) {
            // The continuation the body binds with, pure . function -- not
            // function itself, which a bind would accept only if it already
            // returned a monadic value.
            impl.bind(std::forward<MA>(ma),
                      detail::fmap_continuation_probe<Impl, FUNCTION>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).fmap(std::forward<FUNCTION>(function),
                                             std::forward<MA>(ma));
                      }) {
            return impl_of(self).fmap(std::forward<FUNCTION>(function),
                                      std::forward<MA>(ma));
        } else {
            return impl_of(self).bind(std::forward<MA>(ma), [&](auto &&a) {
                return impl_of(self).pure(
                    std::invoke(function, std::forward<decltype(a)>(a)));
            });
        }
    }

    // invoke: n-ary lift synthesized from bind + pure (left-nested binds):
    //   invoke(f, m1, ..., mn) = m1 >>= \a1 -> ... mn >>= \an ->
    //   pure(f(a1...an))
    // Prefers a native Impl::invoke when present, like Applicative. This
    // derivation assumes a synchronous bind (the continuation is invoked
    // before bind returns), which holds for every Monad instance in this
    // repository.
    template <class FUNCTION, class FIRST, class... REST>
    constexpr auto invoke(this auto &&self, FUNCTION &&function, FIRST &&first,
                          REST &&...rest)
        requires requires(const Impl &impl) {
            impl.invoke(std::forward<FUNCTION>(function),
                        std::forward<FIRST>(first),
                        std::forward<REST>(rest)...);
        } || requires {
            // The derivation's own requirement: binding FIRST, then each of
            // REST in turn, reaches a continuation that can apply FUNCTION
            // to every element and embed the result with pure. The
            // continuation probe spells each step, so a function that
            // cannot take the elements fails here rather than in the body.
            // The probe follows the derived path all the way down, as the
            // body does once it has not found a native Impl::invoke.
            self.bind(
                std::forward<FIRST>(first),
                detail::invoke_continuation_probe<decltype(self), FUNCTION,
                                                  std::tuple<>, REST...>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).invoke(std::forward<FUNCTION>(function),
                                               std::forward<FIRST>(first),
                                               std::forward<REST>(rest)...);
                      }) {
            return impl_of(self).invoke(std::forward<FUNCTION>(function),
                                        std::forward<FIRST>(first),
                                        std::forward<REST>(rest)...);
        } else {
            return self.bind(std::forward<FIRST>(first), [&](auto &&head) {
                if constexpr (sizeof...(REST) == 0) {
                    return self.pure(std::invoke(
                        function, std::forward<decltype(head)>(head)));
                } else {
                    return self.invoke(
                        [&function, &head](auto &&...tail) {
                            return std::invoke(
                                function, head,
                                std::forward<decltype(tail)>(tail)...);
                        },
                        std::forward<REST>(rest)...);
                }
            });
        }
    }

    // join: flatten nested monad.
    // join mma = mma >>= id
    // Prefers a native Impl::join. join/bind is a mutually-derivable pair
    // (join = bind(., id); bind is recoverable from join + fmap), so the
    // fallback addresses Impl directly, matching fmap's bind-basis branch --
    // the one member in this step where the derivation, not just the probe,
    // addresses Impl rather than self.
    template <class MMA>
    constexpr auto join(this auto &&self, MMA &&mma)
        requires requires(const Impl &impl) {
            impl.join(std::forward<MMA>(mma));
        } || requires(const Impl &impl) {
            impl.bind(std::forward<MMA>(mma),
                      [](auto &&inner) { return inner; });
        }
    {
        if constexpr (requires {
                          impl_of(self).join(std::forward<MMA>(mma));
                      }) {
            return impl_of(self).join(std::forward<MMA>(mma));
        } else {
            return impl_of(self).bind(std::forward<MMA>(mma),
                                      [](auto &&inner) { return inner; });
        }
    }

    // kleisli: forward Kleisli composition (>=>).
    // (f >=> g) a = f a >>= g
    // Prefers a native Impl::kleisli. Whether the composition is valid
    // depends on the type of "a", which is not known until the composed
    // function is called, so nothing about f and g can be checked here. The
    // derived kleisli therefore requires only what building the composition
    // needs, and the composition's call operator carries the real
    // constraint: calling it with an argument f cannot take, or whose
    // result g cannot continue, fails softly at the call.
    template <class F, class G>
    constexpr auto kleisli(this auto &&self, F f, G g)
        requires requires(const Impl &impl) { impl.kleisli(f, g); } ||
                 (std::move_constructible<F> && std::move_constructible<G>)
    {
        if constexpr (requires { impl_of(self).kleisli(f, g); }) {
            return impl_of(self).kleisli(f, g);
        } else {
            return detail::kleisli_composition<
                std::remove_reference_t<decltype(self)>, F, G>{
                self, std::move(f), std::move(g)};
        }
    }

    /** ap: one-step contextual application, derived from bind + pure.
     *
     *   ap(mf, ma) = mf >>= \f -> ma >>= \a -> pure(f a)
     *
     * The Applicative base derives ap from its n-ary invoke; a monad-derived
     * instance has no invoke to derive from until one is synthesized, so it
     * gets ap from the bind basis instead. Both derivations exist exactly
     * when the context can hold a callable, and the constraint says so, so
     * the member disappears for contexts that cannot (std::simd::vec) rather
     * than failing inside the body.
     */
    template <class MF, class MA>
    constexpr auto ap(this auto &&self, MF &&mf, MA &&ma)
        requires requires(const Impl &impl) {
            impl.ap(std::forward<MF>(mf), std::forward<MA>(ma));
        } || requires {
            typename applicative_value_t<MF>;
            typename applicative_value_t<MA>;
            requires std::invocable<const applicative_value_t<MF> &,
                                    const applicative_value_t<MA> &>;
        }
    {
        if constexpr (requires {
                          impl_of(self).ap(std::forward<MF>(mf),
                                           std::forward<MA>(ma));
                      }) {
            return impl_of(self).ap(std::forward<MF>(mf), std::forward<MA>(ma));
        } else {
            return self.bind(std::forward<MF>(mf), [&self,
                                                    &ma](auto &&function) {
                return self.bind(ma, [&self, &function](auto &&argument) {
                    return self.pure(std::invoke(
                        function, std::forward<decltype(argument)>(argument)));
                });
            });
        }
    }

    /** Uses a value at a wider grade, defaulted from the grade algebra.
     *
     * The same defaulted subsumption the Applicative base carries; see the
     * note there. Bind is the operation that joins grades, so the monad is
     * where widening is reached for most often.
     */
    template <class TARGET_GRADE, class CARRIER>
    constexpr auto subsume(this auto &&, CARRIER &&value)
        requires requires {
            grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
        }
    {
        return grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
    }

    // bind_with: explicit monad object override.
    template <class MONAD_MAP, class MA, class F>
    constexpr auto bind_with(this auto &&, const MONAD_MAP &monad_map, MA &&ma,
                             F &&f)
        requires requires {
            monad_map.bind(std::forward<MA>(ma), std::forward<F>(f));
        }
    {
        return monad_map.bind(std::forward<MA>(ma), std::forward<F>(f));
    }

    /** The full Functor instance over this monad object.
     *
     * Every typeclass object is stateless and empty, so constructing the
     * full instance and "converting" are the same free type-level move.
     * This is `Monad m => Functor m` superclass subsumption, paid for with
     * one visible call that names which functor is meant, instead of a
     * std::remove_cvref_t incantation at the call site:
     *
     *     f(monad_map.as_functor(), xs);
     *
     * The functor it returns is the one derived from the object in hand,
     * law-compatible with that object's bind by construction. It
     * deliberately does NOT consult functor<T>: a caller who
     * wants the registered default says so by looking it up.
     */
    constexpr auto as_functor(this auto &&self) {
        return derive_functor<std::remove_cvref_t<decltype(self)>>{};
    }

  private:
    //! \omit
    template <class SELF>
    static constexpr decltype(auto) impl_of(SELF &&self) {
        return static_cast<impl_ref_t<Impl, SELF>>(self);
    }
};

/** Typeclass lookup variable for Monad; specialize for each type. */
template <class T>
inline constexpr auto monad = std::false_type{};

/** Restricted `Impl` concept for Monad: satisfied when `IMPL` supplies the
 * one minimal complete basis the `Monad` CRTP base admits today -- `pure`
 * and `bind`. Monad's other complete bases -- `pure` + `fmap` + `join`, and
 * `pure` + `kleisli` -- are deliberately not admitted here; extending this
 * concept to accept them is a separate, unscheduled piece of work. This is
 * the `MINIMAL` pragma to `monad_object`'s class declaration: `fmap`,
 * `invoke`, `join`, `kleisli`, `ap`, `subsume` and `bind_with` are all
 * derived and belong to `monad_object` alone.
 */
template <class IMPL, class CONTEXT>
concept monad_impl = requires(const IMPL &impl, const CONTEXT &context,
                              const applicative_value_t<CONTEXT> &element) {
    impl.pure(element);
    impl.bind(context, detail::probe_witness<CONTEXT>{});
};

/** Deep object concept for a Monad object over `CONTEXT`: satisfied when
 * `OBJ` provides the full object surface -- `pure`, `bind`, `fmap`,
 * `invoke`, `join`, `kleisli` and `bind_with`. `ap` and `subsume` are
 * required only where their own condition licenses them, mirroring
 * `applicative_object`'s treatment.
 *
 * `kleisli` is probed for existence -- the class surface names it -- but its
 * presence is never load-bearing evidence here: `derive_monad<Impl>::kleisli`
 * requires only that `f` and `g` can be moved into the composition, because
 * its `bind` call lives inside the returned composition, whose argument type
 * is unknown until it is called. The composition's own call operator is
 * where that `bind` is checked. It is `bind` and `join` (and `fmap`,
 * `invoke`) that carry a real either-basis condition and do the actual
 * discriminating.
 */
template <class OBJ, class CONTEXT>
concept monad_object =
    requires(const OBJ &obj, const CONTEXT &context,
             const applicative_value_t<CONTEXT> &element) {
        obj.pure(element);
        obj.bind(context, detail::probe_witness<CONTEXT>{});
        obj.fmap(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
        obj.invoke(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
        obj.join(obj.pure(context));
        obj.kleisli(detail::probe_witness<CONTEXT>{}, detail::probe_witness<CONTEXT>{});
        obj.bind_with(obj, context, detail::probe_witness<CONTEXT>{});
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

// -- Operation objects --
//
// See functor.hpp for the shape, the motivation, and why the namespace is
// nested. Still evidence, not proposed wording -- but evidence of one more
// thing now: that the operation-object layer is uniform across the whole
// typeclass family, including the member nobody is proposing. If the spelling
// only worked for the operations D3200R0 happens to propose, that would say
// it had been fitted to those operations instead of to the mechanism.
//
// It also settles a naming question the free-function API had ducked. `bind`
// could not be a function template here without reading as a competitor to
// std::bind; the old spelling was `mbind`, a wart adopted to dodge exactly
// that. An object needs no such dodge: `bind` is a variable, it is
// never found by ADL, and it never enters overload resolution with std::bind.
// The typeclass operation gets its real name.

/** Operation object for Monad's `bind` primitive.
 *
 * The context keys the lookup and is deduced from the first argument.
 */
struct bind_fn {
    /** Sequences a monadic value `ma` through `f` (Haskell's `>>=`).
     *
     * @tparam TC  The Monad instance, from the lookup for the keying argument.
     * Not for callers to supply; pin with mode 2.
     */
    template <class MA, class F,
              const auto &TC = monad<std::remove_cvref_t<MA>>>
        requires requires { TC.bind(std::declval<MA>(), std::declval<F>()); }
    constexpr auto operator()(MA &&ma, F &&f) const {
        return TC.bind(std::forward<MA>(ma), std::forward<F>(f));
    }
};

/** Sequential composition: `bind(ma, f)`. */
inline constexpr bind_fn bind{};

/** Operation object for Monad's derived `join`. */
struct join_fn {
    /** Flattens a nested monadic value; equivalent to `bind(mma, id)`. */
    template <class MMA, const auto &TC = monad<std::remove_cvref_t<MMA>>>
        requires requires { TC.join(std::declval<MMA>()); }
    constexpr auto operator()(MMA &&mma) const {
        return TC.join(std::forward<MMA>(mma));
    }
};

/** Flattens one level of nesting: `join(mma)`. */
inline constexpr join_fn join{};

/** Operation object for Monad's derived `kleisli` composition.
 *
 * Neither argument is in-context -- both are functions returning a monadic
 * value -- so, like `pure`, the context cannot be deduced and is named:
 * `kleisli<std::optional<int>>(f, g)`. Two of the family's operations fall
 * out of the deducing pattern this way, and they are exactly the two that
 * produce a context instead of consuming one.
 */
template <class CONTEXT, const auto &TC = monad<std::remove_cvref_t<CONTEXT>>>
struct kleisli_fn {
    /** Forward Kleisli composition: `(f >=> g) a = f a >>= g`. */
    template <class F, class G>
        requires requires { TC.kleisli(std::declval<F>(), std::declval<G>()); }
    constexpr auto operator()(F &&f, G &&g) const {
        return TC.kleisli(std::forward<F>(f), std::forward<G>(g));
    }
};

/** Kleisli composition in a named context: `kleisli<C>(f, g)`. */
template <class CONTEXT>
inline constexpr kleisli_fn<CONTEXT> kleisli{};

// -- Free-function API --

/** Sequences a monadic value `ma` through function `f` (Haskell's `>>=`).
 *
 * Retained for source compatibility; `bind` is the spelling to use.
 */
template <class MA, class F>
auto mbind(MA &&ma, F &&f) {
    return bind(std::forward<MA>(ma), std::forward<F>(f));
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_MONAD_HPP
