# Tree/typeclass coherence review — 2026-11-08

Reviewed `main` at `49c1b4b`, in the separate `tree-coherence-review`
worktree. Scope: transpose's current primitive contracts, the tree algorithm
headers and adapters, fingertree and its four capability wrappers, and the
binary-tree evidence in `examples/transpose`.

The structural recursion core is consistent with its explicit/lookup split.
The consumer interfaces are not yet coherent with the current transpose
contracts, and fingertree does not currently supply the structural tree
adapters. There are also reproducible correctness defects in monoid-state
propagation and binary-tree effect composition.

This directory contains review evidence, with no production API changes.
Several gaps were identified in the October reviews; the findings below were
checked against the current headers rather than assumed from those reports.

## Verification

- GCC 16.2.1, C++26: build succeeded; **532/532 tests passed**; all **53**
  standalone interface-header compilation steps passed. Benchmarks and the
  opt-in P2300 evidence were disabled.
- GCC and Clang 21, using GCC 16's standard library, agreed on the initial
  registration/concept matrix and the measure/allocator erasure probe.
- GCC 16, C++23 with fingertree disabled: **412/412 tests passed**,
  checking the lower language floor of transpose and tree_algorithms.
- Standalone probes exercise a native collecting Applicative, a collecting
  Applicative whose `pure` returns a different carrier, noncommutative
  measures, a logging Applicative, stateful monoids with unequal allocators,
  callable ref-qualifiers, and rejected-call detection.
- A `FingerTree<int>` traversal with `expected<void, E>` compiles and produces
  `expected<FingerTree<monostate>, E>`. A nonempty error-set grade also works.
  An effect with the uniform empty grade, `expected<int, error_set<>>`, is
  softly rejected, as transpose's existing probe harness deliberately
  requires for vector traversal too: re-indexing at the empty grade gives a
  bare value, so that uniform carrier fails the Applicative subsumption
  requirement. The new vacuous `recover` rule does not extend that carrier's
  traversal support.

The baseline suite's success does not cover the failures below. In particular,
the fingertree Traversable tests mostly exercise `optional` and the default
count tree; their negative probes check invalid callables and non-context
results, but not a valid policy requiring `collect`, reconstruction limits,
or the deep object concepts.

## Findings

### 1. High: all five fingertree traversals accept policies they cannot implement

Locations: `finger_tree_traversable.hpp:41–58`, and the Traversable Impls in
`finger_tree_random_access.hpp`, `finger_tree_rope.hpp`,
`finger_tree_priority_queue.hpp`, and `finger_tree_interval_index.hpp`.
Reference contract: `transpose/traverse.hpp:331–337` and
`transpose/sequence.hpp:309–342`.

`applicative_object_for` accepts either the type-stable pairwise route or a
native `collect(vector<Effect>)`. The vector instance implements both routes,
preferring `collect`. Every fingertree instance uses the same concept but
unconditionally executes `pure` plus an assignment-based pairwise fold.

The probe's policy passes `applicative_object_for<App, labelled<int>>`:
`pure` returns `boxed<T>`, `invoke` returns `labelled<T>`, and `collect` returns
`labelled<vector<T>>`. Vector traversal succeeds. Each of the five fingertree
traversals fails in its body when assigning a `labelled<vector<...>>` to a
`boxed<vector<...>>` accumulator. This is also a hard error during availability
detection.

With a type-stable policy that offers both routes, the observed collect counts
are vector **1**, base fingertree **0**, and all four wrappers **0**.

Repair: implement the native route across all five consumers, using the
existing probe and preserving their reconstruction rules. The interval index
must retain endpoints while traversing payloads. A shared sequencing helper or
delegation to the vector instance would reduce the five copies of this logic.
The generic probe reproduces the carrier issue without an execution
dependency; actual P2300 senders were not exercised in this review.

### 2. High: allocator-mismatch append resets the stored monoid

Location: `finger_tree.hpp:1184–1186`.

The rebuild calls `from_sequence(right.flatten(), d_alloc)` and omits
`d_monoid`. Two trees built with the **same** explicit modular monoid and
different PMR resources therefore combine data measured under different
instances. No incompatible-input assumption is needed to trigger this.

For a modular-sum instance whose default modulus is 100, both operands use
modulus 5. Appending `[1]` to `[4,4]` produces:

```text
expected modulus = 5, actual = 100
expected measure = 4, actual = 9
```

The single-node case in `app3` propagates the rebuilt right tree's instance.
With a zero-initialized default modulus, the same omission can reach a
remainder by zero instead of a visibly wrong value.

