# Bringing transpose, tree algorithms, and fingertree into coherence

Proposed implementation plan, 2026-11-08. Starting point: `49c1b4b` and the
[coherence review](review/2026-11-08-coherence/README.md). The recommendations
below are the original recommendations. The implementation contracts now
adopted for execution are recorded in [coherence-contracts.md](coherence-contracts.md);
that note is not a record of additional historical rulings from Steve.

The outcome is one set of primitive contracts, with adapters that satisfy
them and explicit boundaries where an interface does not apply. Repair the
observed correctness failures first. Then complete the typeclass and
structural adapters without changing the independence of the three proposals.

Stage 1 is complete on `tree-coherence-correctness`; it does
not depend on the proposed API decisions. See the
[correctness implementation note](coherence-correctness.md) for changes,
validation, and remaining scope. Stage 0 is recorded in the
[contract note](coherence-contracts.md). Stage 2 primitive constraints and
object conformance are complete on `tree-coherence-conformance`; see the
[conformance implementation note](coherence-conformance.md). Rope's explicit
chunk-traversal migration is included in stage 2. Stages 3–6 remain outstanding.

## Target contracts

- Transpose remains the only home of typeclass lookup, CRTP derivations,
  Applicative composition, Traversable dispatch, and monoid vocabulary.
- The explicit tree algorithms continue to take plain callables. Their
  headers do not acquire a dependency on transpose or fingertree.
- Tree algorithms and fingertree each depend on transpose. An optional
  integration component depends on both; neither library depends on the other.
- Payload mapping and recursive-position mapping remain distinct. For
  `BinaryTreeF<T, A>`, the structural Functor maps **A**; a Functor for
  `BinaryTree<T>` maps **T**.
- Binary payload traversal is in-order, fringe and fingertree traversal is
  left-to-right, and rose traversal is pre-order. Each returned effect is
  composed once. A fold's monoidal result order does not by itself specify
  when an ordinary mapper's side effects execute.
- Persistent inputs present elements by const reference, including when
  handed over as rvalues. Newly created result values can be moved into
  their destination. Reconstruction owns any configuration or shape metadata
  needed by a deferred effect after the traversal call returns.
- Every declaration admits the call its body actually makes. Invalid
  callables, result types, monoids, and reconstruction requests fail through
  constraints, including when reached through an operation object.
- Traversable does not gain a Foldable superclass. Existing empty-grade
  traversal refusals remain as specified by transpose; vacuous recovery at
  the empty grade does not make bare values into Applicative contexts.

## Recommended interface choices

