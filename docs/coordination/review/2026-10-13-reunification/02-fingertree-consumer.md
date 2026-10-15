# 02 — fingertree as a consumer of the typeclass mechanism

Scope: the "design resistance" fingertree never got to offer. Read: `src/beman/fingertree/**`, `docs/fingertree/*`, `examples/fingertree/`, `docs/coordination/{machinery-deduplication,SYNC,STATUS}.md`, `src/beman/transpose/{monoid,induced_monoid,fold,functor,traverse,apply}.hpp`, the `monoid-selection`, `monoid-carrier-canonicity`, `registered-instance-state`, `monoid-registry-spelling`, `derived-op-constant-evaluation` and `traverse-policy-surface` entries of `docs/transpose/decisions.md`, the coherence skill, and the fingertree commits `af05316`, `046bcc9`, `bc9d892`. All paths below are relative to `/home/user/typeclass`. Nothing was built or run; every claim is from reading the files, and I say where I infer rather than verify.

## Executive summary

1. **The monoid half of the mechanism fits fingertree well, and the record shows fingertree *did* push back there** — the `monoid-selection` ruling (decisions.md:2964-3172) was argued largely from fingertree's costs, and `af05316` is fingertree accepting it. The guess that "fingertree offered no resistance" is half wrong: it resisted the *monoid* design and won. Instance-keyed trees (`Elem<T, MONOID>`), `tuple_monoid` for the priority queue, `dual_monoid` for `reversed()` all land cleanly and are tested.
2. **The Foldable/Traversable half fits poorly, and nobody has yet said so in a decision log.** `traverse` over any `FingerTree<T, M, P, A>` returns `FingerTree<U>` — the default count tree with `std::allocator` (finger_tree_traversable.hpp:58-62). The measure, the monoid instance (including its state), and the allocator are silently discarded. The same happens for all four wrappers. This is the single most important design finding: Traversable wants a type constructor `F<_>`, and a measured sequence is not one unless the measure is fixed per element type.
3. **There is no Functor instance for FingerTree at all** (grep for `functor<`/`fmap` in `src/beman/fingertree/` is empty), which is the honest consequence of point 2 — but it is unrecorded.
4. **The measure-as-homomorphism law is unstated and untested.** No test pins `fold_map(measure_policy, t, t.get_monoid()) == t.measure()`, the one equivalence law the skill's test discipline demands for every new instance. `MONOID` is not constrained by transpose's own `monoid_object` concept (no occurrence in fingertree).
5. **Stateful instances are permitted by the ruling but fingertree has a hole**: `append` on allocator mismatch rebuilds the right operand under `MONOID{}`, not `d_monoid` (finger_tree.hpp:1185), so a `modular_sum{5}` tree appended to a `modular_sum{5}` tree on a different PMR resource recomputes node measures under `modular_sum{0}` — `% 0`. Latent (needs PMR + stateful instance), but it is exactly the "keying on type, not state" cost the ruling predicted (decisions.md:3050-3056) showing up.
6. **Test discipline**: no non-commutative *measure* monoid is tested anywhere, so `dual_monoid`'s flipped `combine` is never observable; the test named `ReversedNonCommutativeElementOrder` (finger_tree.test.cpp:628) uses the commutative count monoid. No bare-CTAD violations. `constexpr` is moot for a `shared_ptr`-based tree; the Impls are not marked `constexpr` while every transpose derived op now is.
7. **Paper C has no draft and the README is still the Copier template** (docs/fingertree/README.md:15-17, 28-30). Five design questions must close before drafting; they are listed in §5. The essay cites the paper as "P4325" while the skill says D4325R0 (essay md:115 vs SKILL.md "Paper numbers in flux").

## 1. How fingertree uses transpose today

**Verified.**

| Facility | Where | Spelling |
|---|---|---|
| Monoid instances | `tree/finger_tree.hpp:17,53-54,297-300` | `beman::transpose::monoid_value_t<MONOID>` as `ft::tag_t`; default `MONOID = beman::transpose::sum_monoid<std::size_t>`; `d_monoid` stored `[[no_unique_address]]` (line 341); `monoid.identity()`/`combine()` threaded through every helper |
| `dual_monoid` | `finger_tree.hpp:1255-1260` | `reversed()` returns `FingerTree<T, dual_monoid<MONOID>, Meas, ALLOCATOR>` |
| `max_monoid` | `finger_tree_interval_index.hpp:38` | `using IntervalMonoid = beman::transpose::max_monoid<std::size_t>` |
| `tuple_monoid` + `sum_monoid` | `finger_tree_priority_queue.hpp:251-253` | `PriorityMonoid<T> = tuple_monoid<sum_monoid<size_t>, optional_min_monoid<T>, optional_max_monoid<T>>` |
| Foldable | `finger_tree_foldable.hpp:18-52` and one Impl per wrapper (`*_random_access.hpp:555-585`, `*_rope.hpp:784-812`, `*_priority_queue.hpp:337-367`, `*_interval_index.hpp:100-129`) | `struct …FoldableImpl { using element_type = T; fold_map(this auto&&, F&&, const Tree&, RESULT_MONOID) }`, Map `: derive_foldable<Impl>`, registration `template<…> inline constexpr auto beman::transpose::foldable<Tree> = Map{}` |
| Traversable | `finger_tree_traversable.hpp:31-80` and the same four wrappers | `traverse(this auto&&, const APPLICATIVE&, F&&, const Tree&)` using `applicative.pure`/`applicative.invoke`; Map `: derive_traversable<Impl>` with a redundant `using Impl::traverse;` (the base already has one at traverse.hpp:109) |
| `overloaded` | `finger_tree.hpp:18,41` | `beman::transpose::overloaded` for every `std::visit` |
| Functor | — | **none** |
| Applicative | consumed only (`pure`, `invoke`, `applicative_value_t`) | never registered |

