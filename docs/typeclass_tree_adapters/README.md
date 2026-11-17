# beman.typeclass_tree_adapters: optional adapters between the libraries

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

`beman.tree_algorithms` and `beman.fingertree` do not depend on each other.
This component is where they meet. It is opt-in: neither library includes its
headers or links its target, so a program that wants a finger tree and the
recursive tree algorithms together asks for that by name.

```cmake
find_package(beman.typeclass_tree_adapters REQUIRED)
target_link_libraries(yourlib PUBLIC beman::typeclass_tree_adapters)
```

The package finds `beman.tree_algorithms`, `beman.fingertree` and, through
them, `beman.transpose`. It needs what fingertree needs: C++26 and
`std::inplace_vector`. It is not built when `BEMAN_TYPECLASS_USE_MODULES` is
on.

This is implementation evidence, like the payload typeclass adapters. No
paper proposes it, and neither library's proposal depends on it.

## Support across the libraries

Every structure below registers Functor, Foldable and Traversable over its
payload, and each instance satisfies the deep object concept of its
typeclass. The last column is how a structural fold reaches it.

| Structure | Library | Payload order | Structural fold |
|---|---|---|---|
| `std::vector<T>` | transpose | index order | none; it is not a tree |
| `FingerTree<T>` | fingertree | left to right | `fold_map(f, tree, monoid)`, with this adapter |
| `FingerTreeRandomAccess<T>` | fingertree | left to right | `backing_tree()`, with this adapter |
| `FingerTreePriorityQueue<T>` | fingertree | stored sequence, not priority | `backing_tree()`, with this adapter |
| `FingerTreeIntervalIndex<T>` | fingertree | stored sequence | `backing_tree()`, payload is `Interval<T>` |
| `BinaryTree<T>`, `BinaryTreeFix<T>` | tree_algorithms | in-order | `fold_map(f, tree, monoid)` |
| `FringeTree<T>` | tree_algorithms | left to right | `fold_map(f, tree, monoid)` |
| `RoseTreeFix<T>` | tree_algorithms | pre-order | `fold_map(f, tree, monoid)` |

`FingerTreeRope` registers Foldable over its string chunks only, and `Expr`
registers Foldable over its integer constants only; neither has a payload
that can change type. Their guides say what to use instead:
[fingertree](../fingertree/typeclass-support.md) and
[tree_algorithms](../tree_algorithms/typeclass-support.md).

For every row, and with the same code for each:

- Folds, and the effects of a traversal, follow the payload order above.
  Each effect is composed once.
- `traverse` works with `optional`, `expected`, a graded `expected` under
  both the short-circuit and the accumulating policy, a void-valued effect
  (the result holds units), and a lanewise context such as `zip_list`, where
  reconstruction runs once per lane.
- A policy's native `collect` is used once when it has one, including a
  policy whose composition changes carrier type; a policy without one is
  sequenced pairwise.
- `fmap` obeys identity and composition, `traverse` with a pure lift is
  `fmap`, and `transpose` is the identity traversal.
- Where there is a structural fold, it agrees with the Foldable for a
  noncommutative monoid.
- An interval index keeps every interval's endpoints through all of the
  above, and a PMR finger tree keeps its resource.

These are the columns of
[`coherence_matrix.test.cpp`](../../tests/beman/typeclass_tree_adapters/coherence_matrix.test.cpp),
which instantiates one set of checks for every row. The test lives with this
component because it is the one target that links all three libraries.

Genuine `std::execution` senders are exercised by transpose's opt-in P2300
evidence, over `std::vector`. The matrix covers the same sequencing route
for every row with a generic collecting context, which needs no external
dependency.

## The structural fingertree adapter

```cpp
#include <beman/typeclass_tree_adapters/finger_tree.hpp>
```

Including this header makes a `FingerTree` a tree the structural verbs can
see. What they see is the representation, not the sequence: one layer is one
real node of the tree as fingertree stores it.

| Layer alternative | Form | Recursive positions |
|---|---|---|
| `FingerEmpty` | a spine level with no elements | none |
| `FingerSingle<A>` | a spine level with one element | the element |
| `FingerDeep<A>` | a left digit, the next spine level, a right digit | one to four elements, an optional spine, one to four elements |
| `FingerLeaf<P>` | an element holding one payload | none |
| `FingerNode2<A>` | an element of two elements | both, in order |
| `FingerNode3<A>` | an element of three elements | all three, in order |

`FingerTreeF<P, A>` is the layer. `A` fills the recursive positions and `P` is
the payload slot of a Leaf. Sequence operations may rebalance a tree freely;
a structural fold reports whatever shape is there, so two equal sequences
can have different shapes.

The representation has two recursive sorts, the tree at a spine level and an
element node. The verbs recurse through one position type, so
`FingerTreePart<TREE>` holds either. It is the same type at every depth,
which is what lets one fold cover a tree of any size.

### Which verb, which ingredients