Record these before implementing the stages that rely on them. The existing
decisions requiring amendments are in
[tree_algorithms/DECISIONS.md](../tree_algorithms/DECISIONS.md); the
[extraction compatibility checklist](../tree_algorithms/notes/extraction-plan.md#7-work-item-f--fixpoint-compatibility-checklist)
still applies.

**Use the current element protocol.** Add `value_type` to concrete fingertree
wrappers. Supply transpose's existing element trait for structural layers,
using the recursive-position type, and for the supported fixed-point payload
trees in their consumer headers. Variant layers need trait specializations.
Keep these registrations outside the mechanism-free `Fix` header. Introduce
no new global lookup mechanism to work around missing metadata.

**Make measured-tree reconstruction explicit.** Use one fingertree-local
reconstruction/rebinding customization shared by Functor and Traversable.
Its contract is: given a source tree and a new payload type, identify the
result type and obtain a builder carrying the required configuration.
The builder consumes fresh values; it must be captured by value when used
inside a deferred context. Proposed consumer-side vocabulary, in a new
`finger_tree_rebind.hpp` header:

```cpp
template<class SourceTree, class U>
struct finger_tree_rebind;

template<class SourceTree, class U>
using finger_tree_rebind_t = typename finger_tree_rebind<SourceTree, U>::type;
```

A specialization supplies `type` and `make_builder(const SourceTree&)`.
The returned owned callable accepts fresh `vector<U>&&` and returns `type`.
Unsupported specializations remain undefined and are detected before a
traversal body is instantiated. This is a representation-rebinding trait,
not a second typeclass registry. Its spelling is subject to the stage 0
declaration review. Proposed defaults:

| Mapping | Reconstruction rule |
|---|---|
| Same payload type | Preserve concrete tree type, allocator instance, and monoid instance; recompute measures from the new values |
| `UnitMeasure<T, Tag>` to payload `U` | Rebind to `UnitMeasure<U, Tag>`; retain the monoid and allocator instances |
| Other measure policies with a declared rebind | Use that rule, including its rule for transferring configuration |
| Other type-changing requests | Reject through constraints; do not silently choose the count measure or default allocator |
| Capability wrappers | Rebind their documented backing-tree family and metadata through the same construction path |

Keep the existing byte allocator type and instance when they support the
result's internal node rebinds. A type-dependent wrapper monoid, such as the
priority queue's product measure, needs its own declared rebinding rule.
Do not infer a new measure law merely because a callable accepts `U`.

**Give Rope an honest boundary.** Keep its Foldable registration over string
chunks. Replace the blanket generic Traversable registration with an explicit
chunk traversal route: generic transformations can traverse `rope.chunks()`;
string-valued results can be rebuilt explicitly with `from_chunks` inside
the context. A named same-chunk-type convenience operation may delegate to
that route. This is a source compatibility change to document and migrate.
Adding `value_type = string` alone cannot make a string-only rope hold units
or arbitrary mapped values. A polymorphic chunk container would be a separate
representation, not an implicit fallback result for Rope traversal.

**Limit generic fixed-point registrations to representations with the
required information.** Provide payload Functor/Foldable/Traversable for
binary, fringe, and rose trees, including their supported Fix forms. Expr
can supply Foldable over its integer constants. Keep the current closed Expr
and Nat as structural fixtures rather than claiming arbitrary payload
Functor/Traversable support. Generalizing expression literals would be a
separate API extension. There is no blanket `Traversable<Fix<F>>` derivation
from structural `fmap` alone.

**Keep canonical monoids narrow.** Add a concatenation Monoid registration
for the default count-measured `FingerTree<T>` once its laws are pinned.
Custom measured or allocated trees use an explicit concatenation instance
with a documented configuration domain. A default-constructed monoid must
not choose a modulus or resource on behalf of a stateful tree. Specify the
compatibility requirement for concatenating differently configured operands;
sharing cached nodes requires a common measure law and compatible state.

## Implementation stages

Each numbered implementation stage should be a reviewable change with its
own branch, worktree, and build directory. Documentation and the corresponding
tests travel with the implementation. The existing review worktree holds
this plan and its evidence.

### 0. Record the contracts and the applicability matrix

Write the decision amendments for element metadata, reconstruction, Rope,
and optional integration ownership. Enumerate which representations support
payload algorithms and which are structural fixtures. Record the agreed
result/effect order and allocator/monoid compatibility rules.

Keep the current explicit algorithm signatures and monoid selection rules.
Mark evidence separately from proposed wording. Inspect affected uses in the
fixpoint and trees evidence repositories before changing shared vocabulary;
record intentional divergence rather than backporting extracted code.

Done when each later API change has a concrete proposed declaration and
result-type example, including a custom weighted tree, a PMR tree, an
interval index, and Rope. This stage does not block the direct bug repairs.

### 1. Repair correctness under the existing interfaces

Affected files: `finger_tree.hpp`, the five fingertree Traversable Impls,
and `examples/transpose/binary_tree.hpp`.

- Pass the stored monoid to the allocator-mismatch append rebuild.
- Replace the binary example's effectful absent-child placeholder with a
  pure absent value; compose existing child contexts once in in-order.
- Honor native `collect` in all applicable runtime-sized traversals,
  retaining the pairwise path for policies without it. Factor the common
  sequencing work only; wrapper reconstruction still owns its metadata.
  The binary example needs a whole-tree collection/reconstruction path for
  policies whose context type cannot support its recursive runtime branches.
- Preserve each element function's actual const-reference argument category.
  When delegating to vector traversal, do not accidentally opt a persistent
  input into vector's consuming-rvalue convention.

Gate: unequal-resource append retains modulus 5 and measure 4 in the
review's example; logging traversal records `123` and `12`, with no duplicate
effect. Every applicable traversal uses one native collection when offered
and works with a policy whose `pure` and composition return different
carriers. An interval index retains every endpoint. A delayed execution
probe outlives the input object and still rebuilds correctly.

### 2. Align primitive constraints and object conformance

Affected files: the five structural Functor Impls, the fingertree Foldable
Impls, wrapper declarations, and the consumer-side element traits.

- Compute repeated callable invocation results from `Fn&`, constrain that
  invocation, and reject unholdable results before instantiating a body.
- Constrain each Foldable primitive with `monoid_object` over its mapped
  result type. Keep the result monoid distinct from the tree's measure monoid.
- Supply the agreed element metadata. Ensure defaulted Foldable operations
  consult the actual element type rather than succeeding accidentally through
  an unrelated alternative in a deep concept.
- Add positive `functor_object`, `foldable_object`, and
  `traversable_object` checks for every advertised instance. Rope is checked
  against its revised supported surface.
- Use representative copyable payloads for full-object probes and test
  move-only support operation by operation. A persistent Foldable can fold
  references to move-only values without being able to copy them into a
  vector. If a deep concept demands a copy outside an operation's contract,
  correct that requirement once in transpose rather than forcing a consumer
  to manufacture exclusive ownership.
- Constrain new convenience bridges on the calls they make. Fix stale
  missing-instance diagnostics without weakening the explicit tier.

Gate: each layer accepts a temporary lvalue-callable object, softly rejects
an rvalue-only callable and an uncallable object, and retains constexpr
coverage. Wrong monoids and invalid result types are soft rejections through
both the instance and operation-object entry points. All advertised deep
concept checks pass on GCC and Clang.

### 3. Preserve reconstruction state and move fresh results

Implement the reconstruction contract in fingertree, independently of
tree_algorithms. Add a construction path that consumes fresh values while
measuring them before moving them into leaves. Use the existing reference
walk to avoid copying persistent input elements into a temporary vector.

Functor, Traversable, and capability-wrapper reconstruction use this same
builder contract. The interval index carries endpoints separately from
payload effects. Wrapper construction must accept the configured backing
tree instead of always invoking its default factory. Maintain Rope's explicit
chunk-traversal route adopted in stage 2.

Add the default concatenation Monoid and explicit configurable concatenation
instance here. Recompute cached measures under the chosen result instance;
sharing across incompatible state is outside its algebra's domain unless
the operation explicitly rebuilds under a selected instance.

Gate: identity-valued traversal preserves the weighted tree's measure 12,
its concrete type and passed monoid state, and a PMR tree's resource. Default
type-changing traversal uses the declared rebound family. Unsupported custom
rebinds reject softly. Both move-only review examples work. Successful unit
effects preserve one unit per payload position on each applicable structure.
Noncommutative concatenation satisfies identity and associativity within its
configuration domain; the measure-homomorphism and dual-reversal laws hold.

### 4. Complete payload typeclass adapters for the library trees

Add consumer headers for the supported binary, fringe, and rose
representations and their Fix forms. Supply Functor, Foldable, and Traversable
registrations with the agreed metadata and payload rebinding. Supply Expr's
constant Foldable without advertising a general payload constructor for the
current closed Expr or Nat.

- A monoid-object convenience bridge in the lookup tier delegates to the
  existing `tree_algorithms::fold_map` implementation. Foldable registrations
  use that bridge, obtaining derived operations from transpose.
- Preserve the mapper and monoid objects passed by the caller, including
  their state. State any copyability requirements of existing projections
  in their declarations; a read-only adapter must not fail unexpectedly
  inside a payload copy.
- Whole-tree traversal sequences payload effects in contractual order and
  rebuilds the representation with its declared shape. An owned shape
  descriptor plus collected effects is a suitable implementation for
  runtime-sized trees and non-type-stable contexts.
- Implement concrete adapters before abstracting their common work. Generic
  structural Functor mapping alone does not sequence a layer's child effects
  or rebind its stored payload.
- Replace duplicated example-local adapters with the shared implementation
  where they represent the same type; otherwise keep the two binary ports
  explicitly distinct and test their semantic agreement.

Gate: structural and typeclass folds agree for a noncommutative result
monoid; `fmap(identity)` and composition preserve the payload tree; traversal
with a pure lift agrees with mapping; `transpose` agrees with identity
traversal. Binary sparse children and rose child order survive reconstruction.
Derived left/right folds and search operations use the existing transpose
derivations. The explicit-tier isolation check still passes.

### 5. Add an optional structural adapter for fingertree

Prototype the adapter in integration tests, then ship it behind an opt-in
header/target that links both libraries. Proposed packaging:
`beman::typeclass_tree_adapters`, with its own installed component. Neither
existing library target links this target or includes its header through its
ordinary umbrella. Confirm the package spelling with the declaration review
in stage 0.

Expose a narrow, library-neutral structural access hook in fingertree. Build
the adapter's layer view around the real Empty/Single/Deep tree forms and
Leaf/Node2/Node3 element forms, preserving digit order and spine position.
Use finite tagged handles and intermediate reconstruction pieces for the two
recursive sorts; do not introduce a type that changes with spine depth.
Read-only projection must keep handles alive and avoid copying payloads.

Supply the layer Functor, elementwise layer fold, projection, and embedding.
Embedding constructs nodes through the configured builder and recomputes
their measures. A logical head/tail decomposition is useful for sequence
algorithms but does not count as completing this structural adapter.

Exercise all existing structural verbs: direct `fold_with`/`unfold_with`,
`refold`, the corresponding Fix representation with `fold_fix`/`unfold_fix`,
and both explicit and lookup `fold_map`. Structural views describe the
representation being visited; sequence operations retain their existing
freedom to rebalance it. Capability wrappers can delegate structural access
to their backing tree with their payload interpretation made explicit.

Gate: project/embed round trips reconstruct an equivalent structure and
measure; direct, Fix-based, and fused algorithms agree; structural
elementwise folding agrees with fingertree's native Foldable. Cover empty,
single, digit boundaries, Node2/Node3, and several spine levels, with an
order-sensitive algebra, a noncommutative measure, explicit monoid state,
and PMR allocation. The compile probe still has a finite instantiation set.

### 6. Make coherence a maintained property

Promote the review's standalone cases into the owning tests as their defects
are repaired. Replace assertions of missing support with positive contract
checks; retain negative cases for intentionally unsupported calls.

Add a small cross-component matrix covering registered objects, supported
result types, native collection, order, configuration preservation, and the
structural/typeclass equivalence laws. Cover optional, ordinary and graded
expected, unit effects, lanewise contexts, and a generic collecting context
whose composition changes carrier type. Run real P2300 evidence when its
dependency is available; keep the generic collecting cases in the default
suite.

Update affected CMake header sets, umbrella headers, module exports where
supported, installed package dependencies, and downstream install tests.
Keep each new instance registration visible before its first use. Update
the tree paper's traversal claim, its synopsis and anchors if signatures
change, the allocator/measure documentation, and the public support matrix.
Regenerate P3200 wording only if its source headers change; verify the
checked-in wording still matches otherwise.

Gate: C++23 builds of transpose/tree_algorithms; C++26 builds including
fingertree/integration; GCC and Clang conformance; standalone headers; the
existing supported module configurations; and downstream installation tests.
The nine reviewed defects are closed or, for Rope, replaced by an explicit
documented contract with migrated call sites. No supported registration
fails its own deep concept in its advertised domain, and no advertised
cross-library algorithm lacks an adapter.

## Dependencies and review boundaries

Stage 1 can proceed immediately. Stage 2 follows the metadata decision in
stage 0. Stage 3 requires the reconstruction and Rope decisions. Stage 4
uses stages 2 and 3; stage 5 uses the conformed layer machinery and the
configured builder. Stage 6 closes the integrated work, with documentation
and local validation also maintained in every earlier stage.

| Review item | Implementation stage |
|---|---|
| Native collection ignored | 1, maintained through 3–5 |
| Append loses monoid state | 1 |
| Binary example duplicates effects | 1, shared adapter agreement in 4 |
| Measures/allocators erased | 0 and 3 |
| Layer callable category and hard errors | 2 |
| Element metadata and deep concepts disagree | 0 and 2; new adapters in 4 |
| Rope result shape | 0 and 2 |
| Unconstrained result monoids | 2 |
| Input/result copying in traversal | 3 |
| Missing tree Foldable/Traversable/Functor adapters | 4 |
| Missing structural fingertree support | 5 |

Completion is measured against the supported-interface matrix and these
gates, not a larger baseline test count. Applicative instances for every
tree, a universal Traversable for arbitrary `Fix<F>`, and a polymorphic Rope
are not prerequisites for the interfaces above to be coherent.
