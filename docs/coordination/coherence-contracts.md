# Tree coherence implementation contracts

Stage 0 of the [coherence plan](coherence-plan.md), adopted for the requested
implementation pass on 2026-11-08. These are implementation contracts for the
coordinated libraries, not additional proposed wording or historical rulings.
The [review](review/2026-11-08-coherence/README.md) records the motivating cases.

## Elements and applicability

Use transpose's existing `applicative_value<T>` trait and nested `value_type`
protocol. Structural layer registrations specialize that trait in their
consumer headers: the element is the recursive position `A`, not payload
`T`. Variant aliases need explicit specializations. Concrete fingertree
wrappers supply `value_type` for their payload (string chunks for Rope).
No metadata or registration is added to the mechanism-free `Fix` header.
Supported fixed-point payload registrations will supply their traits in their
consumer headers, alongside the corresponding instances.

The table describes the completed implementation target. An entry naming a
later stage does not advertise an instance before that stage implements it.
`layer_fold` is the existing structural algebra hook, distinct from Foldable.

| Representation | Functor element | Foldable element | Traversable element | Structural algorithms |
|---|---|---|---|---|
| BinaryTreeF, FringeTreeF, RoseF | child handle A | layer_fold only | none claimed | existing layer adapters |
| ExprF, NatF | child handle A | layer_fold where registered | none claimed | existing layer adapters |
| BinaryTree, BinaryTreeFix | payload T (stage 4) | payload T (stage 4) | payload T (stage 4) | existing adapters |
| FringeTree | payload T (stage 4) | payload T (stage 4) | payload T (stage 4) | existing direct adapter; no new Fix alias promised |
| RoseTreeFix | payload T (stage 4) | payload T (stage 4) | payload T (stage 4) | existing Fix adapter |
| Expr | none claimed | integer constants (stage 4) | none claimed | existing Fix adapter |
| Nat | none claimed | none claimed | none claimed | existing Fix adapter |
| FingerTree | payload T (stage 3) | payload T | payload T | optional adapter (stage 5) |
| RandomAccess, PriorityQueue, IntervalIndex | payload (stage 3) | payload | payload | backing representation through optional adapter |
| Rope | no arbitrary payload mapping | string chunks | use chunks() explicitly | backing representation through optional adapter |

Layer Functors invoke a named callable as an lvalue, even when the caller
passes a temporary callable. Results must be holdable and constructible in
the result layer; payload copies must be admitted by the declaration.
Fingertree Foldable primitives require `monoid_object` for the mapped result,
independently of the measure monoid. Full object concepts are checked with
copyable representative elements. Move-only support is checked per operation:
a const-reference fold does not require copying elements into a vector.

## Order, ownership, and configuration domains

Binary payload traversal is in-order, fringe and fingertree traversal is
left-to-right, and rose traversal is pre-order. Each value effect is composed
once. The monoidal result order of fold_map does not promise the same temporal
order for side effects in an ordinary mapper. Persistent sources always
present elements by const reference, including rvalue handles.

A reconstruction closure owns its shape, endpoints, allocator and monoid
snapshots as needed; it cannot refer to a destroyed source or mapper. The
caller still owns the lifetime of an external PMR resource, as for the source
container. Fresh result values may be moved; shared source values may not.
Cached measures are valid only under the monoid/measure configuration that
produced them. Sharing operands requires the same effective measure law and
compatible configuration. An allocator-mismatch append rebuilds under the
left operand's allocator and monoid, as repaired in stage 1. This does not
license sharing nodes measured under an incompatible monoid state.

## Reconstruction declarations for stage 3

The fingertree-local customization is declared in `finger_tree_rebind.hpp`:

```cpp
template<class SourceTree, class U>
struct finger_tree_rebind;  // unsupported primary remains undefined

template<class SourceTree, class U>
using finger_tree_rebind_t = typename finger_tree_rebind<SourceTree, U>::type;
```

A supported specialization supplies `type` and
`static make_builder(const SourceTree&)`. The returned owned callable accepts
`std::vector<U>&&` and returns exactly `type`. It accepts one fresh value per
source payload, in the source's contractual order. Detection checks both
members and that call before instantiating a mapping/traversal body. Callers
normalize SourceTree's cv/ref qualification. This trait selects a concrete
representation and a configured builder; it is not another typeclass registry.

Same-payload mapping preserves the source concrete type, allocator instance,
and monoid instance and recomputes all measures. UnitMeasure rebinds its
payload while retaining the monoid and byte allocator. Other type-changing
requests need an explicit specialization, including any configuration transfer
it requires. Capability wrappers use the same builder path for their backing
family and own their additional metadata. No arbitrary custom measure silently
falls back to the count measure.

Examples that pin the resulting behavior:

- A weighted `FingerTree<int, Sum, Weight>` rebuilt from `[2,3,7]` remains that
  type and has measure 12; its passed monoid state is retained. Mapping to
  string rejects unless the program supplies a declared rebind.
- For `A = pmr::polymorphic_allocator<byte>`, mapping an int count tree to
  string gives `FingerTree<string, Sum, UnitMeasure<string, size_t>, A>` with
  the original resource and monoid. The tree never chooses a default resource
  on behalf of that input.
- `FingerTreeIntervalIndex<int>` mapped to string yields
  `FingerTreeIntervalIndex<string>`, preserving each start/end pair; a custom
  backing tree must have a compatible declared rebind.
- Rope chunks mapped to lengths yield a contextual `vector<size_t>` through
  `traverse(f, rope.chunks())`. String results may be rebuilt explicitly with
  `FingerTreeRope<>::from_chunks` inside the context. No Rope of integers or
  units is manufactured.