| To do this | Call |
|---|---|
| Fold the structure in place | `fold_with<R>(algebra, fmap, finger_tree_project, &tree)` |
| Fold the elements, everything looked up | `fold_map<R>(map, combine, identity, tree)` |
| Fold the elements with a monoid object | `fold_map(map, tree, monoid)` |
| Fold the elements, everything explicit | `fold_map<R>(map, combine, identity, finger_tree_layer_fold_map, fmap, finger_tree_project, &tree)` |
| Rebuild a tree layer by layer | `unfold_with<Part>(finger_tree_project, fmap, FingerTreeEmbed<Tree>(tree), Part(tree)).sequence()` |
| Build a tree from seeds | `unfold_with<Part>(coalgebra, fmap, FingerTreeEmbed<Tree>{}, seed).sequence()` |
| Convert to and from the Fix form | `to_fix(tree)`, `from_fix(fixed, embed)` |
| Fold the Fix form | `fold_fix<R>(algebra, fixed)`, `fold_map<R>(map, combine, identity, fixed)` |
| Unfold and fold without building | `refold<R, FingerTreeLayer<T>::F>(algebra, coalgebra, seed)` |

`fmap` is `beman::transpose::fmap`, or leave it out and the lookup overloads
find the layer's Functor.

A complete program,
[`examples/typeclass_tree_adapters/finger_tree_structure.cpp`](../../examples/typeclass_tree_adapters/finger_tree_structure.cpp),
draws a tree's shape, folds its elements and rebuilds it. It runs with the
tests. For twelve elements appended one at a time it prints:

```text
shape:    [0 | [<1 2 3> | - | <4 5 6> <7 8 9>] | 10 11]
elements: 0,1,2,3,4,5,6,7,8,9,10,11,
rebuilt:  equal, measure 12
```

### Order

A Deep folds its left digit, its spine, then its right digit, each left to
right. A node folds its children left to right. That is the sequence order,
so the elementwise structural fold gives the same result as the Foldable
fingertree registers for itself, for any monoid including a noncommutative
one. The layer Functor visits positions in the same order.

### Ownership and copying

A part owns a share of the immutable structure it denotes. Copying one is
O(1), and a part stays valid after the tree it was read from is gone. A
projected Leaf holds a `FingerPayloadRef<T>`, which shares the stored payload
in the same way. So:

- Projection and folding copy no payload.
- A projected layer can be kept, returned or stored; nothing in it dangles.
- Each part and each shared payload costs a reference-count adjustment. For a
  plain fold over the elements, fingertree's own Foldable is cheaper.

`finger_payload(leaf.value)` reads a Leaf's payload whichever kind of slot it
is, so one algebra serves projected layers and owned ones.

An owned layer, `FingerTreeF<T, A>`, stores the payload itself. A coalgebra
that makes fresh payloads produces these, and so does
`finger_tree_project_owned`, which copies. The Fix form is made of owned
layers.

### Embedding

`FingerTreeEmbed<TREE>` rebuilds one layer. It is constructed from a prototype
tree, or from a `FingerTreeStructureBuilder<TREE>`, and builds under that
allocator and that monoid instance. Every node goes through the builder,
which computes its cached measure, so a tree built by an unfold satisfies the
measure invariant by construction. A stateful monoid and a PMR resource are
carried through.

An embedding moves the payload out of an owned layer it is given as an
rvalue, and copies from a layer it is only shown or from a shared slot.

The layers it is given must be layers of a well-formed finger tree: the right
sort in each position, digits of one to four elements of one height, a spine
one level below its digits, node children of one height. Layers projected
from a tree always are. A hand-written coalgebra has to produce them, and a
violation is caught by `assert`.

`sequence()` turns the finished part into a tree. It requires the part to be
a spine level that is empty or holds leaves, which is what a whole tree is.

### Capability wrappers

`FingerTreeRandomAccess`, `FingerTreePriorityQueue`, `FingerTreeIntervalIndex`
and `FingerTreeRope` are not registered as structural trees. Their structure
is their backing tree's, and you name it:

```cpp
auto text = beman::tree_algorithms::fold_map<std::string>(
    show_interval, concat, std::string{}, index.backing_tree());
```

The payload is then the backing tree's. For the interval index that is the
whole `Interval<T>`, with its endpoints; the index's Foldable sees only `T`.
For the rope it is a string chunk.

### What is not supported

- **Mapping an owned move-only payload.** The layer Functor copies a Leaf's
  slot. Move-only trees can be projected and folded, because a shared slot
  copies cheaply, but not converted to the Fix form or rebuilt from owned
  layers through `fmap`.
- **Payload typeclasses on the Fix form.** `FingerTreeFix<T>` has no
  registered payload type. Use the tree.
- **Module builds.** See above.

The access this adapter is built on,
`beman/fingertree/tree/finger_tree_structure.hpp`, names no algorithm library
and can be used directly; see the
[fingertree typeclass guide](../fingertree/typeclass-support.md#structural-access).
