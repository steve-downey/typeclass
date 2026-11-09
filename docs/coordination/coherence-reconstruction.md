# Configured fingertree reconstruction

Stage 3 of the [coherence plan](coherence-plan.md), stacked on conformance
PR #46. The implementation follows the [recorded contracts](coherence-contracts.md)
and reuses the existing conformance workspace and Debug build directory on
branch `tree-coherence-reconstruction`.

## Behavior

`finger_tree_rebind` supplies a result type and an owned builder. Same-payload
mapping retains the concrete representation, allocator, and monoid; the
UnitMeasure rebind retains configuration across payload type changes. Default
priority and interval backing families declare their own rebinds. Unsupported
and malformed declarations reject through constraints.

Payload Functors and all four generic Traversables share this builder. The
three payload-polymorphic capability wrappers expose their configured backing
tree and a const-reference payload walk. Traversal materializes pointers to
persistent payloads and delegates sequencing to transpose's vector primitive;
no persistent payload is copied. The final reconstruction closure owns the
builder, including interval endpoints. Fresh payloads move through a new
rvalue-vector factory, which measures each owned value before moving it into
a leaf. Rope retains the explicit chunk-vector route adopted in stage 2.

Assignment also carries the incoming monoid with its representation. The
previous paths could retain cached measures from one monoid while storing
another; an unequal-resource rebuild now uses the destination allocator and
the incoming monoid. The regression covers copy and move assignment with
both shared and distinct PMR resources.

Default count-measured, default-allocated trees acquire a canonical
concatenation Monoid using persistent append. Always-equal allocator paths
avoid instantiating a copy-only rebuild, so move-only payloads work there.
The explicit configured concatenation instance rebuilds both operands under
its selected allocator and monoid, with an O(n + m), copyable-payload contract.
Noncommutative law tests cover identities, associativity, measure homomorphism,
and dual reversal within the selected measure-law domain.

## Primitive dispatch correction

The negative rebind tests exposed a gap in transpose's operation objects:
`traverse` checked its inferred Applicative but not the call through the
selected Traversable; `transpose` similarly stopped at element metadata.
Both now constrain their delegated calls, enforcing the existing
[operation-object decision](../transpose/decisions.md#operation-object-wording).
An independent restricted-result fixture in transpose tests soft refusal and
constexpr success. No deep concept or Foldable superclass change was needed.

P3200 wording was regenerated from the annotated headers. The installed
specgen is 0.15.0; the earlier generation used 0.6.0, so the generated refresh
also includes renderer changes in other fragments. No fragment was edited by
hand. The generation record identifies the tool and input/output hashes.

## Validation

- GCC 16 C++26 Debug: full build, 601/601 tests, examples, and all 56
  standalone public-header checks passed without compiler diagnostics.
- GCC 16 and Clang 21 C++23: transpose's operation-object suite passed
  (27 cases, 94 assertions each), including constexpr dispatch and soft
  refusal of unsupported result families.
- Clang 21 C++26 with GCC 16's standard library: reconstruction (26 cases,
  212 assertions), concatenation monoids (4 cases, 41 assertions), existing
  conformance (19 cases, 208 assertions), and traversal (16 cases,
  279 assertions) passed without compiler diagnostics.
- Both public support-guide examples compile independently with GCC and
  Clang. All three installed-package consumers pass; the fingertree consumer
  exercises configured PMR mapping/traversal and canonical/explicit monoids
  using only installed headers and exported targets.
- Changed C++ files pass clang-format-21. Wording input/output hashes and
  diff whitespace checks pass. Wording was regenerated with specgen 0.15.0.

Benchmarks, opt-in P2300 evidence, and module configurations were not exercised.

## Remaining scope

Stages 4–6 retain their gates: payload adapters for the library trees,
optional structural fingertree integration, and the integrated packaging and
context matrix. This phase adds no dependency between tree_algorithms and
fingertree. See the [public support guide](../fingertree/typeclass-support.md)
for current APIs and Rope migration.