## Rope migration

Remove Rope's generic Traversable registration. Retain its Foldable over
chunks and add `value_type = string`. The replacement is vector chunk
traversal, followed by explicit reconstruction when its results are strings.
This source compatibility change moves from stage 3 to stage 2: the object
conformance gate must not leave a registration advertising an unsupported
result family. Native collection and deferred execution remain covered on
the explicit chunk route. No new same-chunk convenience operation is needed.

## Concatenation and optional structural integration

Stage 3 adds a canonical concatenation Monoid only for default count-measured,
default-allocated FingerTree<T>. Configurable concatenation uses an explicit
`finger_tree_concat_monoid<Tree>` object constructed from a source prototype's
configuration. It exposes `value_type = Tree`, `identity() -> Tree`, and
`combine(Tree, Tree) -> Tree`. Its identity owns the selected configuration;
operands belong to its compatible measure-law domain. Associativity and both
identities are tested with noncommutative measures inside that domain.

Stage 5 prototypes structural access before shipping it in the optional
`beman::typeclass_tree_adapters` target and
`beman/typeclass_tree_adapters/finger_tree.hpp`. This target depends on both
libraries; neither existing library depends on it. The header defines a
finite tagged handle/layer/piece family for both tree and element sorts,
plus projection and configured embedding objects. The native library's access
hook exposes the real Empty/Single/Deep and Leaf/Node2/Node3 forms without
including tree_algorithms. A logical head/tail decomposition does not satisfy
this representation contract. Exact layer alternatives are settled by the
finite-instantiation prototype before becoming public declarations.

## Evidence compatibility

The vocabulary check covered transpose, tree_algorithms and fingertree here,
plus `fixpoint/main/src/smd/concrete/functors.hpp`, its typeclass trait header,
and `trees/main/src/smd/typeclass/typeclass_base.hpp` and tree adapters. All
use the nested-value-type/element-trait distinction; the evidence trees retain
their historical layer metadata and Rope registration. The source repos are
not backported: the divergence is intentional, and extraction remains one-way.
Affected declarations live in the consuming headers listed above; this pass
changes no primitive lookup name, paper wording, org exhibit, or explicit-tier
signature. The extraction plan's section 7 compatibility checklist still
applies to all later adapters.

## Stage 3 declaration details

The three payload-polymorphic capability wrappers expose
`backing_tree() const -> const Tree&`, a public explicit constructor from a
configured backing tree, and `for_each` over const-reference payloads.
These let the builder snapshot configuration and interval endpoints without
copying source payloads. Their Functor and Traversable instances share the
base tree's implementation through these operations. Rope retains its explicit
chunk route and does not acquire these generic instances.

Default priority and interval backing families declare type-changing rebinds
in their wrapper headers. The priority family requires a copyable, totally
ordered result because its measure stores both extrema. Their built-in
monoids and measure policies are stateless; the byte allocator is retained.
Custom backing families need their own declared rebind. A builder must be
move-constructible and invocable as a named lvalue; reconstruction captures it
by value in a mutable closure, so a custom builder need not have a const call
operator.

The explicit configured concatenation instance rebuilds both operands under
its selected allocator and monoid. This costs O(n + m) and requires copyable
payloads, but guarantees that no foreign allocator or cached measure enters
the result. Its algebraic laws apply within the prototype's measure-law
domain. The canonical default-count instance uses native persistent append
and supports move-only payloads with its always-equal allocator.

Vocabulary was checked in all three libraries and both evidence repos.
The new rebind and concatenation names do not exist in the evidence repos;
these consumer-side additions remain an intentional one-way divergence.

The reconstruction refusal gate also exercises transpose's operation-object
contract. `traverse` and `transpose` must constrain the delegated instance call
as required by [operation-object-wording](../transpose/decisions.md#operation-object-wording).
Checking only the inferred Applicative or element metadata is insufficient
when a representation rejects a particular result. This enforces the existing
contract; it adds no lookup mechanism or superclass requirement.

## Stage 4 declaration details

`tree_typeclasses.hpp` is the consumer header for payload Functor, Foldable,
and Traversable on BinaryTree, BinaryTreeFix, FringeTree, and RoseTreeFix,
and for Foldable on Expr's integer constants. The library umbrella includes
it. Fix metadata and registrations live here and recognize only the declared
binary/rose layer families; Fix itself stays mechanism-free. No general
payload construction is inferred for Expr, Nat, or arbitrary Fix families.

The lookup tier adds `fold_map(function, tree, monoid)`, delegating to its
existing explicit-result structural fold. It preserves the named mapper and
passed monoid by reference during the synchronous fold. The existing
projections/layer maps copy payloads, and their carrier folds copy and assign
results: this bridge requires copy-constructible payloads and copyable result
carriers. Payload Functor and Traversable instead read source payloads by
const reference and support fresh move-only results. They own shape metadata
and reuse transpose's vector sequencing, including native collection and
changing carriers. Binary sparse slots, fringe Empty/Leaf/Branch topology,
and rose child order survive reconstruction.

The independently ported transpose example BinaryTree retains its separate
type and Applicative example. Tests pin agreement of payload/effect order
with the library adapter. Expr's example-local length implementation uses
the shared Foldable; its integer-only optional transformation remains an
explicit example operation rather than a generic Traversable registration.

These are consumer-side evidence APIs, consistent with the extraction
compatibility checklist. They change neither structural verb signatures nor
the proposed paper surface. Evidence repositories retain their independent
implementations; extraction remains one-way.
