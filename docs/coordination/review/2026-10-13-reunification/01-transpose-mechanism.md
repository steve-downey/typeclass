# Review 01 — The typeclass mechanism in `beman.transpose`

Scope: `src/beman/transpose/{functor,apply,traverse,transpose,monoid,induced_monoid,fold,monad,laws,overloaded}.hpp`, `detail/`, the instance headers, `papers/D3200R0.md`, `docs/transpose/typeclass-object-pattern.md`, the `.org` explainers, and the named slugs in `docs/transpose/decisions.md`. Repository read-only. Every claim marked **[verified]** was checked by reading the cited lines or by a compile probe (clang 18.1.3, `-std=c++23`, probe sources in my scratchpad; gcc 13 on this box is below the library's GCC 14 floor); claims marked **[inferred]** follow from the standard or from reading, without a probe.

## Executive summary

1. The mechanism is: an `inline constexpr` **variable template per typeclass, primary `= std::false_type{}`**, specialized (full or partial, optionally constrained) to an object whose type is `Map : derive_X<Impl>`, where `derive_X : protected Impl` supplies derived operations by probe-then-derive. Lookup is **template partial-specialization matching plus partial ordering**, not overload resolution; overload resolution appears only inside Impl member overload sets and the operation objects' `operator()` sets. This distinction matters for the planned Haskell/Lean comparison and is not stated anywhere in the docs.
2. Coherence is "one object per concrete type per typeclass per program", enforced by nothing stronger than the ODR. All instances are orphan instances (registration reopens `namespace beman::transpose`). The library itself needed a second registry (`accumulating_applicative`) to escape one-instance-per-type, which is the first place the model shows strain.
3. The superclass story is contradicted by the code: `derive_applicative` provides `map`, `derive_functor` provides `fmap`; a type with only an Applicative instance (`std::array`, `std::expected`) has no `fmap`, while functor.hpp:140-144 and D3200R0:396-400 assert that Applicative "provides the Functor names". Monad objects fail both `functor_object` and `applicative_object` **[verified]**.
4. The monoid design has a split personality: `registered-instance-state` permits a stateful registered `Monoid<T>` and says every defaulted fold shares it, but every defaulted fold constructs a fresh `Monoid<R>{}` and never touches `monoid<R>` **[verified by running a probe]**. Typeclass objects are elsewhere assumed empty and default-constructed (`POLICY{}`, `MONAD_OBJECT{}`).
5. Costs imposed on fingertree / tree_algorithms by transpose's domains: keying on the full type (no type-constructor/rebind notion, so fingertree's `traverse` drops MONOID, MEASURE_POLICY and ALLOCATOR), two incompatible element-type mechanisms (`Impl::element_type` vs `T::value_type` via a trait named `applicative_value`), allocator blindness (`vector<T, pmr>` is unregistered **[verified]**), grade machinery (`subsume`, `graded_context`) in the proposed Applicative core, and a `fold_map` whose shape in tree_algorithms (`map_fn, combine, identity`) does not use transpose's `monoid_object` vocabulary despite the coherence skill saying it must.
6. Repo vs paper: `functor.hpp` and `monoid.hpp` (plus `simd.hpp`) are **shipped-but-silent** — unproposed, no evidence banner, and `monoid.hpp`/`functor.hpp` contents are not listed in the paper's Non-goals. `detail/typeclass_base.hpp` exports public-namespace names (`has_instance_v`, `applicative_value_t`, `always_false_v`, `impl_ref_t`) that a consumer library depends on.
7. The `proposal-name-case` rule ("case says whether a name is proposed") is false in the code: `fold.hpp` and the `*_monoid` family are unproposed and snake_case.
8. Diagnostics for an unregistered type are a bare constraint failure ("no member named 'fmap' in integral_constant<bool,false>"); the `static_assert` messages in every `derive_*` base are unreachable from lookup failure. Compile cost of the core headers is small (~0.1 s over a std-header baseline with clang 18).
9. Several docs and header comments are stale: `docs/decisions.md#…` links (should be `docs/transpose/decisions.md`), `include/…`/`tests/…` paths, `dual_monoid.hpp`, `*_typeclass<T>`, "pure + apply".

---

## 1. The mechanism, precisely

### 1.1 Registration

Three layers plus a registration, uniform across the five typeclasses:

| Layer | What the author writes | Evidence |
|---|---|---|
| **Impl** | a struct with the basis operations as member templates (deducing-this style, `this auto &&` unused) and, for Foldable/Traversable, `using element_type = …;` | `functor.hpp:266-278` (`OptionalFunctorImpl`), `apply.hpp:488-503`, `sequence.hpp:38-73, 93-115` |
| **CRTP base** | `derive_functor<Impl> : protected Impl` etc.; owns derived operations | `functor.hpp:45-96`, `apply.hpp:136-269`, `traverse.hpp:95-152`, `fold.hpp:182-647`, `monad.hpp:34-265` |
| **Map** | `struct XMap : derive_X<XImpl> { using XImpl::basis_op; }` — the `using` re-exports the Impl's basis because `Impl` is a *protected* base | `functor.hpp:301-309`, `apply.hpp:505-511`, `sequence.hpp:117-122` |
| **Registration** | a specialization of the lookup variable template to a value of Map type | `functor.hpp:311-319`, `apply.hpp:513-516`, `traverse.hpp:161-162` + `sequence.hpp:124-127` |

The lookup variables and their primaries:

- `template <class T> inline constexpr auto functor = std::false_type{};` — `functor.hpp:101-102`
- `applicative`, `accumulating_applicative` — `apply.hpp:278-279, 293-294`
- `traversable` — `traverse.hpp:161-162`
- `foldable` — `fold.hpp:651-652`
- `monad` — `monad.hpp:268-269`
- **Monoid is the odd one out**: a *class* template `template <class VALUE_TYPE> struct Monoid;` (declared, never defined) plus `inline constexpr Monoid<VALUE_TYPE> monoid = Monoid<VALUE_TYPE>{};` — `monoid.hpp:43-49`. Registration is a struct specialization (`monoid.hpp:250-275`), and "is registered" is `requires { Monoid<T>{}; }` (`monoid.hpp:76-77`), i.e. default-constructibility of the specialization.

Registration keys on the **complete type** (`std::optional<VALUE_TYPE>`, `std::vector<VALUE_TYPE>`, `FingerTree<T, MONOID, MEASURE_POLICY, ALLOCATOR>` at `finger_tree_foldable.hpp:48-51`), never on a type constructor. One exception in design: the P2300 adapter keys by **constraint**, a constrained partial specialization `template <class S> requires single_value_sender<S> … applicative<S>` (`decisions.md:2147-2165`, sender-instance-keying), with a one-object-for-all-senders Map.

The Map is templated on the element type even though its operations are member templates generic in the callable; e.g. `VectorFunctorImpl<VALUE_TYPE>::fmap` takes `const std::vector<VALUE_TYPE>&` (`functor.hpp:283-284`) so the parameter is load-bearing only for the parameter type. **[verified]**

### 1.2 Lookup

Mode 1: `functor<std::optional<int>>.fmap(f, v)` — direct template-id. Mode 2: pass the object as an argument. Mode 3: NTTP pin, `template <class C, const auto &TC = bt::functor<C>>` (`examples/transpose/lookup_modes_example.cpp:17-20`). Mode 4: **operation objects** — `struct fmap_fn { template <class F, class T, const auto &TC = functor<std::remove_cvref_t<T>>> requires requires { TC.fmap(declval<F>(), declval<T>()); } constexpr auto operator()(F&&, T&&) const; }; inline constexpr fmap_fn fmap{};` (`functor.hpp:201-216`). The keying argument is designated per operation (`apply.hpp:319-320` first in-context argument; `ap` keys on the *argument*, not the callable, `apply.hpp:344-346`). Two operations cannot deduce and are variable templates of objects: `pure<C>(v)` (`apply.hpp:367-376, 875-876`) and `kleisli<C>(f, g)` (`monad.hpp:417-429`). `traverse` and `transpose` are objects but do not use the trailing-NTTP shape; they look up inside the body (`traverse.hpp:531-542`, `transpose.hpp:43-50`).

Detection: `has_instance_v<decltype(functor<T>)>` is `!is_same_v<remove_cvref_t<…>, std::false_type>` (`detail/typeclass_base.hpp:75-77`). Operation objects do not use it; they are constrained on the call (`decisions.md:3338-3346`, operation-object-wording pt 1).

Because the primary is `std::false_type{}`, **naming `functor<T>` for an unregistered T instantiates the primary**. A later specialization in the same TU is then ill-formed ("explicit specialization of 'functor' after instantiation", probe p5 **[verified]**). Across TUs the same situation is an ODR violation with no diagnostic **[inferred, standard]**.

### 1.3 Derived operations

Every derived member follows one shape (`decisions.md:1369-1385`, derived-op-native-preference): a disjunctive trailing `requires` — "Impl has a native `op`" `||` "the derivation's own requirement" — and a body `if constexpr (requires { impl_of(self).op(...) }) return impl_of(self).op(...); else derive;`. `impl_of` is a private static in each base doing `static_cast<impl_ref_t<Impl, SELF>>(self)` (`functor.hpp:92-95`; `impl_ref_t` at `detail/typeclass_base.hpp:58-61`), because `Impl` is a protected base and only a member of the owning base may cast (`decisions.md:1193-1230`). Mutually-derivable pairs (`invoke`/`ap`, `fold_map`/`fold_right`, `bind`/`join`) address `Impl` directly to break cycles; one-way derivations route through `self` so a shadow on an outer wrapper is reached (`functor.hpp:80-87`, `apply.hpp:631-638`, `fold.hpp:191-204`). Applicative's dual basis: `invoke` native or derived by currying through `detail::terminating_partial` (`apply.hpp:547-561`, `detail/applicative_derivation.hpp:27-87`); `ap` native or `invoke(applicative_eval, cf, cx)` (`apply.hpp:577-604`).

Monad grows the Functor *basis* (`fmap` from `bind`+`pure`, `monad.hpp:55-75`) but not the derived surface; the full Functor object is `derive_functor<SomeMonadMap>{}`, offered as `as_functor()` (`monad.hpp:255-257`, decision functor-monad-grounding `decisions.md:1286-1300`).

### 1.4 The `_with` / explicit tier

- Trailing explicit-instance parameters: `traverse(f, value, policy = POLICY{})` (`traverse.hpp:531-542`) and `fold_map(f, structure, monoid)` / `combine_all(structure, monoid)` (`fold.hpp:209-250, 410-429`) — the monoid is a *required* trailing instance unless the result type has a registered `Monoid` (`fold.hpp:258-281`).
- Delegation members: `traverse_with(traversable_map, f, value)` and `traverse_with(traversable_map, applicative_map, f, value)`, `transpose_with(traversable_map, value)` (`traverse.hpp:128-144, 484-517`); `invoke_with(applicative_map, f, args...)` and the NTTP form `invoke_with<APPLICATIVE_MAP>(f, args...)` (`apply.hpp:228-240, 809-833`); `bind_with(monad_map, ma, f)` (`monad.hpp:233-238`). These are members *of an instance* that ignore `self` and call another instance; both `traverse_with` and `invoke_with` are in the proposed wording (`papers/wording/transpose.applicative.delegate.md`, `transpose.traversable.delegate.md`).
- No pinned `_with` *operation objects*: tried and removed (`decisions.md:3225-3228`, operation-objects pt 2; D3200R0:377-384).

