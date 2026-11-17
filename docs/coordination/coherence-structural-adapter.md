# Structural fingertree adapter

Stage 5 of the [coherence plan](coherence-plan.md), on branch
`tree-coherence-structural-adapter`, stacked on the
[2026-11-16 layer coherence review](review/2026-11-16-layer-coherence/README.md).
It closes that review's
[fingertree-structural-adapter](review/2026-11-16-layer-coherence/README.md#fingertree-structural-adapter)
finding. The declarations it settles are recorded in the
[contract note](coherence-contracts.md#stage-5-declaration-details).

## Behavior

A finger tree can now be driven by every structural verb in
beman.tree_algorithms, in its own representation and through a Fix form, by
including one opt-in header. Neither library depends on the other.

**The access hook.** `beman/fingertree/tree/finger_tree_structure.hpp` exposes
the representation as it is and names no algorithm library.
`FingerTreeLevel<TREE>` is the tree at one level of the spine, in its real
Empty, Single or Deep form; level 0 is the sequence.
`FingerTreeNode<T, MONOID>` is one element: a Leaf, a Node2 or a Node3. Both
are owning values that share the immutable structure they describe, so
reading copies no payload and nothing read can dangle. Both are one C++ type
at every depth. `FingerTreeStructureBuilder<TREE>` constructs the same forms
under one allocator and one monoid instance and computes every cached
measure. Heights and levels travel with the parts, so the builder's
preconditions are checked in O(1). An inner spine level never escapes as a
tree: `sequence()` is the only way out and requires a level that is one.

`FingerTreeRope` gains the read-only `backing_tree()` the other three wrappers
already had, so all four reach structural access the same way.

**The adapter.** `beman/typeclass_tree_adapters/finger_tree.hpp`, in the new
target `beman::typeclass_tree_adapters`, supplies what the verbs need:

- `FingerTreeF<P, A>`: one layer. Six alternatives, three for the forms of a
  spine level and three for the forms of an element. It is a class derived
  from its variant, so the instances are registered on a type the header
  owns and not on a `std::variant` specialization.
- `FingerTreePart<TREE>`: the one position type for both recursive sorts, a
  finite tagged value. It is the handle a projection hands out and the piece
  an embedding assembles.
- `finger_tree_project` and `FingerTreeEmbed<TREE>`: the projection and the
  configured embedding. A projected Leaf shares its payload through
  `FingerPayloadRef<T>`; `finger_tree_project_owned` copies it instead, for
  conversions that must own what they produce.
- The layer Functor, the elementwise layer fold, and the lookup
  registrations `project<FingerTree<...>>`, `layer_fold<FingerTreeF<P, A>>`
  and `functor<FingerTreeF<P, A>>`.
- `FingerTreeFix<T>`, `to_fix` and `from_fix`.

Including the header also makes the monoid-object bridge
`tree_algorithms::fold_map(function, tree, monoid)` available for a
FingerTree, because that bridge is constrained on exactly these
registrations.

**Order and laws.** A Deep folds its left digit, its spine, then its right
digit, each left to right; a node folds its children left to right. That is
the sequence order, so the structural elementwise fold agrees with
fingertree's own Foldable for a noncommutative monoid. Only an Empty layer
yields the identity.

**Capability wrappers.** The wrappers are not registered as structural trees.
Their structure is their backing tree's, and a caller names it:
`fold_map<Result>(f, combine, identity, index.backing_tree())`. The payload a
structural algorithm sees is then the backing tree's, an `Interval<T>` for
the interval index and a string chunk for the rope. A registration keyed on
the wrapper would have had to choose between that payload and the wrapper's
`value_type`, and the monoid bridge would have chosen wrongly.

## Boundaries

- **Embedding has preconditions.** A layer handed to `FingerTreeEmbed` must
  be a well-formed finger tree layer: the right sort in each position, digits
  of one to four elements of one height, a spine one level below its digits.
  Layers projected from a tree satisfy this by construction. A hand-written
  coalgebra must, and violations are caught by `assert`, as fingertree's own
  preconditions are. There is no error channel in an algebra's return type.
- **Owned payloads map by copy.** The layer Functor copies a Leaf's payload
  slot, as every layer Functor in tree_algorithms does. A shared slot is
  cheap to copy and works for move-only payloads; an owned move-only payload
  cannot be mapped, so move-only trees are read-only through this adapter.
  `to_fix`, `finger_tree_project_owned` and the monoid bridge are constrained
  out for them.
- **Sharing costs reference counts.** Every part and every shared payload
  holds a share of a node. A structural fold therefore adjusts reference
  counts once or twice per node. This buys layers that cannot dangle;
  fingertree's own Foldable remains the fast path for a plain fold.
- **Module builds leave the adapter out.** With
  `BEMAN_TYPECLASS_USE_MODULES=ON` the target is not created. The header
  would include fingertree's standard headers textually and then import
  beman.tree_algorithms while also specializing that module's variable
  templates, and that combination has not been validated.
- **The Fix form has no payload metadata.** `FingerTreeFix<T>` folds through
  the explicit and the four-argument lookup `fold_map`. It is not registered
  for the monoid bridge or for payload typeclasses; the tree itself already
  has those.

## Validation

- GCC 16, C++26, AddressSanitizer and UndefinedBehaviorSanitizer: full build
  without diagnostics; **742/742 tests** (712 before this stage). The 30 new
  cases are 12 for the access hook (907 assertions), 14 for the adapter
  (1,079 assertions), 3 for conformance, and the self-checking example.
- All standalone interface-header checks pass, including the two new headers.
- The gate's cases are covered: empty, single, every digit size, Node2 and
  Node3, and trees with five or more spine levels; an order-sensitive shape
  algebra; a noncommutative text measure; a monoid with state; PMR allocation
  with the default resource forbidden during the build.
- Direct, Fix-based and fused folds agree on 56 trees: 14 sizes in four
  shapes each. Project-then-embed and `to_fix`-then-`from_fix` both rebuild
  an equal tree with the same shape and measure.
- A compile-only probe folds, round-trips and converts trees whose size is a
  run-time argument, which only compiles if no type varies with spine depth.
- Two compile-only translation units check that neither library's umbrella
  header reaches the other library or the adapter.

- Clang 22.1.8 with GCC 16's standard library, C++26, both sanitizers: the
  hook, adapter and conformance tests pass with the same counts, and the
  compile probe and the example build without diagnostics. These were built
  by hand: the repository's Clang toolchain files select libc++, which has
  no `std::inplace_vector`, so that configuration skips fingertree and the
  adapter with it.
- Installed packages: all four downstream consumers pass. The new one links
  only `beman::typeclass_tree_adapters` and includes headers from all four
  components, so it checks that the package config finds both libraries and
  that they find beman.transpose.
- GCC 16 with `BEMAN_TYPECLASS_USE_MODULES=ON`, Debug: **724/724 tests**. The
  adapter is left out of that configuration as described above; the access
  hook's 12 cases run there.
- Changed C++ files pass clang-format 22.1.8; changed CMake files pass
  gersemi.

Not validated here: MSVC, and C++23 builds, which do not build fingertree.
The stage touches no header either of those configurations compiles.

## Remaining scope

Stage 6 adds the cross-component matrix, promotes the review probes into the
owning tests, and updates the papers and the public support matrix.