Names registered: five `foldable<>` and five `traversable<>` specializations (FingerTree, RandomAccess, Rope, PriorityQueue, IntervalIndex). Two monoid instances defined in fingertree rather than transpose: `optional_min_monoid<T>`, `optional_max_monoid<T>` (priority_queue.hpp:212-241), with adjoined-`nullopt` identity because transpose's `min_monoid`/`max_monoid` use saturating `numeric_limits` bounds (monoid.hpp:117-153) and so need a `numeric_limits` specialization.

**What had to be adapted at merge** (verified from `git show 046bcc9`, `bc9d892`, and the ledger `docs/coordination/machinery-deduplication.md:9-19,54-71`):

- `af05316` (2026-10-08, before the merge) is the big one: `TAG_TYPE` became `MONOID`; `tag_id<Tag>()`/`tag_op<Tag>()` lookups through `monoid_v<Tag>` were replaced by calls on a stored instance; `ReversedMeasure`/`DualMonoid` wrapper and `dual_monoid.hpp` retired; hand-written tags (`IntervalMaxEndTag`, `MinTag`, `MaxTag`, `PriorityTag`-as-struct) replaced by plain `size_t` / `std::tuple` under named instances; `PriorityQueue::size()` went from O(n) to O(1) by adding a count component to the tuple. The typeclass layer was re-vendored from transpose with the namespace renamed.
- `046bcc9` (2026-10-12): deleted the six vendored headers (1763 lines); renamed `Foldable<>`→`derive_foldable<>`, `foldable_typeclass`→`beman::transpose::foldable`, `typeclass::sum_monoid`→`transpose::sum_monoid` (24 sites), local `remove_cvref_t`→`std::remove_cvref_t`, `beman::fingertree::traverse`→`beman::transpose::traverse` in tests; CMake links `beman::transpose` and `find_dependency`. Decided with **no aliases** ([fingertree-public-names], ledger :158-166): `FingerTree<T>` changed type identity.
- `bc9d892`: `fixpoint/overloaded.hpp` deleted in favour of `beman::transpose::overloaded`.
- The 5 Traversable bodies were kept unchanged because `invoke` is a basis in transpose (ledger :65). The ledger's stage-2 note "Check transpose's `traversable_object` probe against the wrappers that have no `value_type`" (:67) — I find no test asserting `traversable_object<…>` on any fingertree type, so that check was resolved by not asserting it. Nothing records that choice.

The merge was mechanical by design ("touches no paper text (Paper C is not yet drafted)", ledger :56). No design question was raised from the fingertree side during it; that is the gap this review fills.

## 2. Where the mechanism fits poorly (and where it fits)

### 2.1 Monoid on the measure carrier, not on elements — fits (significant confirmation)

**Verified.** The `monoid-selection` ruling (decisions.md:2964-3172) argues from fingertree in six bullets (:3023-3041): tag-as-selector, the removed `Monoid<std::size_t>`, hand-written tags, the O(n) `size()`, `DualMonoid` forwarding `>=`, and `MaxTag` vs `Max` identities. The "Consequences" paragraph (:3044-3056) already predicts the instance-keyed node type. `af05316` implements it. Tests pin it: `TwoMonoidsOverOneValueTypeAreDifferentTrees` (finger_tree.test.cpp:725-742) shows count and max over `size_t` are distinct trees with the same `tag_type`; `StatefulInstanceTravelsWithTheTree` (:744-760) shows a modulus survives `snoc`/`cons`/`reversed`/`split`/`concat`.

So the hypothesis "fingertree offered no resistance" is false for Monoid: this is the one place where the consumer's costs drove the mechanism's design, and the result is good. Three monoids over one carrier (count, max, product-of-three) coexist with no wrapper types. **The guess was right here, and it was made by looking at fingertree.**

What is still missing on this axis:

