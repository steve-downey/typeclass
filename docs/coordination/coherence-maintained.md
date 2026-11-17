# Coherence as a maintained property

Stage 6 of the [coherence plan](coherence-plan.md), on branch
`tree-coherence-maintained`, stacked on the
[structural adapter](coherence-structural-adapter.md). It closes the plan.

## What this stage adds

**A cross-component matrix.**
[`coherence_matrix.test.cpp`](../../tests/beman/typeclass_tree_adapters/coherence_matrix.test.cpp)
has one row per structure that registers payload typeclasses, across all
three libraries, and one column per property they promise in common. Every
cell is the same code instantiated for the row, so a structure that stops
honouring a contract the others keep fails there, whichever library it is
in. The rows are `std::vector`, `FingerTree`, the three payload-polymorphic
wrappers, `BinaryTree`, `BinaryTreeFix`, `FringeTree` and `RoseTreeFix`. The
columns are:

| Column | What is checked for every row |
|---|---|
| Registered objects | each instance is found and satisfies its typeclass's deep object concept |
| Order | `fold_map` with a noncommutative monoid, `fold_left`, `fold_right`, `to_vector`, `length`, `empty` |
| Functor laws | identity keeps the type and the payloads; composition agrees with two maps |
| `optional` | success, failure, traverse with a pure lift is `fmap`, `transpose` is identity traversal |
| `expected` | success, and the first failure in documented order is the one reported |
| Graded `expected` | short-circuit keeps the first witness; the accumulating policy keeps every distinct one |
| Unit effects | a void-valued effect yields the same shape holding units |
| Lanewise | `zip_list`: one structure per lane, each with the source's shape |
| Native collection | a policy's `collect` runs once; effects are composed once, in order |
| Pairwise | a policy with no `collect` is sequenced without one |
| Changing carrier | a policy whose `pure` and composition differ in carrier type is sequenced |
| Structure | the structural fold agrees with the Foldable, and the monoid bridge accepts exactly the trees |

One further case checks that a PMR finger tree keeps its resource through
every one of those contexts and through a structural round trip. An interval
index's endpoints are compared in every traversal column, including the
second lane of the lanewise one, where a reconstruction that was used up by
the first lane would show.

The matrix passed as written. It found no new defect; its value is that the
agreement is now pinned in one place instead of inferred from four suites.