Repair: pass the stored monoid to the allocator rebuild. Add a regression
combining explicit state, unequal allocators, and the single-node fast path.
The current stateful-monoid test exercises cons/snoc/reversal, while the PMR
tests use the stateless default monoid; their combination is missing.

### 3. High, evidence code: binary-tree traversal duplicates effects at unary nodes

Location: `examples/transpose/binary_tree.hpp:325–355`.

`empty_child_like` maps an existing child context to a null pointer. The final
composition then uses that derived context and the real child's context.
A logging Applicative records the child's log twice; a deferred computation
would likewise be composed twice. Calling the element function once is not
sufficient to ensure its returned effect occurs once.

The same example composes root, left, right, while its Foldable result order
is left, root, right. The logging probe produces:

```text
full tree:  fold_map = 123, traverse effects = 213
unary tree: fold_map = 12,  traverse effects = 122
```

Repair: represent an absent child by an effect-free value, and compose the
present contexts once each in the agreed traversal order. This is the
example's implementation defect, not a defect in `derive_applicative`.
The two binary-tree implementations remain different types; fixing this
example alone would not add Traversable support to the library tree adapter.

### 4. Medium: traversal erases the measured tree's configuration

Location: `finger_tree_traversable.hpp:61–65`; corresponding reconstruction
expressions in all four wrappers.

The base implementation always rebuilds `FingerTree<U>`. It discards the
input's monoid type and state, measure policy, allocator type, and allocator
instance. The opening comment claims measure semantics are preserved, which
the implementation contradicts.

An identity-valued optional traversal over an integer-weighted `[2,3,7]` tree
changes its measure from **12** to **3** and changes its concrete tree type.
The same traversal over a PMR tree returns a tree with `std::allocator`.
This occurs even when `U` is the original element type. The wrappers similarly
reset their backing `Tree` parameter to its default.

Repair needs a reconstruction/rebinding policy. Same-element-type traversal
can preserve the existing configuration and recompute changed measures. A
type-changing traversal needs an explicit rule for obtaining a compatible
measure policy and allocator; an arbitrary `MEASURE_POLICY<T>` cannot simply
be assumed to measure `U`. This is an interface decision, not a spelling fix.
Preserving a sequence's element order does not by itself preserve this
configuration.

### 5. Medium: all five structural Functor Impls use the wrong callable category and fail hard in probes

Locations: `BinaryTreeFFunctorImpl`, `FringeTreeFFunctorImpl`,
`RoseFFunctorImpl`, `ExprFFunctorImpl`, and `NatFFunctorImpl`.
Representative location: `binary_tree.hpp:181–187`.

Each computes `invoke_result_t<Fn, const A&>` but invokes the named callable
as the lvalue `fn`. A temporary callable with an `&`-qualified call operator
is rejected although its actual invocation would work. An `&&`-only callable
gets past that result computation and fails at the invocation instead.

All five also deduce the return type from an unconstrained body. A
`requires`-expression probing `fmap(NotCallable{}, layer)` diagnoses from
inside that body rather than yielding false. These are precisely the
callable-category and rejected-call problems transpose's vector Functor and
Traversable instances have already addressed.

Repair: constrain the actual `Fn&` invocation and holdable result, and compute
the result in that category. Retain constexpr coverage and left-before-right
child mapping. No recursion-scheme redesign is required.

### 6. Medium: element metadata does not agree with the deep object concepts

`transpose/functor.hpp:256` and `transpose/traverse.hpp` extract an element
through `applicative_value_t<Structure>`, whose primary protocol is the
structure's nested `value_type`. An Impl's `element_type` does not supply that
trait automatically.

- All five structural layer Functors are registered and callable, but fail
  `functor_object<Map, Layer>`. None supplies the required element trait.
- RandomAccess, PriorityQueue, Rope, and IntervalIndex register Traversable
  objects but all fail `traversable_object<Map, Structure>`.
- RandomAccess, PriorityQueue, and IntervalIndex also fail `foldable_object`.
  Rope happens to pass that concept: the alternative asking for default
  `combine_all`/`fold` succeeds on its string elements. This does not repair
  the absent structure element trait.
- The base `FingerTree<T>` passes both Foldable and Traversable object
  concepts for the tested copyable element type.

Repair: agree on the element protocol, then test the actual registered
objects against the deep concepts. For a structural layer, the mapped element
is recursive position **A**, not the payload **T**. Variant-based layers
need trait specializations or another explicit protocol, rather than a
nested alias on `std::variant`.

### 7. Medium: Rope's registered traversal cannot support arbitrary effect values