- **(significant) `MONOID` is unconstrained.** `monoid_object<M, T>` exists (monoid.hpp:62-67) and fingertree never uses it (grep is empty). A `MONOID` with a misspelled `combine` fails deep inside `make_node2`. Recommendation: `requires beman::transpose::monoid_object<MONOID, beman::transpose::monoid_value_t<MONOID>>` on the class template, plus a `measure_policy<P, T, V>` concept (`std::invocable<const P&, const T&>` returning `V`). Cheap, and it is the point of having the concept.
- **(significant) The homomorphism law is unstated.** A measured sequence is correct iff `measure(xs ++ ys) == combine(measure(xs), measure(ys))` and `measure(leaf x) == policy(x)`. The Foldable instance makes this a one-line test: `foldable<FT>.fold_map(Meas{}, t, t.get_monoid()) == t.measure()`. Nothing like it exists in any `*.test.cpp`. This is the equivalence law the skill requires ("Every new verb/instance gets an equivalence law pinning it to an already-tested primitive", SKILL.md Test discipline) and the Foldable instance is a new instance.

### 2.2 Measure caching and type-level identity of the monoid — fits, with one sharp edge

**Verified.** Caching is why the tree is keyed on the instance *type*: `Elem<T, MONOID>` (finger_tree.hpp:45-54, 65-66). Two trees with different measures over the same element type never share `Elem` nodes. `has_unit_count_measure` (:959-960) uses type identity of the instance to pick O(1) `size()`, `operator==` short-circuit and `flatten().reserve`.

**(significant) Stateful instances defeat the type-level key, and the code has one concrete hole.** The ruling accepts this ("Two trees whose instances are the same type with different state … would concatenate without complaint", decisions.md:3050-3056). fingertree's `append` fast path (`finger_tree.hpp:1183-1189`) never compares `d_monoid` values, as predicted. But the *slow* path is worse than predicted:

```cpp
// finger_tree.hpp:1184-1186
if (!alloc_equal(d_alloc, right.d_alloc)) {
    auto rebuilt = from_sequence(right.flatten(), d_alloc);   // monoid defaults to MONOID{}
    return app3(d_alloc, d_monoid, *this, {}, rebuilt);
```

`from_sequence`'s third parameter defaults to `MONOID{}` (:1164-1166). For `modular_sum` (test file :45-54) that is `d_modulus == 0`, and every `make_node2/3` in `from_elems_impl` computes `(a + b) % 0`. Not reachable with `std::allocator` (`is_always_equal` short-circuits at :357), reachable with `pmr::FingerTree<int, modular_sum>` across two resources. The other rebuild sites (:916, :931, :947) correctly pass `d_monoid`. This is a one-token fix (`d_monoid` as third argument) and a missing test (stateful instance + PMR mismatch). Also `leaf()` (:962-966) uses `MONOID{}`; harmless today because a leaf's measure comes from the policy, not from `combine`, but it means `FingerTreeRandomAccess::insert` (random_access.hpp:519) builds a `Tree::leaf` carrying a default-constructed instance that then gets `concat`'d — fine only because `app3` takes the left operand's instance.

**(minor) Commutativity is not expressible, so `reversed()` pays for nothing.** `reversed()` returns a `dual_monoid<sum_monoid<size_t>>` tree even for the count measure, so `Rev::has_unit_count_measure` is `false` (test pins this at :722) and `size()`, `begin()`, `end()` on a reversed count tree are O(N) (`finger_tree_iterator.hpp:414-432`). The dual of a commutative monoid is the same monoid; the mechanism has no way to say so. Recommendation to transpose: an optional declared trait on instances, exactly the pattern `consumes_rvalue_structure` already uses for traversable objects (traverse.hpp:67-69, 116-117): `static constexpr bool is_commutative = true;` on `sum_monoid`, `product_monoid`, `max_monoid`, `min_monoid`, `any_monoid`, `all_monoid`, re-exported by `dual_monoid` and `tuple_monoid` (all-of). Then `reversed()` can return `FingerTree<T, MONOID, …>` when `MONOID` is commutative and the dual otherwise. Low cost, and it is the first *property* a consumer has asked a monoid instance to declare — worth a decision slug.

### 2.3 Allocator / PMR

**Verified.** The allocator story is fingertree's own and is well worked out (`docs/fingertree/allocator-design.md`, `allocate_spine` at finger_tree.hpp:383-403, the null-upstream probe `finger_tree_pmr_probe.test.cpp`). The monoid instance travels beside the allocator (:340-348) in the same way, which the design doc notes (allocator-design.md:534-538).

**(significant) The typeclass path breaks the Lakos coherency story.** `traverse` reconstructs with `FingerTree<U>::from_sequence(std::move(values))` (traversable.hpp:60), i.e. `std::allocator<std::byte>` and `MONOID{}`. A `pmr::FingerTree<int>` traversed through `beman::transpose::traverse` comes back as a non-PMR tree with no compile-time or runtime notice. The allocator design spends two pages making mixed graphs unrepresentable through the public API; the Foldable/Traversable surface is public API and ignores it. The derived `to_vector` (fold.hpp:579-601) allocates with `std::allocator` too, which is fine for a *result* vector but worth stating.

