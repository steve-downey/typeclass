# Collapsing the typeclass machinery onto one definition

Plan for the work the [2026-10-12 SYNC entry](SYNC.md#2026-10-12--the-three-libraries-merge-into-bemantypeclass) names as the point of the merge.
Stages carry a reading-order number and a slug; cross-reference them by slug only.
Open questions live in the same namespace, under [Questions](#questions).

## What is duplicated

| Copy | Canonical | How far apart |
|---|---|---|
| `fingertree/typeclass/typeclass_base.hpp` | `transpose/detail/typeclass_base.hpp` | Near-copy, resynced in `af05316`; adds a local `remove_cvref_t` alias (19 uses in `tree/*`), lacks `has_instance_v` |
| `fingertree/typeclass/monoid.hpp` | `transpose/monoid.hpp` | Near-copy, already drifted: still spells the lookup `typeclass::monoid_v<T>`, which transpose renamed to `monoid<T>` on 2026-10-10 ([lookup-variable-naming](../transpose/decisions.md#lookup-variable-naming)) |
| `fingertree/typeclass/foldable.hpp` | `transpose/fold.hpp` | Token-identical after renaming; lacks the operation objects |
| `fingertree/typeclass/functor.hpp` | `transpose/functor.hpp` | Older snapshot (`Functor`, `functor_typeclass`); **no users in fingertree** |
| `fingertree/typeclass/applicative.hpp` | `transpose/apply.hpp`, `detail/applicative_derivation.hpp` | Older design: `pure` + `apply` basis, `apply_pure`, `apply_pure_with` — all removed by ruling ([SYNC 2026-07-14 entry 5](SYNC.md)). Internal users touch only `pure`, `invoke` and `applicative_value_t` |
| `fingertree/typeclass/traversable.hpp` | `transpose/traverse.hpp` | Older snapshot; `traverse` is a free function there and an operation object here, same argument order |
| `fingertree/fixpoint/overloaded.hpp` | `tree_algorithms/overloaded.hpp` | Identical body. **transpose has no `overloaded`** |
| `tree_algorithms/functor.hpp` | `transpose/functor.hpp` | Same mechanism, pre-ruling spellings (`Functor`, `functor_typeclass`, `has_functor_instance`). Its own comment says it is a "consumer-side stub … to be replaced by a dependency on" the P3200 facility |

Not duplicated, despite the SYNC entry: `tree_algorithms/fix.hpp` has no counterpart anywhere, and stays where it is.
The two `BinaryTree` ports (`tree_algorithms/binary_tree.hpp`, `examples/transpose/binary_tree.hpp`) and tree_algorithms' `layer_fold_typeclass` vs transpose's `foldable` are related but out of scope; see [binary-tree-ports](#binary-tree-ports) and [layer-fold-and-foldable](#layer-fold-and-foldable).

## Target shape

`beman.transpose` is the root of the build graph and the only home of the mechanism.
`beman.tree_algorithms` and `beman.fingertree` each link `beman::transpose`; neither links the other.

```txt
beman.transpose  <──  beman.tree_algorithms   (lookup tier only)
                 <──  beman.fingertree
```

This is the dependency the papers already describe — Papers C and D *consume* P3200's mechanism — made real in the build instead of simulated by copies.
It does not change any paper's normative independence:

- Paper D's explicit tier (`fix.hpp`, `box.hpp`, `child_slot.hpp`, `recursion_schemes.hpp`, `fold_map.hpp`) stays mechanism-free, and [explicit-tier-isolation](#explicit-tier-isolation) makes that a test, not a convention.
- Papers C and D still have no dependence in either direction, normative or in the build.

## Stages

### Stage 1 — [ruling-before-code](#ruling-before-code)

Tree_algorithms' DECISIONS.md is frozen with "change the doc first, then the code."
Before any header moves, record the decision there and in the companions:

- `docs/tree_algorithms/DECISIONS.md` Decision 4 and the fold-family scope entry: the lookup header stops being a *stub* and becomes a real consumer of `beman.transpose`; the lookup objects become `functor<Layer>`, `layer_fold<Layer>` and `project<Tree>` ([consumer-lookup-names](#consumer-lookup-names)).
- `docs/tree_algorithms/notes/extraction-plan.md` §§ on Decision 4 and the C/D independence paragraph.
- `.claude/skills/tree-proposals-coherence/SKILL.md`: the invariants paragraph on lookup overloads, and drop the "copy of transpose, keep in step" hazard once it no longer exists. Its division-of-labor line "P3200 proposes only what its three-domain proof forces" gains the [overloaded-home](#overloaded-home) exception: P3200 also proposes `overloaded`, as supporting vocabulary for writing algebras.
- `docs/transpose/decisions.md`: an entry, under a slug for the question, recording that P3200 proposes `overloaded` and why ([overloaded-home](#overloaded-home)).
- A SYNC entry pinning the start state.

Done when: the three docs agree and every open [question](#questions) blocking a later stage has a `Decided by`.

### Stage 2 — [fingertree-onto-transpose](#fingertree-onto-transpose)

First because it is the most mechanical and touches no paper text (Paper C is not yet drafted), and because its copies are already drifting.

- Delete `fingertree/typeclass/*`; include `<beman/transpose/{fold,monoid,traverse,apply}.hpp>` instead.
- Re-spell in `tree/*` and tests:
  - `Foldable<>`, `Traversable<>` become `derive_foldable<>`, `derive_traversable<>`.
  - `foldable_typeclass`, `traversable_typeclass` become specializations of `beman::transpose::foldable`, `beman::transpose::traversable`. These are 5 instances plus 7 test registrations.
  - `typeclass::sum_monoid`, `max_monoid`, `dual_monoid`, `tuple_monoid`, `product_monoid`, `monoid_value_t` become `beman::transpose::…`. There are 24 sites, including the `FingerTree` default template argument and `reversed()`'s return type.
  - The local `remove_cvref_t` alias becomes `std::remove_cvref_t`.
  - `beman::fingertree::traverse(...)` in tests becomes `beman::transpose::traverse(...)`.
- The 5 Traversable impls keep their `applicative.pure`/`applicative.invoke` bodies unchanged: `invoke` is a basis in transpose's design.
- `fingertree/CMakeLists.txt`: link `beman::transpose`; drop the 6 headers from the file set; `beman_install_library(beman.fingertree … DEPENDENCIES "beman.transpose")` so the installed config does `find_dependency`.
- Check transpose's `traversable_object` probe against the wrappers that have no `value_type` (Rope, RandomAccess, IntervalIndex); add `value_type` or don't assert the concept on them.
- `docs/fingertree/allocator-design.md`, `measure-based-splitting.md`: re-spell the names they quote.

Leaves `fixpoint/overloaded.hpp` for [overloaded-home](#overloaded-home).
Unblocked: [fingertree-public-names](#fingertree-public-names) is decided (no aliases).

### Stage 3 — [overloaded-home](#overloaded-home)

`overloaded` moves into transpose and P3200 proposes it ([overloaded-home](#overloaded-home)); both copies go.

- `include/beman/transpose/overloaded.hpp` and `overloaded.test.cpp`, in `beman::transpose`, from tree_algorithms' copy and tests, without the module guard (transpose has no module unit), with transpose's prolog, guard and clang-format style. Add it to transpose's header file set.
- Delete `fingertree/fixpoint/overloaded.hpp` and `tree_algorithms/overloaded.hpp`. Point `finger_tree.hpp`, `finger_tree_iterator.hpp` and tree_algorithms' headers, tests and examples at `<beman/transpose/overloaded.hpp>`.
- tree_algorithms' uses are in the lookup-tier example trees (`expression`, `fringe_tree`, `functors`), never the explicit tier, so [explicit-tier-isolation](#explicit-tier-isolation) still holds. Check that when the test lands.
- The module build needs the same global-module-fragment include as stage 5's `functor.hpp`.
- P3200 (`papers/D3200R0.md`): a section motivating `overloaded` for algebras, its design including the exhaustiveness catch-all and why it is a diagnostic rather than a fallback, prior art (P0051), and wording. `make wording-check` covers it once it is in the wording.
- Paper D: `overloaded{…}` in its two examples (`papers/algorithms-for-trees.md:44,67`) now refers to P3200's facility; say so where the examples first use it.

Pairs naturally with stage 2, since fingertree's copy is the last file left in `fixpoint/`.

### Stage 4 — [transpose-constexpr-derivations](#transpose-constexpr-derivations)

Prerequisite for stage 5, and a change to transpose alone.
transpose's derived operations are not `constexpr`: `derive_functor::fmap`/`replace`, and the 17 deducing-this members of `derive_foldable` (`fold.hpp:210` onward), likewise in `apply.hpp`, `traverse.hpp`.
tree_algorithms' DEV-04 requires `constexpr` at declaration on every CRTP-derived operation, with `static_assert` coverage, and its `functor.hpp` meets it today; switching it to transpose's base as-is would lose that.

- Add `constexpr` to every derived member of every `derive_*` base, and a `static_assert` per verb in the matching `.test.cpp`.
- `make wording-check`: if the P3200 synopsis shows these members, the wording changes with them.
- Record it in [docs/transpose/decisions.md](../transpose/decisions.md) under a slug for the question ("are the derived operations constant-evaluable?").

### Stage 5 — [tree-algorithms-functor-onto-transpose](#tree-algorithms-functor-onto-transpose)

- Delete `Functor`/`functor_typeclass`/`has_functor_instance` from `tree_algorithms/functor.hpp`. Either delete the header or leave it as the one include of `<beman/transpose/functor.hpp>`; the umbrella `tree_algorithms.hpp` is the only reason to keep it.
- Re-spell every specialization as `beman::transpose::functor<…>` with base `derive_functor<…>`. The sites are `binary_tree`, `expression`, `fringe_tree`, `functors`, `rose_tree`, `benchmarks/tree_algorithms/fix_unique_tree.hpp`, and the binary_tree, fringe_tree and rose_tree tests.
- Rename tree_algorithms' own lookup objects `layer_fold_typeclass`, `project_typeclass` and their `has_*_instance` helpers to `layer_fold`, `project` ([consumer-lookup-names](#consumer-lookup-names)).
- `recursion_schemes_lookup.hpp`'s `layer_fmap` dispatches through `transpose::functor` (or simply calls `transpose::fmap`); `has_functor_instance` becomes `has_instance_v`.
- `tree_algorithms/CMakeLists.txt`: link `beman::transpose`; installed config gains `find_dependency(beman.transpose)`.
- **Module build**: `tree_algorithms.cppm` includes the umbrella inside `export { … }`, which would pull transpose's names into `beman.tree_algorithms`'s purview. Include the transpose headers in the global module fragment (`module; #include … export module …`) so the later include is guarded out. Verify with `-DBEMAN_TYPECLASS_USE_MODULES=ON`; `import std;` alongside GMF std includes is the likely place for gcc to object.
- Paper D: `papers/algorithms-for-trees.md:188,296,522` name `functor_typeclass` (and line 188 the other two); re-spell, keeping the public-text rule (no slugs in the prose). `docs/tree_algorithms/notes/anchor-inventory.md`, `recursive-tree-algorithms.org`, and the two examples that name it.

### Stage 6 — [explicit-tier-isolation](#explicit-tier-isolation)

The invariant that the explicit tier has no mechanism dependence becomes checkable once a mechanism is actually reachable from the include root.
Add a CTest that preprocesses each explicit-tier header alone (`-M` or `-H`) and fails if any `beman/transpose/` header appears in its closure.
The explicit-tier headers are `fix.hpp`, `box.hpp`, `child_slot.hpp`, `recursion_schemes.hpp` and `fold_map.hpp`.
Could land as early as stage 2, which would make it a guard on stage 5.

### Stage 7 — [close-the-ledger](#close-the-ledger)

- A SYNC entry pinning the end state, correcting the 2026-10-12 entry's claim about `fix.hpp`.
- Remove the "copy of beman.transpose's …, keep the two in step" comments (gone with the files) and the README's statement that deduplication is pending.
- STATUS.md and the coherence skill's map.

## Outcome

Executed 2026-10-12 on `reintegrate`; every commit below builds and passes on its own.

| Stage | Commits |
|---|---|
| [ruling-before-code](#ruling-before-code) | `bfbdc04` |
| [fingertree-onto-transpose](#fingertree-onto-transpose) | `046bcc9` |
| [overloaded-home](#overloaded-home) | `34921e6` (move, history kept), `bc9d892` (fingertree's copy), `9e384d9` (documentation) |
| [transpose-constexpr-derivations](#transpose-constexpr-derivations) | `4e687ae`, recorded as [derived-op-constant-evaluation](../transpose/decisions.md#derived-op-constant-evaluation) |
| [tree-algorithms-functor-onto-transpose](#tree-algorithms-functor-onto-transpose) | `9ec82a9` (transpose, for the module build), `77180c5` |
| [explicit-tier-isolation](#explicit-tier-isolation) | `05ea8b4`, a compile check: one generated translation unit per explicit-tier header, `#error` on any beman.transpose include guard |
| [close-the-ledger](#close-the-ledger) | this entry, and the SYNC entry of the same date |

Found and fixed on the way, each its own commit:
- `7373a4b`: tree_algorithms' module unit did not build on gcc 15 or 16, and had not in the tree_algorithms repository either: `<cassert>` was first included inside the module purview.
- `223bd2c`: specgen 0.2.0 wraps synopses in `namespace std { }`, so `make wording-check` failed on every commit, the merge included, until the fragments were regenerated.

Left open:
- **P3200 text for `overloaded`**: the section motivating it for algebras, its design and the catch-all as a diagnostic, P0051 as prior art, and wording. Paper D's examples should then say they use P3200's `overloaded`. Neither is written.
- **Module-build tests**: the module unit builds, but tests and examples built against it include standard headers after `import`, which gcc 15 and 16 reject. Predates this work. Closed on 2026-10-14: the tests, examples and benchmarks now include the test framework's, the standard and beman.transpose's headers before the library's own, which are imports under the module build; whether the module build is required, advisory or waived stays with [module-build-status](review/2026-10-13-reunification/REVIEW.md).
- **`fold_left`/`fold_right` derived through `fold_map`** compose `std::function`, so they are not constant-evaluable; see the log of [derived-op-constant-evaluation](../transpose/decisions.md#derived-op-constant-evaluation).
- **`examples/tree_algorithms/fixpoint_tree_example.cpp`** defines its own `overloaded`, and `expression_algorithms.cpp` its own `example_*_typeclass` lookups. Both are self-contained teaching examples and were left alone.
- **gcc 14**: `examples/tree_algorithms/search_tree_on_fix.cpp` formats a `std::vector` with `std::format`, which libstdc++ 14 cannot; every test passes there. Predates this work.
- [fallback-overload-set](#fallback-overload-set), deferred by ruling.

## Verification, every stage

- `make TOOLCHAIN=gcc-16 test`: all three libraries.
- gcc-15: fingertree is skipped, and the other two still pass.
- A `-Werror` build.
- `-DBEMAN_TYPECLASS_USE_MODULES=ON`, from stage 5 on.
- Header sets verify.
- `cmake --install` and an installtest that `find_package`s fingertree and tree_algorithms *without* transpose found explicitly, to prove the `find_dependency` chain.
- `make wording-check`, `make -C papers D3200R0.html algorithms-for-trees.html`.
- `make lint`.

## Questions

### fingertree-public-names

- **Question:** When fingertree's monoids and lookup objects move to `beman::transpose`, does fingertree keep its old spellings (`beman::fingertree::typeclass::sum_monoid`, `foldable_typeclass`, …) as aliases?
- **Status:** DECIDED 2026-10-12
- **Decision:** No aliases, no back-compatibility. fingertree takes transpose's names directly.
- **Why:** fingertree's names were always provisional. The old spellings are the ones [lookup-variable-naming](../transpose/decisions.md#lookup-variable-naming) retired, and aliases would keep a second vocabulary alive. Accepted cost: the defaulted `FingerTree<T>` changes type identity (its monoid parameter becomes `beman::transpose::sum_monoid<std::size_t>`), and so do `PriorityMonoid`, `IntervalMonoid` and `reversed()`'s return type. A user-written applicative with an `apply` basis must become `ap` or `invoke`.
- **Decided by:** Steve
- **Log:**
  - 2026-10-12: decided as recommended.

### overloaded-home

- **Question:** Where does `overloaded` live, and which paper proposes it, given C and D must not depend on each other?
- **Status:** DECIDED 2026-10-12
- **Decision:** P3200 proposes `overloaded`, and the code lives in beman.transpose as `beman::transpose::overloaded`. tree_algorithms and fingertree use it from there.
- **Why:**
  - It is tiny, but it is just enough code that no one should have to write it again, so it is proposed rather than shipped as an unlabelled utility.
  - It is useful in P3200 for writing algebras.
  - tree_algorithms needs it even more, and fingertree less so.
  - transpose is already the root of the build graph and both consumers link it, so it adds no C↔D edge and no fourth package.
  - It is an exception to "P3200 proposes only what its three-domain proof forces": the line is amended in stage 1, not silently broken.
- **Still to do in the paper:** address the earlier `overload` proposal (P0051, "C++ generic overload function"; check its last revision and why it stalled), and present the catch-all as the log below describes it.
- **Decided by:** Steve
- **Log:**
  - 2026-10-12: Steve: propose it. The recommendation of an unproposed transpose utility is withdrawn.
  - 2026-10-12: Steve, on the catch-all: `overloaded` is overload resolution, not case analysis, so the catch-all is not a "match anything" fallback. A match-anything needs a type that does that. The `static_assert` only moves the error to a better diagnostic.
  - 2026-10-12: checked against gcc-16 / libstdc++ 16, visiting `variant<A, B, C>` with no case for `C`. Four effects bear on what the paper claims for the catch-all:
    - **Void visitor:** the better diagnostic lands as intended: "overloaded: unhandled variant alternative — add a case".
    - **Value-returning visitor:** the diagnostic is lost. The catch-all returns `void`, so libstdc++'s "std::visit requires the visitor to have the same return type for all alternatives" fires first. The catch-all cannot know the visitor's return type, so it can't be fixed just by changing its declared return type.
    - **Generic lambdas:** a deliberate generic lambda (`[](const auto&)` or `[](auto)`) alongside the catch-all is ambiguous, and the visit fails. So `overloaded{[](int){…}, [](const auto&){…}}`, the common fallback idiom, does not compile with the catch-all present.
    - **Convertible alternatives:** the catch-all also takes alternatives that would otherwise convert. A `double` alternative goes to the catch-all, an exact template match, instead of an `int` case. That catches a silent conversion, but it is a change in overload resolution, not only a diagnostic.
  - 2026-10-12: Steve: P3200 proposes it, useful there for algebras and needed even more in tree_algorithms, less so in fingertree; the code moves into beman.transpose. Set aside: its own paper and a fourth package, which the recommendation had favoured; and Paper D, which would have added a C→D edge through fingertree.
  - 2026-10-12: Steve: `overloaded` keeps the catch-all; the form is long-established in his code and does not change. A deliberate generic lambda inside `overloaded` being ambiguous is accepted as part of that design, not a defect to fix. The form that admits a generic fallback is a separate type with a worse name, not a qualified spelling of this one (no `exhaustive_overloaded`). See [fallback-overload-set](#fallback-overload-set).

### consumer-lookup-names

- **Question:** Do tree_algorithms' own lookup objects `layer_fold_typeclass<Layer>` and `project_typeclass<Tree>` follow [lookup-variable-naming](../transpose/decisions.md#lookup-variable-naming)'s plain-name rule (`layer_fold<Layer>`, `project<Tree>`) once `functor_typeclass` becomes `functor`?
- **Status:** DECIDED 2026-10-12
- **Decision:** Yes: `layer_fold<Layer>` and `project<Tree>`, renamed in stage 5 alongside `functor`.
- **Why:** The same reasons as [lookup-variable-naming](../transpose/decisions.md#lookup-variable-naming); otherwise one lookup surface is spelled two ways in the same paper. It is a Paper D naming change, so it is recorded in DECISIONS.md in stage 1.
- **Decided by:** Steve
- **Log:**
  - 2026-10-12: decided as recommended.

### layer-fold-and-foldable

- **Question:** Is tree_algorithms' `layer_fold_typeclass<Layer>` (an elementwise fold over one layer) a Foldable instance for the base functor under another name, and if so should it be one?
- **Status:** OPEN — out of scope for this plan
- **Recommendation:** Leave it. Foldable's *proposal* home is Paper D while its *code* lives in transpose's `fold.hpp` as evidence; collapsing them is a paper-scope question, not a duplicate-code one, and the coherence skill already forbids growing a third definition — which removing fingertree's copy in stage 2 helps with.
- **Decided by:**
- **Log:**

### binary-tree-ports

- **Question:** Should the two `BinaryTree` ports become one?
- **Status:** OPEN — out of scope for this plan
- **Recommendation:** Leave it for now. Their instances are disjoint: transpose's registers Foldable, Applicative and Traversable; tree_algorithms' registers the layer functor, projection and layer fold. transpose's is an example. Unifying them would make a transpose example depend on tree_algorithms, which inverts the build order.
- **Decided by:**
- **Log:**

### fallback-overload-set

- **Question:** What is the overload-set type that admits a deliberate generic fallback, and is it proposed alongside `overloaded` (which would put it in P3200 too, per [overloaded-home](#overloaded-home))?
- **Status:** DEFERRED. No work on it until every other stage and question in this plan is cleared. Decided: the type is distinct from `overloaded`, and it is not proposed unless the committee requires it. Open: its name, held as a candidate list so it is ready if it is required.
- **Decision so far:**
  - `overloaded` is the exhaustive form, with the consteval catch-all, and keeps its name.
  - The plain form, `struct : Ts... { using Ts::operator()...; }`, is a separate type. It gets a deliberately worse name, because it is the less-preferred form: no exhaustiveness check, and an alternative with no case can silently go to a generic lambda or a converting overload.
  - `overloaded` is not renamed or qualified to make room for it.
- **Why:** Overload resolution is not case analysis, so `overloaded`'s catch-all is a diagnostic, not a "match anything" case. That makes it incompatible with a user's own generic lambda (`[](const auto&)` and `[](auto)` are both ambiguous with it; see the [overloaded-home](#overloaded-home) log). Code that wants a generic fallback needs a type without the catch-all.
- **Candidate names** (none chosen; each says it is the lesser form without qualifying `overloaded`):
  - `partially_overloaded`: preferred. It uses pattern matching's own term: a match set is *exhaustive* (total) or *partial*. Grammatically, `overloaded` is a participle, so it takes an adverb: "partially overloaded" parses and "partial overloaded" does not. It fits the type loosely: with a generic-lambda fallback the set does cover every case. What the type lacks is the *check* for coverage, so "partial" is the conventional term and "unchecked" the literal one.
  - `unchecked_overloaded`: says exactly what is missing, and reads as a warning at the call site.
  - `nonexhaustive_overloaded`: the same idea, stated as the property it lacks.
  - `overloaded_with_fallback`: names the one reason to choose it.
  - `loose_overloaded` or `permissive_overloaded`: deliberately unflattering.
  - `overload_set`: reserved, do not use. It is the right name for a future language feature that captures a name's overload set as an object and forwards it for resolution later. Nothing is proposed now, but it has been wanted for a long time. A library type should not take that name.
  - `overload`: avoid. It is P0051's spelling, one letter from `overloaded`, and reads as the better-sounding name, which is backwards here.
- **Decided by:** Steve (that the type exists separately, that the worse name goes on it, and that it is proposed only if required)
- **Log:**
  - 2026-10-12: recorded from the gcc-16 check of the catch-all's interaction with generic lambdas.
  - 2026-10-12: Steve: `partially_overloaded` preferred over `partial_overloaded`, which reads as ungrammatical (an adjective cannot modify the participle `overloaded`). `overload_set` is reserved for a feature that models the language's overload set exactly.
  - 2026-10-12: terminology for the paper, if it is written. *Exhaustive* vs *partial* is about whether the set of cases covers every input. *First-match* vs *best-match* is the selection strategy. A case statement and the pattern-matching proposals (P1371, P2688) are first-match: the arms are tried in order. Overload resolution, and so `overloaded`, is best-match: the most specific candidate wins, regardless of order. *Refutable* vs *irrefutable* is about whether a single pattern can fail, and does not apply here.
  - 2026-10-12: Steve: not proposed unless it is required; keep a list of names ready; no work on it until the rest of this plan is done.
