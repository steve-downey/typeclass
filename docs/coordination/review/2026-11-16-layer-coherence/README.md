# Layer coherence review — 2026-11-16

Reviewed `main` at `4522a1b`. Scope: coherence between the three layers as
they would be read for Standard adoption — transpose's typeclass base and
Applicative interface, the tree algorithms that build on it, and the
fingertree implementation as a consumer of both.

The three layers share one mechanism and one definition of it. They are not
yet coherent enough for adoption: the fingertree cannot be driven by the tree
algorithms at all, the layers disagree on what `fold_map` is, and the papers
lean on vocabulary that no paper proposes.

This follows the [2026-11-08 coherence review](../2026-11-08-coherence/README.md)
and its [plan](../../coherence-plan.md). Stages 0–4 of that plan are in the
code and their repairs were confirmed. Two findings here restate work the plan
already holds open
([fingertree-structural-adapter](#fingertree-structural-adapter) and part of
[proposal-homes](#proposal-homes)); the rest are new.

This directory holds review evidence only. No header, test, or paper was
changed.

## Verification

- GCC 16.0.1 (`r16-8246`), C++26: build succeeded and **712/712 tests
  passed**.
- Clang 22.1.8 was used for the
  [instance-placement](probes/instance-placement.cpp) probe only, to show the
  two compilers disagree.
- Every finding below with a probe was reproduced from this directory by
  [`probes/run.sh`](probes/run.sh); its output is recorded under
  [Reproducing the probes](#reproducing-the-probes).

The baseline suite's success does not cover these findings. No test, example,
or install test includes both `beman/fingertree` and `beman/tree_algorithms`.

## Findings

Findings are in severity order. Each heading is the finding's name; refer to
it by that name.

### fingertree-structural-adapter

**High. Already open:**
[stage 5](../../coherence-plan.md#5-add-an-optional-structural-adapter-for-fingertree)
of the plan, not started.

The fingertree is not usable with the tree algorithms. There is no layer
type, projection, or embedding for `FingerTree`; `beman::typeclass_tree_adapters`
does not exist; and no header includes both libraries. The only connection is
through transpose's Functor, Foldable and Traversable instances.

Probe [`fingertree-with-tree-algorithms.cpp`](probes/fingertree-with-tree-algorithms.cpp):

```text
ta::BinaryTree<int>    project=1  ta::fold_map bridge=1  transpose::fold_map=1  transpose::traverse=1
ta::FringeTree<int>    project=1  ta::fold_map bridge=1  transpose::fold_map=1  transpose::traverse=1
ft::FingerTree<int>    project=0  ta::fold_map bridge=0  transpose::fold_map=1  transpose::traverse=1
```

The rejection is soft, which is correct. The gap is that the claim "usable
with the tree algorithms" has no code or test behind it.

Direction: stage 5 as planned, plus one cross-library test in the suite so the
property is maintained and not only reviewed.

### fold-map-meaning

**High. New.**

`fold_map` names two things of different kinds with different argument orders.

| | Kind | Call |
|---|---|---|
| `transpose::fold_map` (`include/beman/transpose/fold.hpp:1143`) | operation object | `(f, structure, monoid)` |
| `tree_algorithms::fold_map` (`include/beman/tree_algorithms/fold_map.hpp:55`, `:79`) | function-template overload set | `<Result>(map_fn, combine, identity, …, tree)` |
| the monoid bridge (`include/beman/tree_algorithms/fold_map_lookup.hpp:141`) | function template | `(f, tree, monoid)` |

Probe [`name-clash.cpp`](probes/name-clash.cpp): with both namespaces in scope
the call is `error: reference to 'fold_map' is ambiguous`. In one standard
namespace an object and a function template cannot share a name at all.

`include/beman/transpose/functor.hpp:160–173` commits to the opposite: "A name
in here means one thing across the entire family."

The papers inherit the split. `papers/algorithms-for-trees.md:528` says
`fold_map` is the primitive that paper proposes and that `fold_left` and its
family derive from it. In code the derived family comes from the Foldable
instance member in `fold.hpp`, which is labeled evidence
(`include/beman/transpose/fold.hpp:17–23`) and which no paper proposes. Only
the four registered library trees connect the two through the bridge.

Direction: decide one name, one kind, and one argument order. The existing
`_with` convention (`fold_with`, `unfold_with`) suggests the explicit-ingredient
form takes a suffix and the operation object keeps the plain name. This is a
shared-vocabulary change and needs the sweep the coherence skill describes.

### proposal-homes

**High. Part new, part already open.**

Three places where proposed text depends on something unproposed.

- **Functor (new).** Paper D's lookup tier resolves `fmap` through "P3200's
  `functor<Layer>`" (`papers/algorithms-for-trees.md:188`, `:296`, `:521`).
  P3200 proposes Applicative and Traversable only;
  `include/beman/transpose/sequence.hpp:44–45` says "Functor is not proposed by
  this paper". Tree layers are not Applicatives, so they cannot obtain `fmap`
  from an Applicative instance either. The lookup tier therefore has no
  proposed Functor to consume.
- **Monoid (new).** `FingerTree`'s default template argument is
  `beman::transpose::sum_monoid<std::size_t>`
  (`include/beman/fingertree/tree/finger_tree.hpp:299`), and the wrappers are
  built on `tuple_monoid` and `max_monoid`. P3200 does not propose Monoid,
  Paper D keeps it out of its primary API (`papers/algorithms-for-trees.md:172`),
  and Paper C is undrafted. The monoid vocabulary has three consumers and no
  home.
- **Traverse derivation (already open,**
  [stage 6](../../coherence-plan.md#6-make-coherence-a-maintained-property)**).**
  `papers/algorithms-for-trees.md:413` says `traverse` for any fixed-point
  tree is derivable through the layer's mapping function.
  `include/beman/tree_algorithms/tree_typeclasses.hpp:207–208` says the
  opposite: "A structural Functor alone does not declare payload traversal."

Direction: name the paper that proposes Functor and the paper that proposes
the Monoid vocabulary, or restate the dependents so they do not need them.
Correct the paper sentence on `traverse`.

### lookup-tier-rejection

**Medium. New.**

The tree algorithms' lookup tier does not follow the mechanism's own rule for
unavailable calls. `include/beman/transpose/detail/typeclass_base.hpp:90–94`
states that rule, recorded as
[operation-object-wording](../../../transpose/decisions.md#operation-object-wording):
an operation is constrained on the call it makes, so an unregistered type is a
call that does not match.

The lookup-tier `fold_map<Result>(f, combine, identity, tree)` and
`fold_fix<Result>(algebra, tree)` are unconstrained and fail by
`static_assert` in the body
(`include/beman/tree_algorithms/fold_map_lookup.hpp:67`, `:97`;
`include/beman/tree_algorithms/recursion_schemes_lookup.hpp:44`).

Probe [`lookup-hard-error.cpp`](probes/lookup-hard-error.cpp): a concept
probing either call on an unregistered type evaluates **true**, so the probe's
`static_assert(!…)` fails, and an actual call then errors on instantiation.
The same file shows transpose's `fold_map` and `fmap` objects and the 3-argument
bridge all answering false softly.

Two related shape differences:

- The lookup tier is function templates, found by ADL and open to overloading;
  transpose's operations are objects.
- `layer_fold<Layer>` and `project<Tree>` are bare callables. Neither has a
  named operation object, a derive base, or an object concept, so nothing can
  ask whether a registration is well-formed.
  [layer-fold-and-foldable](../../machinery-deduplication.md#layer-fold-and-foldable)
  is still open.

Direction: constrain the lookup overloads the way the bridge already is
(`detail::registered_fold_source`), and settle `layer-fold-and-foldable` so the
two lookup variables either become typeclass objects or are documented as a
different, lighter shape.

### instance-header-placement

**Medium. New.**

[instance-placement](../../../transpose/decisions.md#instance-placement) rules
that a type's author puts its instances in the type's header, because use
before specialization is ill-formed, no diagnostic required. Neither consumer
does this.

- `binary_tree.hpp` and its siblings define the types; their payload instances
  are in `tree_typeclasses.hpp`.
- `finger_tree.hpp` defines the container; its instances are in
  `finger_tree_functor.hpp`, `finger_tree_foldable.hpp`,
  `finger_tree_traversable.hpp` and `finger_tree_monoid.hpp`.

Probe [`instance-placement.cpp`](probes/instance-placement.cpp), for both
`BinaryTree<int>` and `FingerTree<int>`:

- With only the type's header, `has_instance_v<foldable<Tree>>` is false.
- Asking that question and then including the instance header is a GCC 16
  error: `partial specialization … after instantiation`.
- Clang 22 accepts the same translation unit and prints `before=0 after=0`.
  The instance is silently absent.

A related shape: `include/beman/tree_algorithms/tree_typeclasses.hpp:320–328`
registers with blanket constrained specializations
(`template <class Tree> requires payload_tree<Tree> … functor<Tree>`). The
mechanism's documentation does not describe this registration shape. Two
libraries each doing it would be ambiguous for a type that satisfies both
constraints.

Direction: either include the instance headers from the type headers, or
record why these two consumers are exempt and what a user must include. Decide
whether constrained blanket registration is a supported shape.

### shape-contents-derivation

**Medium. New.**

The same Functor/Traversable derivation is written twice. Both copies record
the element positions, traverse them as a `std::vector`, and rebuild the shape
with `applicative.invoke(builder, …)`:

- `include/beman/fingertree/tree/finger_tree_traversable.hpp:27–57`
- `include/beman/tree_algorithms/tree_typeclasses.hpp:276–302`

They carry different constraint sets, and both miss the final `invoke`
(`finger_tree_traversable.hpp:51`, `tree_typeclasses.hpp:298`).

Probe [`collect-carrier.cpp`](probes/collect-carrier.cpp): a policy that
satisfies `applicative_object_for<App, eff<int>>`, but whose native `collect`
returns a carrier its own `invoke` does not accept, traverses a `std::vector`
correctly. For `FingerTree<int>` and `BinaryTree<int>` it is a hard error from
the body, including inside a `requires` probe.

There are also two rebind traits doing one job: `finger_tree_rebind` and
`payload_model<Tree>::rebind`.

Direction: one shape-plus-contents derivation owned by transpose, with the
complete constraint, used by both consumers. This removes the duplicated
constraint and gives the rebind trait one spelling.

### payload-foldable-copies

**Medium. New as a cross-layer inconsistency;** the copying itself is accepted
in [coherence-contracts](../../coherence-contracts.md).

On the tree algorithms' trees, Foldable is the weakest of the three instances.
A read-only `length` or `any_of` requires copyable payloads and copies them,
while `fmap` and `traverse` on the same tree do neither.

Probe [`fold-copies.cpp`](probes/fold-copies.cpp):

| Structure | Copies per element in `length` | `length`, `any_of` with a move-only payload | `fmap`, `traverse` with a move-only payload |
|---|---|---|---|
| `BinaryTree`, `FringeTree` | 2 | unavailable | available |
| `BinaryTreeFix`, `RoseTreeFix` | 1 | unavailable | available |
| `FingerTree`, `std::vector` | 0 | available | available |

The cause is that the projection copies the payload into the layer
(`include/beman/tree_algorithms/binary_tree.hpp:268`) and the layer `fmap`
copies it again (`:189` requires `copy_constructible<T>`), and the payload
Foldable delegates to that structural fold
(`include/beman/tree_algorithms/tree_typeclasses.hpp:262–272`).

`papers/algorithms-for-trees.md:343` says `fold_map` "folds the tree where it
stands".

Direction: `payload_model<Tree>::record` already yields element positions for
Functor and Traversable. A Foldable over those positions would copy nothing
and accept move-only payloads. It needs a law test pinning it to the
structural fold, with a non-commutative monoid, and a decision that payload
Foldable need not be derived from `fold_with`.

### fingertree-foldable-natives

**Medium-low. New.**

The fingertree's Foldable instances supply only `fold_map`
(`include/beman/fingertree/tree/finger_tree_foldable.hpp:19`), so `length` and
`empty` are derived by a full fold. The container knows both in O(1)
(`include/beman/fingertree/tree/finger_tree.hpp:1098`, `:1104`). The vector
reference instance supplies a native `length`
(`include/beman/transpose/sequence.hpp:164`).

Probe [`fold-cost.cpp`](probes/fold-cost.cpp), 2,000,000 elements, `-O2`:

```text
n=2e6: tree.empty() 0.0us  transpose::empty(tree) 13394.0us
n=2e6: tree.size()  0.0us  transpose::length(tree) 13426.3us
n=2e6: vec.size()   0.0us  transpose::length(vec)  0.0us  transpose::empty(vec) 0.0us
```

Also:

- The Functor and Traversable Impls are shared across the base tree and its
  wrappers; the Foldable Impls are five separate structs.
- Rope's fold iterates `rope.chunks()`, which materializes a copy of every
  chunk (`include/beman/fingertree/tree/finger_tree_rope.hpp:160`).

Direction: supply native `length` and `empty` where the measure gives them,
share one Foldable Impl as the other two typeclasses do, and fold Rope by
reference.

### deep-concept-witness

**Low-medium. New.**

`functor_object` and `traversable_object` probe with a witness that maps the
element type to itself (`include/beman/transpose/functor.hpp:287–292`). They
therefore certify same-type maps only.

Probe [`endo-map.cpp`](probes/endo-map.cpp), for a tree whose measure accepts
only `int`:

```text
weighted tree: functor_object=1 traversable_object=1 fmap int->int=1 fmap int->string=0 traverse->optional<string>=0
```

The instance is honest about each call. The concept name promises more than it
checks. A reader of the Standard would take `functor_object` to mean Functor.

Direction: either state in the concept's description that it checks the
same-type case, or decide whether a structure that can only map to its own
element type should register `functor` at all.

### vocabulary-drift

**Low. New.** Small disagreements that add up to a second dialect.

- **Element metadata has two spellings.** Instances declare
  `Impl::element_type`; lookups go through `applicative_value<T>`. The latter
  is named for Applicative but is specialized for tree layers, which are not
  Applicatives (`include/beman/tree_algorithms/expression.hpp:121–124`).
- **Three registration shapes.** Lookup variables (`functor<T>`), a class
  template registry (`Monoid<T>`), and trait structs (`applicative_value`,
  `finger_tree_rebind`).
- **Generic monoids in the wrong library.** `optional_min_monoid` and
  `optional_max_monoid` are general and live in fingertree
  (`include/beman/fingertree/tree/finger_tree_priority_queue.hpp:30`). They
  are not `constexpr` or `noexcept` as transpose's monoids are.
- **`empty` means two things.** `FringeTree::empty()` is a factory
  (`include/beman/tree_algorithms/fringe_tree.hpp:83`). `FingerTree::empty()`
  and `transpose::empty` are predicates.
- **Stale names.** Comments still say `functor_typeclass`
  (`include/beman/tree_algorithms/expression.hpp:34`, `:35`, `:212`;
  `include/beman/tree_algorithms/functors.hpp:34`;
  `docs/tree_algorithms/DECISIONS.md:56`, `:133`).
- **Numbered references.** Header comments cite "Decision 4" and "Decision 6"
  (`include/beman/tree_algorithms/fold_map_lookup.hpp:28`,
  `include/beman/tree_algorithms/recursion_schemes_lookup.hpp:24`,
  `include/beman/tree_algorithms/binary_tree.hpp:52`), and tests cite
  `DEV-01` and `DEV-04`. These are serial identifiers, not names.
- **Layers are `std::variant` aliases.** `ExprF`, `NatF` and `FringeTreeF`
  are alias templates for `std::variant`, so their Functor and `layer_fold`
  registrations are keyed on `std::variant` specializations
  (`include/beman/tree_algorithms/expression.hpp:72`, `:128`, `:207`). The
  instance is registered on a `std` type, not on a type the library owns.

## What holds

- The explicit tier (`fix`, `box`, `child_slot`, `recursion_schemes`,
  `fold_map`) includes nothing from transpose.
- There is one definition of the lookup machinery, in transpose. Neither
  consumer carries a copy, and neither links the other.
- The stage 1–4 repairs are in the code: fingertree traversal goes through
  the vector instance's `collect` route, append passes the stored monoid,
  layer Functors and fingertree Foldable are constrained, rebinding preserves
  the tree's configuration, and Rope has no generic Functor or Traversable.
  `endo-map.cpp` confirms the last:
  `rope: functor registered=0 traversable registered=0 foldable registered=1`.

## Decisions needed

These are design rulings, not repairs.

- [fold-map-meaning](#fold-map-meaning): which single name, kind, and argument
  order survives.
- [proposal-homes](#proposal-homes): which paper proposes Functor, and which
  proposes the Monoid vocabulary.
- [payload-foldable-copies](#payload-foldable-copies): whether payload Foldable
  may walk positions directly, pinned to the structural fold by a law test, or
  must stay derived from `fold_with`.
- [instance-header-placement](#instance-header-placement): whether the
  instance headers merge into the type headers, and whether constrained
  blanket registration is supported.

## Reproducing the probes

The sources are in [probes/](probes/). From anywhere in the worktree, after a
GCC 16 build has generated the config headers under
`.build/build-gcc-16/include`:

```sh
docs/coordination/review/2026-11-16-layer-coherence/probes/run.sh
```

`CXX`, `CLANGXX` and `BUILD_INCLUDE` override the compilers and the generated
header directory. The script compiles each probe with
`-std=gnu++26 -Iinclude -I$BUILD_INCLUDE -Wall -Wextra`.

| Probe | Finding | Selector |
|---|---|---|
| `fingertree-with-tree-algorithms.cpp` | [fingertree-structural-adapter](#fingertree-structural-adapter) | none |
| `name-clash.cpp` | [fold-map-meaning](#fold-map-meaning) | none; must fail to compile |
| `lookup-hard-error.cpp` | [lookup-tier-rejection](#lookup-tier-rejection) | `-DPROBE=0` baseline, `1` lookup `fold_map`, `2` lookup `fold_fix` |
| `instance-placement.cpp` | [instance-header-placement](#instance-header-placement) | `-DWHICH=0` BinaryTree, `1` FingerTree; `-DLATE` includes the instance header after first use |
| `collect-carrier.cpp` | [shape-contents-derivation](#shape-contents-derivation) | `-DWHICH_S=0` vector, `1` FingerTree, `2` BinaryTree |
| `fold-copies.cpp` | [payload-foldable-copies](#payload-foldable-copies) | none |
| `fold-cost.cpp` | [fingertree-foldable-natives](#fingertree-foldable-natives) | none; built `-O2` |
| `endo-map.cpp` | [deep-concept-witness](#deep-concept-witness) | none |

Observed output at `4522a1b` (diagnostics truncated; timings vary by machine):

```text
== name-clash: both namespaces in scope
…/probes/name-clash.cpp:9:12: error: reference to ‘fold_map’ is ambiguous
== instance-placement WHICH=0: type header only (rc 0 means no instance visible)
   rc=0
== instance-placement WHICH=0 LATE: instance header after first use, gcc
include/beman/tree_algorithms/tree_typeclasses.hpp:325:23: error: partial specialization of ‘beman::transpose::foldable<Tree>’ after instantiation of ‘beman::transpose::foldable<beman::tree_algorithms::BinaryTree<int> >’ [-fpermissive]
== instance-placement WHICH=0 LATE: instance header after first use, clang
before=0 after=0
== instance-placement WHICH=1: type header only (rc 0 means no instance visible)
   rc=0
== instance-placement WHICH=1 LATE: instance header after first use, gcc
include/beman/fingertree/tree/finger_tree_foldable.hpp:54:23: error: partial specialization of ‘beman::transpose::foldable<beman::fingertree::tree::FingerTree<T, MONOID, MP, Alloc> >’ after instantiation of ‘beman::transpose::foldable<beman::fingertree::tree::FingerTree<int> >’ [-fpermissive]
== instance-placement WHICH=1 LATE: instance header after first use, clang
before=0 after=0
== lookup-hard-error PROBE=0
   rc=0
== lookup-hard-error PROBE=1
…/probes/lookup-hard-error.cpp:28:15: error: static assertion failed
   rc=1
== lookup-hard-error PROBE=2
…/probes/lookup-hard-error.cpp:30:15: error: static assertion failed
   rc=1
== collect-carrier WHICH_S=0
vector: ok, 3 collected
== collect-carrier WHICH_S=1
…/probes/collect-carrier.cpp:53:67:   required from here
include/beman/fingertree/tree/finger_tree_traversable.hpp:51:34: error: no matching function for call to ‘App::invoke(…)’
== collect-carrier WHICH_S=2
…/probes/collect-carrier.cpp:53:67:   required from here
include/beman/tree_algorithms/tree_typeclasses.hpp:298:34: error: no matching function for call to ‘App::invoke(…)’
== fold-copies
-- move-only payload: which typeclass operations are available
std::vector<P>               length=1 any_of=1 fmap=1 traverse=1
ft::FingerTree<P>            length=1 any_of=1 fmap=1 traverse=1
ta::BinaryTree<P>            length=0 any_of=0 fmap=1 traverse=1
ta::BinaryTreeFix<P>         length=0 any_of=0 fmap=1 traverse=1
ta::FringeTree<P>            length=0 any_of=0 fmap=1 traverse=1
ta::RoseTreeFix<P>           length=0 any_of=0 fmap=1 traverse=1
-- payload copies made by a read-only length() over 3 elements
BinaryTree    length=3 copies=6
BinaryTreeFix length=3 copies=3
FringeTree    length=3 copies=6
RoseTreeFix   length=3 copies=3
FingerTree    length=3 copies=0
vector        length=3 copies=0
== fold-cost
n=2e6: tree.empty() 0.0us  transpose::empty(tree) 13394.0us
n=2e6: tree.size()  0.0us  transpose::length(tree) 13426.3us
n=2e6: vec.size()   0.0us  transpose::length(vec)  0.0us  transpose::empty(vec) 0.0us
== endo-map
weighted tree: functor_object=1 traversable_object=1 fmap int->int=1 fmap int->string=0 traverse->optional<string>=0
rope: functor registered=0 traversable registered=0 foldable registered=1
== fingertree-with-tree-algorithms
ta::BinaryTree<int>    project=1  ta::fold_map bridge=1  transpose::fold_map=1  transpose::traverse=1
ta::FringeTree<int>    project=1  ta::fold_map bridge=1  transpose::fold_map=1  transpose::traverse=1
ft::FingerTree<int>    project=0  ta::fold_map bridge=0  transpose::fold_map=1  transpose::traverse=1
```