Where the mechanism *could* help and does not: transpose's instances carry no allocator notion at all (`applicative.pure(std::vector<U>{})`, traversable.hpp:42). This is correct for P3200's scope, but Paper C has to say what `traverse` means for an allocator-aware persistent container, and today's answer ("you get the default-allocated default-measured tree") is unlikely to survive LEWG.

### 2.4 constexpr

**Verified.** FingerTree is `shared_ptr`-based and cannot be constant-evaluated; no fingertree operation is `constexpr` apart from the user monoids in tests (finger_tree.test.cpp:38-42, compile_probe:50-51). `derived-op-constant-evaluation` (decisions.md:3445-3471) made every `derive_*` operation `constexpr`. The cost to fingertree is zero; the benefit is zero. The Impls' `fold_map`/`traverse` are not declared `constexpr` (foldable.hpp:26-27, traversable.hpp:36-37); harmless, but the skill's DEV-04 reads "constexpr at declaration on every CRTP-derived operation", and these are the Impl side, so strictly N/A. **Not a fit problem**, and the right answer is to say so in Paper C's implementation-experience section rather than chase it.

### 2.5 `std::inplace_vector` / C++26 floor

**Verified.** fingertree requires C++26 and `<inplace_vector>` (fingertree/CMakeLists.txt:26,35; `Digit = std::inplace_vector<ElemPtr, 4>` at finger_tree.hpp:168); the build probes and skips (src/beman/CMakeLists.txt:4-33); README.md:59-60 says GCC 16 only. transpose is C++23 (transpose/CMakeLists.txt:14), tree_algorithms C++23. fingertree is also the one library that cannot be a module ("The finger-tree headers include many standard headers, which conflict with `import std`", fingertree/CMakeLists.txt:3-7).

**(minor, coherence)** The skill says "Do not import C++26-isms into proposal repos" and "Language floors differ deliberately: tree_algorithms C++23, fixpoint C++26" — fingertree's C++26 floor is not listed there and should be, since it is now in the same repository. Not a mechanism-fit problem; transpose compiles under fingertree's floor trivially. The one real cost is CI: `inplace_vector` availability decides whether the Foldable/Traversable instances for FingerTree are *ever compiled* on a given toolchain, so transpose changes can break fingertree's instances silently on every toolchain but gcc-16 (ledger :148 says gcc-15 skips fingertree).

### 2.6 Persistent structural sharing vs transpose's value semantics — fits (confirmation)

**Verified.** This is the second place fingertree *did* shape transpose: `consumes_rvalue_structure` (traverse.hpp:33-69, 116-117; D3200R0.md:215-240) exists precisely because "a persistent sequence or tree shares its interior with every earlier version of itself, so an rvalue conveys the handle and not exclusive ownership". The default is non-consuming; fingertree's Impls declare nothing and so get the right behaviour for free. `operand-value-category` (decisions.md:1797) is the ruling. The guess was right and was informed by the persistent case.

What remains unexploited: every fingertree Impl takes the tree by `const&` only, so `traverse(f, std::move(tree))` works but gains nothing — correct for a shared structure, and should be stated once in Paper C.

### 2.7 Traversable over a finger tree — fits poorly (blocking for Paper C)

**Verified** from `finger_tree_traversable.hpp:14-26,31-64`:

- **Does traverse rebuild with re-measuring?** Yes. It `flatten()`s to `std::vector<T>`, folds `applicative.invoke` over a growing `std::vector<U>` in context, then `FingerTree<U>::from_sequence`. Every result node is re-measured — under `UnitMeasure<U, size_t>` and `sum_monoid<size_t>`, whatever the source tree's `MONOID`/`MEASURE_POLICY` were. The header's own comment claims "Preservation: monoid measure semantics are preserved through the traversal" (:17); that is false for any non-default tree. `IntervalIndex::traverse` does rebuild its `IntervalMonoid` tree (interval_index.hpp:161-165), because the measure (`d_end`) does not depend on the payload; `PriorityQueue::traverse` rebuilds `FingerTreePriorityQueue<U>` which re-measures under `PriorityMonoid<U>` — meaningful only if `U` is totally ordered, which is not checked.
- **Is order preserved?** Yes, sequence order. **Is shape preserved?** As a *sequence*, yes; as a *tree*, no — `from_sequence`'s bottom-up build (:787-839) produces a different spine shape than the source. For a persistent structure the shape is an implementation detail, so this is acceptable, but the paper must say "shape" means sequence shape.
- **Cost.** O(n) materialization plus O(n) rebuild, and the accumulation step `invoke`s a lambda taking `std::vector<U> values` *by value* (traversable.hpp:48-52), so the per-element cost depends on whether the applicative's `invoke` moves or copies its operands. `optional_applicative_impl::invoke` (apply.hpp:494-502) is declared with `FIRST &&` and `detail::contained_ref_t<FIRST>`, which I read as forwarding the contained value's category — so under `std::optional` the vector is moved and the whole traversal is O(n); **inference**, I did not read the out-of-line body. Under an applicative that must copy operands (a `std::vector` cartesian instance, or anything that evaluates its function more than once) the same Impl is O(n²). The header's "traversal is not a primary performance path, so O(n) materialization is acceptable" (:13-14) is a fingertree statement about the `std::optional`-shaped use only; the paper can state a complexity only after choosing which applicatives the instance supports.
- **The derived operations.** `derive_traversable` adds `for_each`, `transpose`, `traverse_with`, `transpose_with` (traverse.hpp:119-144). `transpose` on `FingerTree<std::optional<int>>` yields `std::optional<FingerTree<int>>` — fine for the count tree, since the count measure of `optional<int>` and `int` agree. For a `FingerTree<std::optional<int>, max_monoid<size_t>, SomeMeasure>` the result's measure is count, silently.