### 1.5 Monoid selection for a carrier with several monoids

Decided in `decisions.md:2964-3172` (monoid-selection): no `Monoid<int>`/`Monoid<bool>` registration; the caller passes an **instance object over the bare type** (`sum_monoid<T>`, `max_monoid<T>`, `any_monoid`, … `monoid.hpp:86-195`), combinators `dual_monoid<M>`, `tuple_monoid<M...>` hold operands by value (`monoid.hpp:207-243`). The two-argument `fold_map(f, v)` exists only when `Impl::element_type` is declared and `has_registered_monoid<fold_result_t<Impl, F>>` (`fold.hpp:258-268`); otherwise the call is not a candidate. `monoid_object<M, T>` is the concept (`monoid.hpp:61-65`). Instances may carry state, copies permitted (`decisions.md:3024-3059`).

### 1.6 Minimal end-to-end example (from the code)

`crtp_absorption.test.cpp:43-96` is the smallest complete registration in the tree: a context `boxed<V>` with `value_type`; an Impl with `pure` and n-ary `invoke` (trailing return types, `this auto &&`); `struct BoxedApplicativeMap : derive_applicative<BoxedApplicativeImpl<V>> { using Impl::invoke; using Impl::pure; };` and, **inside `namespace beman::transpose`**, `template <class V> inline constexpr auto applicative<boxed<V>> = BoxedApplicativeMap<V>{};`. Use: `bt::applicative<boxed<int>>.map(f, boxed<int>{6})` and `bt::traverse(boxed_of, std::vector<int>{1,2,3})` yielding `boxed<std::vector<int>>` (`crtp_absorption.test.cpp:160-171`).

---

## 2. Evaluation on its own terms

### F1 — Lookup is specialization matching, not overload resolution (note, but load-bearing for the research plan)

**Evidence.** `functor.hpp:101-102, 311-319`; probe p1/p2. Partial ordering between `functor<std::vector<T>>` and a constrained `functor<T> requires std::ranges::range<T>` picks the concrete form **[verified p1]**; two constrained generics that both match yield "ambiguous partial specializations of 'functor<std::list<int>>'", surfaced through the operation object as a substitution failure **[verified p2]**. Constraints only break ties when neither specialization is more specialized by deduction ([temp.class.order] via [temp.func.order]) **[inferred]**.

**Why it matters.** The owner's stated research compares "C++ overload resolution as the registration/lookup mechanism" with Haskell overlapping instances and Lean instance resolution. In this code the registry is a *variable template* and the resolution step is *partial-specialization partial ordering* — a strictly weaker, non-backtracking, no-priority relation than Lean's priority-ordered synthesis and closer to GHC's `OVERLAPPING`/`OVERLAPPABLE` only in that most-specific-by-deduction wins; there is no equivalent of `IncoherentInstances`, no instance priority, no local instances, and constraint subsumption is the only overlap tie-breaker. Overload resolution does occur, but one level down: inside an Impl's own overload set (the three `invoke` cores of `expected_applicative_impl`, `expected.hpp:160-233`, kept disjoint by complementary constraints by design, `expected.hpp:194-199`) and in operation-object `operator()` sets (`fold_map_fn`, `fold.hpp:690-707`). The docs never draw this line (`typeclass-object-a-new-extension-point.org:157-171` says only "why variable template, not class template").

**Recommendation.** Add a decision entry (there is **no slug for the registration mechanism itself**: `grep -n "^## " decisions.md` shows no `lookup`/`registration` slug; the only rationale is the `.org` explainer) stating: registry = variable template; resolution = partial ordering; constraints = tie-breaker only; overload resolution = within an instance. Frame the comparison paper around *that*, not around overload resolution.

### F2 — Coherence is ODR-grade; all instances are orphans (significant)

**Evidence.** Registration always reopens `namespace beman::transpose` from the consumer (`finger_tree_foldable.hpp:45-51`, `tree_algorithms/binary_tree.hpp:208-215`, `crtp_absorption.test.cpp:61-96`). The paper describes this as "one object per type per concept" (D3200R0:291-293). Two specializations of the same variable in one TU are a redefinition error; in two TUs they are IFNDR **[inferred]**. Use-before-specialize is a hard error in one TU (p5 **[verified]**) and silent across TUs. transpose itself registers `functor/foldable/traversable<std::vector<T>>` and `Monoid<std::vector<T>>` (`functor.hpp:316-319`, `sequence.hpp:80-82, 125-127`, `monoid.hpp:263-275`): any downstream library wanting a different vector instance cannot have one.

**Why it matters.** Haskell's orphan-instance warning and Rust's orphan rule exist precisely because this is the failure mode of global, type-keyed registries. Nothing here even *detects* the collision. The paper's standardization story ("`std::typeclass::applicative<T>`") makes `std` the owner of every instance for `std` types, which is the right coherence posture — but then no user can ever register, say, a zip applicative for `std::vector` or a list applicative for `std::optional`-of-lists, and the library's own answer (a second registry, `accumulating_applicative`, `apply.hpp:293-294`) does not generalize.