**The public matrix.** The
[adapter guide](../typeclass_tree_adapters/README.md#support-across-the-libraries)
carries the same rows and columns as a table for readers, and says where
Rope and Expr stand outside it.

**The paper.** [`papers/algorithms-for-trees.md`](../../papers/algorithms-for-trees.md)
no longer says `traverse` for a fixed-point tree is derivable through the
layer's mapping function. It says what is true: mapping a layer's children
does not order their effects and knows nothing about the payload, so the
reference implementation supplies `traverse` for the tree families that
declare those things. The synopsis and anchors are unchanged because no
signature changed. The planning note that first made the claim carries a
dated correction.

**Allocator and measure documentation.**
[`allocator-design.md`](../fingertree/allocator-design.md#structural-construction)
states the structural builder's rules: one allocator, one monoid instance,
every node from that allocator, and why a foreign spine is asserted against
and not rebuilt.

## The reviewed defects

Each of the nine defects in the
[2026-11-08 review](review/2026-11-08-coherence/README.md) is closed by a
repair in stages 1–4 and held closed by a test in the owning suite. The
review's standalone probes are evidence of what was found; these are the
checks that keep it fixed.

| Defect | Owning test |
|---|---|
| Fingertree traversals ignored native collection | `finger_tree_traversable.test.cpp`: NativeCollectionPreservesOrderAndEndpoints, CollectionAllowsChangingCarrier; matrix: NativeCollectionIsUsedOnce, CollectionMayChangeCarrier |
| Allocator-mismatch append reset the monoid | `finger_tree_pmr.test.cpp`: unequal-resource append preserves the monoid instance |
| Binary example duplicated effects | `transpose/binary_tree.test.cpp`: traversal composes every effect once in fold order; `tree_typeclasses.test.cpp`: independent binary ports agree |
| Traversal erased measure and allocator | `reconstruction.test.cpp`: weighted reconstruction, count-policy rebinding under PMR; matrix: ConfigurationSurvivesEveryContext |
| Layer Functors used the wrong callable category | `tree_algorithms/conformance.test.cpp`: layer declarations match their callable and child type |
| Element metadata disagreed with the deep concepts | `fingertree/conformance.test.cpp`, `tree_typeclasses.test.cpp`; matrix: RegisteredObjectsSatisfyTheirDeepConcepts |
| Rope's traversal promised arbitrary results | `fingertree/conformance.test.cpp`: Rope advertises only its chunk Foldable; `finger_tree_traversable.test.cpp`: Rope chunk traversal |
| Fingertree Foldable omitted the monoid constraint | `fingertree/conformance.test.cpp`: Foldable conforms over payloads and rejects invalid calls |
| Traversal copied at both ends | `reconstruction.test.cpp`: move-only sources and fresh results; no payload copies |

The missing structural fingertree support, which that review listed under
coverage and the [2026-11-16 review](review/2026-11-16-layer-coherence/README.md)
named `fingertree-structural-adapter`, is closed by stage 5. Its probe's
negative result is replaced by positive checks in
`typeclass_tree_adapters/conformance.test.cpp`: the projection and the
monoid bridge are registered for a FingerTree once the adapter is included.
The negative cases that remain there are intended: the wrappers are not
registered, and calls that would copy a move-only payload are constrained
out.

## Gates

- **C++26, GCC 16, ASan and UBSan:** full build without diagnostics,
  **851/851 tests** (742 after stage 5; the matrix adds 109 cases and 707
  assertions). All standalone interface-header checks pass.
- **C++26, Clang 22.1.8 with GCC 16's standard library, both sanitizers:**
  the matrix passes with the same counts. Built by hand, for the reason
  given in the [stage 5 note](coherence-structural-adapter.md#validation).
- **C++23, GCC 16 and Clang 22:** the tree_algorithms payload, conformance
  and fold_map suites pass (413, 105 and 13 assertions on each compiler),
  built by hand. This stage and stage 5 change no header a C++23 build
  compiles. A full C++23 configuration was not run: the repository's GCC
  toolchain file fixes the dialect at C++26.
- **Modules, GCC 16, Debug:** 724/724, run at stage 5. Nothing this stage
  adds is built in that configuration.
- **Installed packages:** all four downstream consumers pass.
- **P3200 wording:** `scripts/wording-verify.sh` reports the checked-in
  wording matches the headers; no transpose header changed.
- No supported registration fails its own deep concept in its advertised
  domain: the matrix asserts all three concepts for all nine rows.
- No advertised cross-library algorithm lacks an adapter.

Not run: MSVC, and the opt-in P2300 evidence, whose dependency is not
installed here. No sender-specific tree case was added; the generic
collecting context covers the same sequencing route in the default suite.

## What the plan did not settle

The plan's stages are complete. The
[2026-11-16 review](review/2026-11-16-layer-coherence/README.md) raised
questions the plan never asked, and they are open. Four need a ruling before
any code:

- [fold-map-meaning](review/2026-11-16-layer-coherence/README.md#fold-map-meaning):
  one name, kind and argument order for `fold_map`.
- [proposal-homes](review/2026-11-16-layer-coherence/README.md#proposal-homes):
  which paper proposes Functor, and which proposes the Monoid vocabulary.
  This stage corrected the traversal sentence only.
- [payload-foldable-copies](review/2026-11-16-layer-coherence/README.md#payload-foldable-copies):
  whether payload Foldable may walk positions directly.
- [instance-header-placement](review/2026-11-16-layer-coherence/README.md#instance-header-placement):
  whether instance headers merge into type headers. The adapter's
  `project<FingerTree<...>>` registration is deliberately outside
  fingertree's headers, which is the same question from the other side.

The remaining findings there are repairs that can proceed without one:
[lookup-tier-rejection](review/2026-11-16-layer-coherence/README.md#lookup-tier-rejection),
[shape-contents-derivation](review/2026-11-16-layer-coherence/README.md#shape-contents-derivation),
[fingertree-foldable-natives](review/2026-11-16-layer-coherence/README.md#fingertree-foldable-natives),
[deep-concept-witness](review/2026-11-16-layer-coherence/README.md#deep-concept-witness)
and [vocabulary-drift](review/2026-11-16-layer-coherence/README.md#vocabulary-drift).