Location: `finger_tree_rope.hpp:185–205`.

The constraint admits a function returning `optional<int>`, collects
`vector<int>`, then passes it to `FingerTreeRope<>::from_chunks`, which only
accepts `vector<string>`. The error comes from the body, including inside a
`requires` probe. The newer, common `expected<void, E>` effect also fails:
the unit payload becomes `monostate`, which cannot be rebuilt as a rope chunk.

A function returning `optional<string>` works, explaining why the existing
wrapper probe passes. Simply adding `value_type = string` would address the
metadata issue but leave this reconstruction limitation in place.

Repair: decide whether Rope offers a restricted same-chunk-type traversal or
a general shape whose result can hold another element type, and state the
restriction in its declaration. Its current blanket registration promises
more than its result constructor can implement.

### 8. Medium: fingertree's Foldable bases omit transpose's monoid constraint

Location: `finger_tree_foldable.hpp:25–33` and the Foldable Impls in all four
wrappers. Contrast `transpose/sequence.hpp:127–145`.

The trailing result types reject an uncallable mapper, but the supplied result
monoid is unconstrained. Probing `transpose::fold_map(f, tree, BadMonoid{})`
reaches the body through `derive_foldable`'s deduced return type and errors on
missing `identity` and `combine`, rather than answering false.

Repair: apply `monoid_object<ResultMonoid, Result>` to each primitive, as the
vector instance does. Keep the result monoid distinct from the tree's measure
monoid; that distinction is already correct in the implementation.

### 9. Medium: traversal still requires copies at both ends

Locations: `finger_tree_traversable.hpp:50,63`, `finger_tree.hpp:1164–1173`.

The pairwise composition forwards its accumulator and effects as rvalues,
which agrees with transpose's updated Applicative handling. But `flatten()`
copies the input elements, and `from_sequence(const vector<T>&)` copies each
result into its leaf even when called with `std::move(values)`.

Both probes fail to compile:

- An integer tree mapped to `optional<unique_ptr<int>>`.
- A tree constructed by `snoc(unique_ptr<int>)`, mapped through
  `const unique_ptr<int>&` to `optional<int>`.

The second operation needs only immutable access to shared input elements;
it does not require consuming them. Persistence correctly explains the lack
of `consumes_rvalue_structure`, but does not require this input flattening
copy. The first operation creates fresh result values that could be moved.

Repair: walk the input by reference, and provide a construction path that
can consume freshly collected results. This is separate from allowing
consumption of a persistent input structure.

## Interface coverage

These entries describe shipped registrations/adapters, not interfaces a
caller could write independently.

| Representation | Structural tree algorithms | Functor registration | Foldable registration | Traversable registration |
|---|---|---|---|---|
| BinaryTree | Projection; Fix conversions | Layer only | No | No |
| FringeTree | Projection and embedding | Layer only | No | No |
| RoseTreeFix | Fix-native | Layer only | No | No |
| Expr / Nat | Fix-native | Layer only | No | No |
| FingerTree | No supplied layer/projection/embedding | No | Yes | Yes |
| Four fingertree wrappers | No supplied structural adapter | No | Yes | Yes, with the limitations above |
| Transpose's example BinaryTree | Separate example type | No | Yes | Yes, with finding 3 |

Thus the requested property that fingertree supports all tree algorithm and
tree typeclass interfaces is **not currently met**. `transpose::fold_map`
works over its registered Foldable; `tree_algorithms::fold_map` has no
registered projection/layer fold for it. The latter's unconstrained lookup
overload is nameable, but a call fails its missing-instance assertion.

`fold_fix` and `unfold_fix` deliberately operate on `Fix<F>`; supporting a
different representation means supplying an appropriate layer/conversion or
using the direct `fold_with`/`unfold_with` route. An optional integration
adapter can provide this without making Papers C and D depend on each other.

The two `fold_map` call shapes also remain separate:

```cpp
transpose::fold_map(f, structure, result_monoid);
tree_algorithms::fold_map<Result>(f, combine, identity, tree);
```

This is intentional in the frozen tree-algorithm decisions: its explicit
tier takes plain callables and values without including transpose. It does
not prevent the lookup tier from offering a monoid-object bridge and
registering Foldable objects that delegate to the existing structural fold.
Those bridges are currently absent. A new independent implementation of the
derived folds would make the split worse.

The library trees' Functor registrations map recursive positions; they do
not map stored payloads. A general tree Traversable cannot be obtained from
that `fmap` alone: sequencing a layer's child contexts and rebinding its
payload are additional information. The paper's statement that fixed-point
tree traversal is derivable through the layer mapping function should be
qualified accordingly (`papers/algorithms-for-trees.md:413`). This does not
call for making Foldable a Traversable superclass.