**Recommendation.** State the coherence rule explicitly in D3200R0 (who may specialize `applicative<T>` for which `T`, mirroring [namespace.std]/2's rule for program-defined types), and record the cross-TU hazard. Consider whether named instances (mode 2 / NTTP) are the *intended* answer for alternate disciplines so that the registry is only ever a default — which is what fingertree in fact does with its MONOID template parameter.

### F3 — The superclass encoding is contradicted by the code (significant)

**Evidence.** `derive_applicative` has `map`/`lift` (`apply.hpp:164-179`), no `fmap`; `derive_functor` has `fmap`/`replace` (`functor.hpp:48-88`), no `map`. `functor<std::expected<…>>` and `functor<std::array<…>>` are not registered (grep; probe p6: `!is_invocable_v<fmap_fn, …, std::array<int,3>>` **[verified]**). `monad<optional>` satisfies neither `functor_object` nor `applicative_object` **[verified p6]**. Yet `functor.hpp:140-144` and D3200R0:396-400 say "`fmap` is Functor's map whichever instance supplies it, and Applicative is free to provide the Functor names because they can not mean anything else", and `operation-objects-and-the-lookup-modes.org` (section "One namespace") says "`map` and `lift` are spelled `fmap` and `pure`. An alias doesn't need a second name."

**Why it matters.** The design's stated justification for a single namespace is that one name means one thing across the family; the family then uses two names for Functor's map. A generic algorithm written over `functor<T>` cannot be fed an `expected`, and one written over `applicative<T>.map` cannot be fed a `vector`. The deliberate "no superclass edge" choice (`functor.hpp:252-257`, `decisions.md:1567-1590`) is defensible (structural conformance), but then *something* must make an Applicative-registered type reachable as a Functor. Today only Monad has that bridge (`as_functor()`), and it builds a fresh object rather than consulting the registry.

**Recommendation.** Either rename `derive_applicative::map` to `fmap` and make `derive_applicative` satisfy `functor_object`, or give the Functor lookup a fallback partial specialization that derives from a registered applicative (constrained on `has_instance_v<decltype(applicative<T>)>`). Fix the explainer sentence either way.

### F4 — Stateful registered instances are permitted on paper and ignored in code (significant)

**Evidence.** `decisions.md:3174-3208` (registered-instance-state): "`monoid_v<T>` is one `inline constexpr` object per type per program, shared by every fold that defaults to it… Instrumentation… is a legitimate use." But every defaulted path constructs `Monoid<R>{}` afresh: `fold.hpp:267, 279` (two-argument `fold_map`), `fold.hpp:588, 599` (`to_vector`), `induced_monoid.hpp:108` (`ELEMENT_MONOID d_element{}`); `monoid<T>` is used only by `monoid_identity`/`monoid_combine` (`monoid.hpp:277-288`). Probe p7 registered `Monoid<Tag>` holding an `int*` counter, specialized `monoid<Tag>` to point at a global, and called `combine_all(vector<Tag>)`: the counter stayed 0 **[verified by execution]**. The same stateless assumption is hard-coded for typeclass objects: `POLICY policy = POLICY{}` (`traverse.hpp:537`), `MONAD_OBJECT{}`/`APPLICATIVE_OBJECT{}` (`induced_monoid.hpp:69, 75, 111, 115` with the comment "every typeclass object in this library is stateless and empty"), `as_functor()` (`monad.hpp:256`), and the wording promises objects model `semiregular` (`traverse.hpp:158-160`, checked at `objects.test.cpp:162-166`).

**Why it matters.** Either registered instances are values (then `monoid<T>` must be *the* object the library uses and the defaulted paths must read it), or they are types (then `monoid<T>` is redundant and the decision text is wrong). For fingertree, which keys trees on the monoid *type* and stores an instance (`finger_tree.hpp:296-300`), the decision's consequence paragraph ("keying on the instance type checks the type, not the state", `decisions.md:3113-3118`) is the honest answer, and the library should match it.

**Recommendation.** Pick one. Simplest: delete the state allowance for *registered* instances, keep it for *passed* instances, and have the defaulted paths spell `monoid<R>` (so a reader sees the registry used). Record it under the existing slug.

### F5 — Two element-type mechanisms, one of them misnamed (significant)

**Evidence.** Traversable/Foldable require `Impl::element_type` (`traverse.hpp:103-110`, `fold.hpp:71-79, 258-268`). The deep concepts and Applicative use `applicative_value_t<T>` = `T::value_type` (`detail/typeclass_base.hpp:79-100`), including in `functor_object` (`functor.hpp:259-264`) and `foldable_object` (`fold.hpp:890`). A type with a Functor instance but no `value_type` — every tree_algorithms layer functor, e.g. `BinaryTreeF<T, A>` — fails `functor_object` while `has_instance_v` is true **[verified p6]**. fingertree's Rope/RandomAccess/IntervalIndex wrappers lack `value_type` (`machinery-deduplication.md`, stage 2 bullet "add `value_type` or don't assert the concept on them").

**Why it matters.** A trait named `applicative_value` is the element-type oracle for Functor and Foldable too; its `value_type` convention comes from `optional`/`expected`/`vector`, i.e. from transpose's domains. Recursion-scheme layers (`F<A>` with `A` the recursive position, no `value_type`) and container wrappers do not fit, so the conformance concepts cannot describe the two consumer libraries' instances.

**Recommendation.** One element-type hook, named for what it is (`element_t<T>`), with a fallback chain: `Impl::element_type` → `T::value_type` → customizable trait. Let the Functor concept take the element type as a parameter rather than computing it.

### F6 — Keying on the complete type: no type constructor, no rebind (significant; see also §3)

**Evidence.** `pure<std::optional<int>>(42)` must name the full type (`apply.hpp:358-365`); `OptionalFunctorMap<VALUE_TYPE>` is per element type although `fmap` is a member template. The sender decision shows the alternative and its cost: one object for all `S`, so `pure` cannot return `S` and `applicative_object_for` fails for every sender except `just(x)` (`decisions.md:2247-2270`).

**Why it matters.** C++ has no higher-kinded key, so the design keys on `optional<int>` where Haskell keys on `Maybe`. The consequence is that the "same constructor, different element" relation — the whole content of Functor — is nowhere represented, and so the library has no rebinding operation for a consumer to customise (§3.1).

### F7 — Partial ordering as overlap: adequate for concrete-vs-generic, fragile for generic-vs-generic (minor)

**Evidence.** p1/p2 above; sender-instance-keying's tripwire "ambiguity… must be resolved by subsumption, never by adding a tie-breaker tag" (`decisions.md:2174-2176`). **Why it matters.** Once two constraint-keyed registrations exist (a `range<T>` functor and a `sender<T>` applicative are different variables, so fine; but two *range*-ish functors are not), users get an ambiguity they can only fix by editing one of the two libraries' constraints. **Recommendation.** Document that constraint-keyed registrations are reserved for the library (the std story) or must be mutually subsuming; the paper should say so where it introduces `applicative<T>` specialization.

### F8 — Error messages: detectability bought by giving up the message; dead `static_assert`s (minor)

**Evidence.** Unregistered type through an operation object: "no matching function… constraints not satisfied… because 'TC.fmap(…)' would be invalid: no member named 'fmap' in 'std::integral_constant<bool, false>'" (p3 **[verified]**); through the variable: "no member named 'fmap' in 'std::integral_constant<bool, false>'" (p4). A callable that does not accept the element: "no type named 'type' in 'std::invoke_result<…>'" at `traverse.hpp:536` (p12). The `derive_*` bases' helpful messages (`apply.hpp:138-142`, `traverse.hpp:97-106`, `fold.hpp:184-188`, `monad.hpp:36-39`) fire only if someone instantiates `derive_X<std::false_type>`, which lookup never does (acknowledged in `operation-objects-and-the-lookup-modes.org`, "Diagnostics"). `derive_functor` has no such assert at all (`functor.hpp:45-46`). tree_algorithms restores a message with its own `static_assert(has_instance_v<…>)` (`recursion_schemes_lookup.hpp:44-47`).

**Recommendation.** Delete the unreachable asserts or make them reachable (a `has_instance_v`-guarded `static_assert` in the operation objects *after* overload resolution is impossible, but a non-template `operator()(F&&, T&&) = delete("…")` fallback or a documented `has_instance_v` idiom would help). At minimum do not advertise them.

### F9 — Compile-time and constexpr (note)

**Evidence.** clang 18 `-fsyntax-only`: std-header baseline 0.52-0.57 s; core headers (functor, apply, fold, traverse, sequence, monad, monoid, array) 0.61 s; with one traverse/transpose/fmap/length/bind/invoke each, 0.75 s **[measured, single runs]**. `error_set.hpp:448` crashes clang 18 (below the documented Clang 19 floor, `README.md:58`) — the grade headers are where the toolchain sensitivity is. `invoke`, `fmap`, `length`, `combine_all` work in constant expressions **[verified p10]**; `fold_left`/`fold_right` derived via `std::function` do not (`fold.hpp:40-69`, `decisions.md:3445-3470`). Deep-concept probing instantiates derived-op bodies because return types are deduced (`detail/typeclass_base.hpp:213-219`), so `applicative_object` checks are not free; the `laws.hpp` harness is `consteval` and deliberately kept out of constraints (`laws.hpp:8-20`).

### F10 — ADL, value category, modules (notes)

- **ADL.** Operation objects are not ADL-findable (by construction). But the Impl-member style means the *instance* for a user type lives in `beman::transpose`, not next to the type; there is no "instance found via the type's namespace" story, which is exactly what makes every instance an orphan (F2). `mbind` (`monad.hpp:437-440`) is a remaining free function template, ADL-findable, "retained for source compatibility" in an unproposed header — delete.
- **Value category.** The Applicative side is careful (`contained_ref_t`, `forward_contained`, `forward_at`, `detail/typeclass_base.hpp:136-184`; `consumes_rvalue_structure`, `traverse.hpp:67-69`, `sequence.hpp:103`). The Functor side is not: `VectorFunctorImpl::fmap` takes only `const std::vector&` (`functor.hpp:283-284`), so `fmap(f, std::move(v))` copies every element, and there is no Functor analogue of the consuming declaration. Foldable likewise (`sequence.hpp:53-55`). The decision slug `operand-value-category` (`decisions.md:1797`) covers only Applicative.
- **Modules.** transpose has no module unit; `BEMAN_TRANSPOSE_USE_MODULES()` is hard-wired `0` (`config.hpp:10`, `config_generated.hpp.in:7`). fingertree's and tree_algorithms' module units `#include` transpose headers inside `export { }` (`fingertree.cppm:6-11`), so transpose's names are re-exported from consumer modules; `machinery-deduplication.md` (stage 5) records the GMF workaround and that module-build tests do not compile on gcc 15/16. Open question not addressed anywhere: a consumer that specializes `functor<MyT>` inside *its own* named module is specializing a template attached to another module; whether that is well-formed and what it means for coherence should be checked before the paper says "a program may specialize it". `functor.hpp:21-24` shows the authors are aware of module-visibility issues for lambdas in constraints.

---

## 3. Decisions driven by transpose's domains that tax a container or recursion-scheme library

### 3.1 Full-type keying + no rebind → fingertree's traverse is not policy- or allocator-preserving (significant)

**Evidence.** `finger_tree_traversable.hpp:8-9` ("Reconstruction… re-applying the default UnitMeasure to the result type") and `:60` (`FingerTree<U>::from_sequence(std::move(values))`): the result drops MONOID, MEASURE_POLICY and ALLOCATOR. transpose's contexts rebind trivially (`std::optional<remove_cvref_t<Result>>`, `functor.hpp:273`), so the mechanism never grew a "same structure at element `U`" hook. **Why it matters.** For a measured, allocator-aware persistent container the rebind *is* the hard part. **Recommendation.** Either give `derive_traversable`/`derive_functor` a `rebind<U>` customization (an alias template on the Impl) that the derived operations use, or state in the paper that shape preservation is "structure modulo policy parameters" — and check Paper C's claims against that.

### 3.2 Allocator blindness (significant for fingertree, which owns the PMR story)

**Evidence.** `functor<std::vector<VALUE_TYPE>>` etc. match only `std::allocator` (`functor.hpp:316-319`, `sequence.hpp:80-82, 125-127`, `monoid.hpp:263-275`); `vector<char, pmr::polymorphic_allocator<char>>` has no instance **[verified p0]**. The vector traversal seeds with `applicative.pure(std::vector<Element>{})` (`sequence.hpp:224`), `to_vector` returns `std::vector<Element>` (`fold.hpp:596-600`), `collect` is `collect(std::vector<CONTEXT>)` (`traverse.hpp:272-275`). **Recommendation.** At least key the vector instances on `std::vector<T, A>`; decide whether the fold/traverse result containers are allocator-propagating (they are not today).

### 3.3 Grade machinery inside the proposed Applicative core (significant for paper scope; minor for consumers)

**Evidence.** `apply.hpp:8` includes `grade.hpp`; `derive_applicative::subsume<TARGET_GRADE>` (`apply.hpp:222-226, 796-804`) and `derive_monad::subsume` (`monad.hpp:224-231`); `applicative_object` has a `graded_context` conjunct (`apply.hpp:479-482`); `accumulating_applicative` is a second registry in the core header (`apply.hpp:293-294`). `crtp_absorption.test.cpp:118-123` presents "the instance did not ask for this member and cannot tell it is there" as a feature. **Why it matters.** Every container/tree Applicative carries error-grading surface and wording; the paper's own Non-goals ("Standardizing the full algebraic hierarchy") sit beside a grade algebra in the wording (`D3200R0:670-710`). **Recommendation.** Move `subsume` to the graded carriers' objects (expected.hpp) and drop the `graded_context` conjunct from `applicative_object`.

### 3.4 Monoid: parameterisation mostly fits, registry and detection do not (minor)

Monoid-selection (`decisions.md:2964-3172`) *was* shaped by fingertree (the decision cites fingertree's five tag types, `reversed()` and the O(n) `size()`), and fingertree now spells `FingerTree<T, sum_monoid<size_t>, …>`, `dual_monoid<MONOID>`, `tuple_monoid<…>` (`finger_tree.hpp:298, 1255-1259`, `finger_tree_priority_queue.hpp:68`). This is the one place where the consumer did exert design pressure. Residual costs: (a) F4; (b) canonical-monoid detection by type equality, `has_unit_count_measure = same_as<MONOID, sum_monoid<size_t>> && …` (`finger_tree.hpp:959-960`), is exactly the "the value no longer records its monoid" consequence the decision predicted (`decisions.md:3104-3118`); (c) `Monoid<T>` is a class-template registry (needs default construction, `monoid.hpp:77`) while every other typeclass is a variable; `monoid<T>` is a hard error for unregistered `T` (incomplete type) where `functor<T>` is a `false_type` sentinel (`decisions.md:1620-1627`).

### 3.5 Foldable: proposed by Paper D, implemented in transpose, and spelled two ways (significant)

**Evidence.** transpose: `fold_map(f, structure, monoid)` with `monoid_object` (`fold.hpp:209-219`; `sequence.hpp:50-57`). tree_algorithms: `fold_map<Result>(map_fn, combine, identity, tree)` and `layer_fold<Layer>` instances shaped `(map_fn, combine, identity, F<Result>) -> Result` (`fold_map_lookup.hpp:44-50, 76-78`; `fold_map.hpp:55, 79`); no `monoid_object`, no `identity()`/`combine()` in `include/beman/tree_algorithms/*.hpp` (grep). The coherence skill says "Monoid operations are spelled `identity()` and `combine()` — defined once, in transpose, and used by fingertree and anything new" (`SKILL.md`, Naming). `machinery-deduplication.md` (layer-fold-and-foldable) leaves the question OPEN. **Why it matters.** The same verb name in one codebase with two argument shapes, and the proposing paper (D) not using the proposing library's monoid vocabulary. **Recommendation.** Decide (slug) whether Paper D's `fold_map` takes a `monoid_object`; if yes, `layer_fold` instances take the instance too.

### 3.6 Deducing-this and `this auto &&` on Impls (minor)

Every shipped Impl writes `this auto &&` on members that never use `self` (`functor.hpp:269, 283`, `apply.hpp:492, 498`, consumers `finger_tree_foldable.hpp:26`, `binary_tree.hpp:182`). The requirement comes from the base's protected-inheritance/`impl_of` machinery and GCC 14 floor (`detail/typeclass_base.hpp:12-20`), not from anything the consumer needs. A plain `const` member works for an Impl (the probe in F4 used one); the docs do not say so.

### 3.7 Things transpose's domains got right for consumers (for balance)

`consumes_rvalue_structure` was designed with persistent, shared structures in mind (`traverse.hpp:44-51`); `collect` keeps `traverse` usable where composition is not type-stable (`traverse.hpp:258-275`); `invoke` as the interface imposes nothing on containers.

---

## 4. Repo surface vs paper surface

Legend: **P** = proposed (appears in `papers/wording/*.md` or D3200R0 names it as proposed); **E** = unproposed and explicitly labelled evidence/non-normative in the header; **S** = shipped-but-silent (unproposed, no label).

| Header | Names | Status | Evidence |
|---|---|---|---|
| `apply.hpp` | `applicative_eval`, `discard_first_eval`, `discard_second_eval`, `applicative_impl`, `derive_applicative` (`invoke`, `ap`, `map`, `lift`, `zip_with`, `discard_first`, `discard_second`, `subsume`, `invoke_with`×2), `applicative<T>`, `accumulating_applicative<T>`, `applicative_object`, objects `invoke`, `ap`, `pure<C>`, `zip_with`, `discard_first`, `discard_second`, `optional_applicative_impl/map` (expos) | **P** | `wording/transpose.applicative.*.md` |
| `traverse.hpp` | `applicative_context`(expos), `consuming_traversable_object`, `derive_traversable` (`for_each`, `transpose`, `traverse_with`×2, `transpose_with`), `traversable<T>`, `traversable_object_t`/`structure_element_t`/`traverse_element_t`/`traverse_context_t`/`transposable_structure`/`transposing_object` (expos), `collecting_applicative`(expos), `applicative_object_for`, `traversable_impl`, `traversable_object`, object `traverse` | **P** | `wording/transpose.traversable.*.md`, `transpose.alg.traverse.md` |
| `transpose.hpp` | object `transpose`; umbrella | **P** | `transpose.alg.transpose.md` |
| `sequence.hpp` | `vector_traversable_impl/map` (expos), `traversable<vector<T>>` | **P** | `transpose.range.*.md` |
| `sequence.hpp` | `VectorFoldableImpl/Map`, `foldable<vector<T>>` | **E** | `sequence.hpp:33-37` (`\omit`, "carries no wording") |
| `array.hpp` | `array_applicative_impl/map`, `applicative<array<T,N>>`, `transpose_tuple` | **P** | `transpose.array.*.md` |
| `grade.hpp`, `error_set.hpp`, `expected.hpp` | grade algebra, `error_set`, `recover`, expected applicative + accumulating | **P** | `transpose.grade.*`, `errset.*`, `expected.*` |
| `expected.hpp` | `ExpectedMonadImpl/Map`, `monad<expected<…>>` | **E-ish** | `\omit` at `expected.hpp:432-446`; no banner, but covered by monad.hpp's banner |
| `overloaded.hpp` | `overloaded` | **P (decided, no paper text yet)** | `decisions.md:3415-3443`; `machinery-deduplication.md` "Left open: P3200 text for `overloaded`" |
| `functor.hpp` | `derive_functor` (`fmap`, `replace`), `functor<T>`, objects `fmap`, `replace`, `functor_impl`, `functor_object`, `OptionalFunctorImpl/Map`, `VectorFunctorImpl/Map`, `detail::constant_function` | **S** | no banner; not in wording; D3200R0 Non-goals (575-591) do not mention Functor; D3200R0:420-421 lists `functor<T>` among "the lookup variables" as if proposed |
| `monoid.hpp` | `Monoid<T>`, `monoid<T>`, `monoid_value_t`, `monoid_object`, `sum/product/max/min_monoid`, `any/all_monoid`, `first_monoid`, `dual_monoid`, `tuple_monoid`, `Monoid<string>`, `Monoid<vector<T>>`, `monoid_identity`, `monoid_combine` | **S** | header has invariants (`monoid.hpp:21-36`) but no "not proposed" banner; `decisions.md:3380-3384` says "the paper proposes neither Monoid nor Foldable"; D3200R0 Non-goals do not list Monoid |
| `fold.hpp` | `derive_foldable` (11 members), `foldable<T>`, 10 operation objects (`fold_map`, `fold_left`, `fold_right`, `length`, `combine_all`, `any_of`, `all_of`, `empty`, `to_vector`, `find_first`), `foldable_impl`, `foldable_object`, `detail::left/right_compose_monoid`, probe witnesses | **E** | `fold.hpp:17-23` banner; D3200R0:584-588 |
| `monad.hpp` | `derive_monad` (`fmap`, `invoke`, `join`, `kleisli`, `ap`, `subsume`, `bind_with`, `as_functor`), `monad<T>`, `monad_impl`, `monad_object`, `OptionalMonadImpl/Map`, objects `bind`, `join`, `kleisli<C>`, free `mbind` | **E** (`mbind` is cruft) | `monad.hpp:19-26`; `monad.hpp:433-440` |
| `induced_monoid.hpp` | `kleisli_monoid`, `lifted_monoid` | **E** | `induced_monoid.hpp:13-26` |
| `laws.hpp` | `beman::transpose::test::laws::*` (grade laws only — not Functor/Applicative laws, which live in `test_support.hpp`) | **E (test deliverable)** | `laws.hpp:6-9`; not in the CMake header set (`CMakeLists.txt:15-42`) though it sits beside shipped headers |
| `sender.hpp`, `zip_list.hpp`, `simd_lanes.hpp` | demo contexts + instances | **E** | "Non-normative demonstration type" banners (`sender.hpp:6`, `zip_list.hpp:6`) |
| `simd.hpp` | `SimdVecApplicativeImpl/Map`, `applicative<simd::basic_vec<T,ABI>>` | **S** | `simd.hpp:6-17` motivates but never labels it non-normative/unproposed; not in wording |
| `detail/typeclass_base.hpp` | **public-namespace** `always_false_v`, `impl_ref_t`, `has_instance_v`, `applicative_value`, `applicative_value_t`; `detail::probe_witness*`, `forward_contained`, `forward_at`, `make_array` | **S / detail leakage** | `detail/typeclass_base.hpp:41-100`; `has_instance_v` consumed by `tree_algorithms/recursion_schemes_lookup.hpp:44`, `fold_map_lookup.hpp:60` |
| `detail/applicative_derivation.hpp` | `terminating_partial`, `applicative_eval_t`, `detail::ap` free function | detail | `applicative_derivation.hpp`; `detail::ap` used only by `apply.test.cpp:250` |
| `config.hpp` | `BEMAN_TRANSPOSE_USE_MODULES()` always 0 | note | `config.hpp:10` |

Shipped-but-silent to fix: **functor.hpp**, **monoid.hpp**, **simd.hpp**, and the public names in `detail/typeclass_base.hpp`. The paper's Non-goals should name Functor and Monoid alongside Foldable and Monad, and the two headers need the same banner `fold.hpp` and `monad.hpp` carry.

---

## 5. Internal consistency

### F11 — `fmap` vs `map`, `pure` vs `lift`, `fold` vs `combine_all` (significant)

See F3 for `fmap`/`map`. `lift` is `pure` under another name (`apply.hpp:641-661`), `fold` is `combine_all` under another name (`fold.hpp:457-490`) — both are members on the objects while the explainer says aliases get no second name. `identity`/`combine` are consistent across transpose and fingertree (`monoid.hpp`, `fold.hpp:44-68`, `induced_monoid.hpp`, `finger_tree.hpp:114-174`); tree_algorithms does not use them (F3.5). `ap`/`invoke`/`pure` are consistent and match the coherence skill.

### F12 — The case rule does not hold (minor, but it is a stated rule)

`decisions.md:3394-3413` (proposal-name-case): "Anything that reaches the proposal's text is snake_case… Implementation names that do not reach the proposal are CapitalCase. Case therefore says whether a name is proposed." Counter-examples in code: `derive_foldable`, `fold_map`, `foldable`, `length`, `to_vector` (`fold.hpp`, unproposed), the whole `*_monoid` family and `monoid_object` (`monoid.hpp`), `kleisli_monoid`, `lifted_monoid`, `zip_list`, `sender`, `simd_lanes`, `derive_monad`, `bind`, `join`, `kleisli`. CapitalCase is in fact used only for Impl/Map classes of unproposed instances (`OptionalFunctorMap`, `ZipListApplicativeImpl`, `ExpectedMonadImpl`) and for `Monoid`. Either restate the rule as "Impl/Map classes of unproposed instances are CapitalCase" or drop the claim that case signals proposal status.

### F13 — Namespace layout and detail leakage (minor)

Single namespace `beman::transpose` by decision (`decisions.md:3265-3299`); `detail` reopened in eight headers; `beman::transpose::test::laws` in `laws.hpp`. Duplicated entities for the wording generator: `detail::applicative_eval` and namespace-scope `applicative_eval` are two objects (`apply.hpp:19-32`, citing steve-downey/specgen#33), likewise `discard_*_eval`. `has_instance_v` and `applicative_value_t` are public API living in `detail/` (F5, table). `probe_witness` is `detail::` but appears in proposed concept definitions (rendered `$probe-witness$`, fine) and in `fold.hpp` as named witnesses that exist to work around an MSVC ICE (`fold.hpp:80-88`).

### F14 — Shape uniformity claims vs. code (minor)

`functor.hpp:132-134` ("Shape, uniform across every typeclass: … default a trailing `const auto &TC` NTTP") — `traverse_fn`/`transpose_fn` do not (`traverse.hpp:531-542`, `transpose.hpp:43-50`); `derive_functor` lacks the `false_type` `static_assert` the other four bases have; `derive_traversable` uses `using Impl::traverse;` re-export where `derive_functor::fmap` is an own member (the own-member rule, `decisions.md:1218-1230`, is applied to Functor and not to Traversable/Monad's `bind`/`pure`).

### F15 — Stale references and names (minor, many)

- Header comments point at `docs/decisions.md#…` (should be `docs/transpose/decisions.md`): `functor.hpp:109-110`, `monoid.hpp:31`, `fold.hpp:32, 146, 679`, `traverse.hpp:269, 548`, `grade.hpp:13-28`, `detail/typeclass_base.hpp:14, 56, 73`, `error_set.hpp` (14 sites), `expected.hpp` (5), `sender.hpp:29`, `induced_monoid.hpp:19`, `laws.hpp:25, 140, 247, 261`. `laws.hpp:18` says `include/beman/transpose/grade.hpp`; `laws.hpp:20` `docs/transpose-grading-plan.md`.
- `docs/transpose/README.md`: "Headers live under `include/beman/transpose/`", lists `dual_monoid.hpp` (deleted 2026-10-08, `decisions.md:3151`), says "three lookup modes" and "`*_typeclass<T>` variable template" (renamed 2026-08-02/10-10).
- `docs/transpose/typeclass-object-pattern.md:32-34` (`include/`, `tests/` paths), `:209-226` and `:243-247` ("Minimal required operations for Applicative impl are `pure` and `apply`" — the basis is `pure` + `invoke`|`ap`, and `apply` was renamed `ap`).
- `docs/transpose/baseline-vocabulary-audit.md:68-76` inventories `Functor`, `functor_typeclass`, `monoid_v` (pre-rename). It is a dated audit, i.e. transactional — candidate for removal under the owner's end-of-month rule.
- `operation-objects-and-the-lookup-modes.org` ("One namespace"): "`map` and `lift` are spelled `fmap` and `pure`" — contradicted by `apply.hpp:164-179`.
- `typeclass-object-pattern.md:142-170` example calls the public layer a "CPO" while the paper and headers insist these are *not* CPOs (`functor.hpp:124-130`, D3200R0:341-352).

---

## What I verified vs. inferred

Verified by compile/run probes (clang 18.1.3, C++23): p0 pmr vector unregistered and headers compile; p1 concrete-beats-constrained-generic; p2 generic-vs-generic ambiguity; p3/p4/p12 diagnostics text; p5 specialize-after-use error; p6 `applicative` has `map` not `fmap`, `functor<array>` absent, monad object fails both deep concepts, `as_functor()` passes `functor_object`, `functor_object` is SFINAE-false for a `value_type`-less layer while `has_instance_v` is true, shipped objects are empty; p7 registered stateful `Monoid<Tag>` is ignored by `combine_all` (exit code 6, not 8); p10 constexpr `invoke`/`fmap`/`length`/`combine_all`; compile timings (single runs). Everything else is from reading the cited lines. Cross-TU ODR behaviour and the module-attachment question are inferred from the standard, not demonstrated.