**Why this is a mechanism-fit problem and not just a fingertree bug.** Traversable (and Functor) are indexed by a type constructor `F : Type → Type`. `FingerTree<_, MONOID, MEASURE_POLICY, ALLOCATOR>` is a type constructor only if `MEASURE_POLICY` is fixed, and `MEASURE_POLICY` is a function *of the element type* (`UnitMeasure<T, tag>` names `T`; `IntervalMeasure<PAYLOAD>` names the payload; `RopeChunkMeasure` is `std::string`-only). Haskell has the same problem: `Data.FingerTree` offers `traverse'` with a `Measured v2 b` constraint on the result, and `Data.Sequence` is Traversable only because it fixes `v = Size` and wraps elements in `Elem` so the measure is constant. fingertree's registration "for all monoid and measure combinations" (traversable.hpp:77-80) is the *union* of those two designs, and it resolves the mismatch by throwing the measure away. The mechanism has no way to say "this type is Traversable only at this slice of its parameters" except by registering only that slice — which is available today and unused.

**Recommendation (blocking for Paper C; a transpose decision slug is warranted too):**

1. Register `traversable<FingerTree<T, sum_monoid<size_t>, UnitMeasure<T, size_t>, ALLOCATOR>>` only (the `Data.Sequence` reading), reconstructing with the *same* allocator and `MONOID`, and leave the general tree with an explicit, measure-changing free function such as `traverse_measured<U_MONOID, U_POLICY>(f, tree)` — the `traverse'` reading. Same for the wrappers: `RandomAccess` and `Rope` are already count/byte sequences and can stay Traversable; `PriorityQueue` and `IntervalIndex` should probably not be (an interval index traversed into a different payload type is fine, a priority queue traversed into an unordered type is not).
2. Alternatively, register generally but constrain `traverse` to `requires std::same_as<U, T>` (a traversal that only changes context, not element type) and document that `fmap`-shaped traversals are not available. This is the smaller change and the weaker story.
3. Either way, strike the "Preservation" bullet at traversable.hpp:17 until it is true.
4. Record in `docs/transpose/decisions.md` a slug for the question "What does Traversable mean for a structure whose shape depends on the element type?" — fingertree is the first consumer to hit it, and tree_algorithms' `FringeTree` (a count-measured sequence, fringe_tree.hpp:39-50) is the second.

### 2.8 Functor `fmap` changing the element type — unhandled

**Verified.** There is no `functor<FingerTree<…>>` registration. Also verified: `applicative_value_t<T>` is `T::value_type` when that nested alias exists (detail/typeclass_base.hpp:81-100), so `applicative_value_t<FingerTree<int>>` is `int` (finger_tree.hpp:880) and the conformance probes `foldable_object`/`traversable_impl`/`traversable_object` (fold.hpp:877-894, traverse.hpp:352-427) are well-formed for the core tree — a *structure* trait being read off an *applicative* trait, which works by coincidence of naming. The four wrappers (`Rope`, `RandomAccess`, `PriorityQueue`, `IntervalIndex`) declare no `value_type`, so `applicative_value_t` of any of them is ill-formed and none of transpose's conformance concepts can even be asked of their eight registered instances. That is the ledger's open check at machinery-deduplication.md:67 ("add `value_type` or don't assert the concept on them"), resolved by silence. Recommendation: add `using value_type = T;` to the wrappers and assert conformance (see §5). Separately, transpose might want a `structure_value_t` that is not spelled `applicative_`, since Foldable/Traversable are indexed by structures that are not contexts.

The honest position for Paper C: a measured sequence is a Functor *only* in the `Data.Sequence` slice (count measure), where `fmap` is `traverse` under the identity applicative — and `derive_traversable` does not derive `fmap` (no `fmapDefault`). If fingertree wants `fmap`, it writes an Impl for the count slice. That is a two-line consequence of the recommendation in §2.7 and should be decided together.

### 2.9 Foldable — fits, modulo the law in §2.1

**Verified.** `FingerTreeFoldableImpl::fold_map` walks `for_each` (foldable.hpp:26-34) — O(n), no allocation, correct left-to-right order, correct use of the result monoid passed by value. The derived `length`, `to_vector`, `find_first`, `any_of`, `combine_all` all work and are tested (foldable.test.cpp:15-80). The `IntFoldMapNamesItsMonoid` test (:62-72) pins that the two-argument `fold_map` is *not* a candidate for `int`, which is `monoid-selection` point 2 observed from the consumer side — a genuinely good test.