No Applicative instance for every tree/container is required by the current
design. The concrete defect is that existing Traversable instances do not
support every Applicative policy their declarations admit. Likewise, the
absence of a canonical `Monoid<FingerTree<...>>` is an additional registration
gap, not evidence that its existing passed measure/result monoids are the
wrong abstraction; stateful configurations need an explicit combination rule.

## Checks that agree

- The explicit algorithm tier remains independent of transpose. The
  configure-time isolation checks pass.
- Lookup uses the one `transpose::functor<Layer>` registry and its CRTP base;
  there is no copied Functor machinery.
- `fold_fix`, `unfold_fix`, `refold`, `fold_with`, and `unfold_with` preserve
  their structural relationships. Existing tests pin lookup/explicit and
  direct/Fix equivalence, including constexpr evaluation.
- Elementwise result order is binary in-order, fringe left-to-right, and
  rose pre-order; string concatenation distinguishes these orders.
- Fingertree's native Foldable walks leaves left-to-right, passes a distinct
  result monoid, and its ordinary optional traversal preserves element order
  and count. Its refusal to consume shared input values is appropriate.
- A 100-element tree measured under string concatenation satisfies
  `fold_map(measure_policy, tree, tree.get_monoid()) == tree.measure()`.
  Its dual-monoid reversal and split/append reconstruction satisfy the same
  measure law. These were run as additional probes; the existing suite does
  not pin that full combination with a noncommutative measure.

One sequencing distinction needs to stay explicit: the structural
`fold_map` combines its results in the advertised order, but runs the mapper
while folding layers bottom-up. The probes return `123` while mapper calls
occur as `132` for the binary tree and `231` for the rose tree. This does
not change the result of a pure mapper. Those implementations cannot serve
as evidence of an effectful traversal's callback order without further work.

## Suggested repair order

1. Preserve the passed monoid in allocator-mismatch append; eliminate the
   duplicate effects in the binary-tree evidence; honor `collect` in all
   fingertree traversals.
2. Align primitive constraints, callable categories, and element metadata;
   add positive deep-concept checks and negative availability probes.
3. Settle measured-tree rebinding and Rope's result shape, including the
   new unit-valued effects; preserve allocators and move fresh result values.
4. Supply optional structural fingertree adapters and Foldable bridges for
   the library trees. Pin their agreement with existing folds using a
   noncommutative result monoid and an order-sensitive Applicative.

## Reproducing the focused probes

The sources are in [probes/](probes/). From the worktree root, after configuring
`build/review` with GCC 16 and C++26:

```sh
g++-16 -std=c++26 -I. -Iinclude -Ibuild/review/include \
  docs/coordination/review/2026-11-08-coherence/probes/coherence.cpp \
  -o build/review/coherence
build/review/coherence
```

The same compile/run command applies to `native.cpp`, `monoid-state.cpp`,
`binary-effects.cpp`, and `measure-laws.cpp`, changing the source and output
names. The first four print the observed failures; `measure-laws` asserts
the successful laws.

For `native.cpp`, compiling with `-DWIDEN -DSTRUCTURE=N -fsyntax-only` must
currently fail for each `N` from 0 through 4 (base tree, rope, random access,
priority queue, interval index). Without `WIDEN`, it runs and shows the
collect counts.

`rejected-calls.cpp` uses `-DPROBE=N -DLAYER=L -fsyntax-only`:

| PROBE | Intended contract | Current result |
|---|---|---|
| 0 | Registered layer satisfies `functor_object` | Assertion fails |
| 1 | Uncallable mapper is a soft rejection | Body error |
| 2 | Temporary `&`-qualified mapper is usable | Body error |
| 3 | `&&`-only mapper is a soft rejection | Body error |
| 4 | Invalid result monoid is a soft rejection | Body error |
| 5 | Rope traversal with an integer-valued effect | Body error |
| 6 | Base-tree traversal with a move-only result | Copy error |
| 7 | Base-tree traversal over move-only input by const reference | Copy error |
| 8 | Base-tree traversal with a void-valued effect | Compiles |
| 9 | Rope traversal with a void-valued effect | Body error |
| 10 | Uniform empty-grade effect is softly refused, matching transpose | Compiles (negative assertion) |
| 11 | Base-tree traversal with a nonempty error-set grade | Compiles |

For probes 0–3, run every `L` from 0 through 4: binary, fringe, rose,
expression, and natural-number layers. All twenty cases were compiled.