Two notes:

- The four wrapper Impls fold over `to_vector()`/`chunks()`/`entries()` (random_access.hpp:568-571 etc.), i.e. they materialize O(n) before folding. The core tree does not. Delegating to `foldable<Tree>.fold_map(...)` would remove four copies of the same loop and the allocation; it is also the "Impl built from an existing instance" pattern P3200 might want to show.
- `fold_left`/`fold_right` are derived through `std::function` composition (fold.hpp:343-353) — O(n) heap allocations for a structure that prides itself on none. fingertree could supply a native `fold_left` via `for_each` in two lines. `derived-op-constant-evaluation` already flags the `std::function` route as open (:3466-3471); fingertree is a consumer with a reason to want it closed.

### 2.10 The split `MONOID` + `MEASURE_POLICY` vs one `measured` object

**Verified** that fingertree has two template parameters where Haskell has one typeclass (`class Monoid v => Measured v a | a -> v where measure :: a -> v`). Consequences in the code:

- `MEASURE_POLICY` must be stateless: `meas_fn()` default-constructs one per call (finger_tree.hpp:320) while `MONOID` may carry state. A weight table or a locale-dependent width measure cannot be a policy today.
- The default `MEASURE_POLICY = UnitMeasure<T, ft::tag_t<MONOID>>` couples the two parameters (:299); a user naming `MONOID` must also name a policy, and the measure-based-splitting doc has to explain "three things" (measure-based-splitting.md:296-300).
- The two wrappers that fix both parameters (`Rope`, `RandomAccess`) express "must use sum_monoid over RopeChunkMeasure" only in a doc comment (rope.hpp:676-678, random_access.hpp:459).

**This is a pushback fingertree should make to itself, not to transpose**: the mechanism's instance-object pattern (one object with `identity`/`combine`) extends naturally to one object with `identity`/`combine`/`measure(const T&)` — a `measured_object<M, T>` concept in fingertree, keyed on the instance, stored once, stateful-capable, with `monoid_value_t<M>` still the tag. Whether that object lives in fingertree (it is Paper C vocabulary) or is proposed as a transpose combinator (`measured<MONOID, POLICY>`) is a question for the coordination log. I lean to fingertree: P3200 proposes only what its three-domain proof forces (SKILL.md), and nothing in transpose needs `measure`.

## 3. What fingertree would have pushed back on

Honest tally. **Confirmed-right guesses (no pushback):** instance objects over bare carriers (§2.1); no registered `Monoid<size_t>` (`has_monoid` sentinels, monoid.test.cpp:73-80, were argued from fingertree's rope/random-access needs and fingertree now lives happily without one); `dual_monoid`/`tuple_monoid` as instance combinators (§2.1, §2.2); non-consuming traversal default (§2.6); passing instances by value (§2.2 — the fingertree copies `d_monoid` per node build and nobody cares); `overloaded` in transpose (§1).

**Pushbacks fingertree would have made, with concrete proposed changes:**

1. **(blocking) Traversable/Functor over a parameter-dependent shape.** Proposed change: a decision slug in transpose on "Traversable for a type whose shape depends on the element type", recommending that consumers register only the slice where the structure is a genuine type constructor, with a worked example (`FingerTree` count slice vs. measure-changing `traverse'`). fingertree code change per §2.7.
2. **(significant) Monoid instance properties.** `is_commutative` (and perhaps `is_idempotent`) as optional declared members, re-exported by combinators, following `consumes_rvalue_structure`. Lets `reversed()` keep `has_unit_count_measure`, lets a future fingertree choose a lazy structural reverse ("possible only for commutative monoids", finger_tree.hpp:1252-1253), and gives `fold_map` a legal reassociation/parallel-fold hook. Not a wording change for P3200 (Monoid is unproposed) but a `monoid.hpp` change.
3. **(significant) Adjoined-identity min/max.** fingertree carries `optional_min_monoid`/`optional_max_monoid` because transpose's are saturating. These are general-purpose and belong beside `first_monoid<T>` in `monoid.hpp` (which is already over `std::optional<T>`, monoid.hpp:174-187). The "one definition of the machinery" invariant (SKILL.md) is being honoured for the mechanism and quietly not for the instance vocabulary.
4. **(significant) Allocator-blindness of the typeclass results.** fingertree should insist that a Traversable instance over an allocator-aware structure reconstruct with the source allocator; transpose cannot help, but the paper for the mechanism could say "the instance author is responsible for carrying what the type constructor fixes". Record as a consequence under the §3.1 slug.
5. **(minor) `fold_left` via `std::function`.** A consumer that advertises zero global allocation under PMR (pmr_probe) cannot route `fold_left` through the derived path. Either supply natives in fingertree or finish the open item in `derived-op-constant-evaluation`.
6. **(minor) A native `length`/`empty` hook.** `derive_foldable::length` prefers `impl.length` (fold.hpp:285-307). FingerTree's count-slice `measure()` is O(1); the Impl could supply `length` for that slice. Today `foldable<FingerTree<int>>.length(t)` is O(n). Nothing in the mechanism prevents it; nobody wrote it.

**Where I think the owner's hypothesis was wrong**: the Monoid redesign of 2026-10-07/08 *was* fingertree's design resistance, delivered through the decision log rather than through a drafted paper. What never happened is the same exercise for Foldable/Traversable/Functor, because fingertree's instances were written as mechanical ports with `flatten()`+`from_sequence()` and the merge kept them unchanged (ledger :65).

## 4. Paper C readiness

**Verified:** no draft exists (`papers/` holds `D3200R0.md` and `algorithms-for-trees.md` only; README.md:22 says "not yet drafted"); `docs/fingertree/README.md` is the Copier template with `(... TODO: description)`, `std::todo`, `PnnnnRr`, "Usage TODO" (:15-17, 28-30), and a supported-platforms table claiming C++17 on GCC 11 (:49-59) that contradicts the C++26 floor (CMakeLists.txt:26). STATUS.md:523-528 lists three fingertree items, all unactioned since 2026-07-13. The essay calls it "P4325" (trees-and-transposition-for-29.md:115, EDIT-LETTER.md:66,180, CRITIQUE.md:81,348) while the skill says D4325R0; one of these is wrong and both will be public.

**Design questions that must be settled before drafting** (ordered by how much paper text they gate):

1. **What is proposed: a container, a concept, or both?** The essay says "a persistent measured sequence abstraction … without committing to one implementation" (essay md:115); the code is one implementation with a four-parameter class and four wrappers. The wrappers take a fully specialized `Tree` parameter "robust to drift" (random_access.hpp:435-439) — that is a concept waiting to be named. Decide whether `measured_sequence<S>` is the proposal and `FingerTree` the reference implementation.
2. **The Traversable/Functor question (§2.7-2.8).** Which slice is Traversable; what `traverse` returns; allocator propagation; whether `fmap` exists. This also decides whether Paper C has a *normative* dependence on P3200 (today: the Foldable/Traversable instances are "implementation not wording", STATUS.md:526-527 — fine, but then the paper must not promise `traverse` works on a measured sequence).
3. **One `measured` object or `MONOID` + `MEASURE_POLICY` (§2.10).** Changes the class template signature, every example, and the measure-based-splitting doc.
4. **Container requirements honesty.** `static_assert(std::ranges::sized_range<FingerTree<int>>)` (finger_tree.test.cpp:690) holds for every instantiation while `size()`, `begin()`, `end()` are O(N) for non-count measures (finger_tree.hpp:975-984, iterator.hpp:414-432). Either `sized_range` is conditional on the count slice, or the paper states that `size()` is O(N) for custom measures (unusual for a standard container). Same for `operator==` via `flatten()` (:998-1004) allocating two vectors.
5. **Stateful instances across operations.** Fix §2.2's `append` hole, then decide: is mixing two trees with equal instance types and different states a precondition (document, assert in debug), or does `append` compare instances when `MONOID` is `equality_comparable`? The ruling leaves it to the consumer; Paper C is the consumer.
6. **Naming surface.** `cons`/`snoc`/`view_l`/`view_r`/`head`/`tail`/`init`/`last` alongside `front`/`back`/`push_*`-shaped wrapper names and the `is_empty` back-compat alias (:972-973). STATUS [S-6] (STATUS.md:427-436) opens exactly this and is "partially resolved". The `fold_left`-is-Foldable's ruling there (:433-436) means the sequence quotient interface is derived, not member — the paper needs that settled.
7. **`reversed()` type and commutativity (§2.2).**
8. **Allocator design as proposed text.** The allocator doc is the most paper-ready material in the library, but the `allocate_spine` placement-new technique (finger_tree.hpp:363-403) is implementation experience, not wording; decide what of the Lakos argument is normative.

Smaller hygiene before drafting: the Doxygen `@tparam Tree` comments say "Default: FingerTree — correct at all sizes", a leftover from the `finger_tree4`-era comparisons (finger_tree.hpp:7,11 still say "Like finger_tree4"); compile-time-design.md and measure-based-splitting.md cite `include/beman/fingertree/tree/finger_tree.hpp` and `tests/beman/fingertree/…` paths that no longer exist (compile-time-design.md:733, 796; measure-based-splitting.md:255); the example (`examples/fingertree/finger_tree.cpp`) uses no transpose verb, so the consumer relationship the papers describe is not shown anywhere runnable.

## 5. Test discipline compliance

**DEV-01 (non-idempotent, order-sensitive algebras; monoid tests need a non-commutative monoid).** *Partially compliant.*
- Foldable: `StringFoldMap` (foldable.test.cpp:74-80) folds under `Monoid<std::string>` and checks `"abcd"` — non-commutative, order-sensitive. Good.
- Measure monoids: every measure monoid exercised is commutative — `sum_monoid<size_t>`, `max_monoid<size_t>`, `modular_sum`, `weighted_monoid` (sum), `PriorityMonoid` (sum/min/max), `weight_monoid` (sum). Consequently `dual_monoid`'s flipped `combine` (monoid.hpp:201-204) is **never observable** in any fingertree test: `ReversedMeasureOnCommutative` (:616-626) says so in its name, and `ReversedNonCommutativeElementOrder` (:628-635) tests element order under the commutative count monoid — its comment "regardless of the monoid's commutativity" is about `T`, not the measure. A rope whose measure is the chunk text under `Monoid<std::string>` (so `measure()` is the concatenation) would make `reversed().measure()` differ from `measure()` and pin that `dual_monoid` is doing its job. **Significant gap**, and exactly the "idempotent or commutative-monoid-only test algebras cannot be the sole correctness gate" rule (DECISIONS.md:87).
- `WeightedSplitAtMeasure` (:472-488) uses a non-idempotent sum, fine.

**DEV-03 (no bare CTAD on wrapped types).** *Compliant.* All `FingerTree` uses name the full type or an alias (`FT{}`, `Tree{}`); `dual_monoid<MONOID>{d_monoid}` names the argument (finger_tree.hpp:1259). The only CTAD is `overloaded{…}` (an accepted idiom everywhere in the repo) and `std::optional{x}` in tests, neither a wrapped type under DEV-03's definition.

**DEV-04 (`constexpr` at declaration on every CRTP-derived operation; each verb covered by a `static_assert`).** *N/A on the derived side, unmet on the instance side, and structurally unmeetable.* Derived operations are transpose's and are `constexpr` since `4e687ae`. fingertree's Impl members are not `constexpr` (and cannot be meaningfully: `shared_ptr`). The 30 `static_assert`s in fingertree tests are all type-level (`is_same_v`, range concepts, `!has_fold_map`); none evaluates an operation at compile time, and none asserts `foldable_object<…>` or `traversable_object<…>` conformance of the registered objects — the one compile-time check that *is* available and would have caught the ledger's open question (:67). Recommendation: add `static_assert(beman::transpose::foldable_object<std::remove_cvref_t<decltype(foldable<FT>)>, FT>)` and the traversable twin for each of the five registrations.

**Equivalence laws.** None for the Foldable/Traversable instances (§2.1: the homomorphism law; also `traverse(identity-ish) ≡ copy` and `fold_map(f, t, m) ≡ ranges::fold_left(t.flatten() | transform(f), m.identity(), m.combine)` would pin the instances to already-tested primitives).

**Coverage of the wrappers' typeclass instances.** *Absent.* `finger_tree_rope.test.cpp`, `_random_access.test.cpp`, `_priority_queue.test.cpp`, `_interval_index.test.cpp` contain no `foldable`/`traverse` call (grep). Eight registered instances have zero tests; only `FingerTree`'s two are exercised.

## Appendix: file:line index of the load-bearing evidence

- Instance keying and rationale: `include/beman/fingertree/tree/finger_tree.hpp:43-54, 291-300, 338-348, 959-960`
- `append` rebuild under `MONOID{}`: `finger_tree.hpp:1183-1189` vs correct sites `:916, :931, :947`
- `reversed()` and dual: `finger_tree.hpp:1241-1260`; O(N) consequences `finger_tree_iterator.hpp:414-432`
- Traversable rebuild to default tree: `include/beman/fingertree/tree/finger_tree_traversable.hpp:14-26, 42-62, 77-80`
- Foldable Impl: `include/beman/fingertree/tree/finger_tree_foldable.hpp:18-52`
- Wrapper-local monoids: `include/beman/fingertree/tree/finger_tree_priority_queue.hpp:207-258`
- transpose monoid vocabulary: `include/beman/transpose/monoid.hpp:20-67, 117-153, 174-187, 195-230`
- Foldable base and `std::function` fold programs: `include/beman/transpose/fold.hpp:25-69, 143-155, 182-250, 283-355`
- Consuming traversal (persistent-structure accommodation): `include/beman/transpose/traverse.hpp:33-69, 116-117`; `papers/D3200R0.md:215-240`
- Rulings: `docs/transpose/decisions.md:2964-3172` (monoid-selection, fingertree bullets at :3023-3041, consequences :3044-3056), `:3174-3206` (registered-instance-state), `:3445-3471` (derived-op-constant-evaluation)
- Merge adaptation: `docs/coordination/machinery-deduplication.md:9-19, 54-71, 158-166`; commits `af05316`, `046bcc9`, `bc9d892`
- Tests: `tests/beman/fingertree/finger_tree.test.cpp:20-57, 616-660, 716-760`; `finger_tree_foldable.test.cpp:62-80`; `finger_tree_traversable.test.cpp:13-51`
- Paper C status: `README.md:22`; `docs/fingertree/README.md:15-30, 49-59`; `docs/coordination/STATUS.md:523-528`; essay `docs/coordination/essay/trees-and-transposition-for-29.md:115`; skill `.claude/skills/tree-proposals-coherence/SKILL.md` ("Paper numbers in flux")
