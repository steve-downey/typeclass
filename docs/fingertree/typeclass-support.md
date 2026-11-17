# Fingertree typeclass interfaces

The instances use the Functor, Applicative, Foldable, and Traversable machinery
from beman.transpose. Include the corresponding consumer header before the
first lookup. Persistent inputs present their payloads by const reference,
including when the tree handle is an rvalue.

| Representation | Element | Functor | Foldable | Generic Traversable |
|---|---|---|---|---|
| FingerTree | T | finger_tree_functor.hpp | finger_tree_foldable.hpp | finger_tree_traversable.hpp |
| FingerTreeRandomAccess | T | wrapper header | wrapper header | wrapper header |
| FingerTreePriorityQueue | T | wrapper header | wrapper header | wrapper header |
| FingerTreeIntervalIndex | payload T | wrapper header | wrapper header | wrapper header |
| FingerTreeRope | string chunk | none | wrapper header | traverse chunks explicitly |

Folds and traversals use left-to-right sequence order. Interval-index folds
visit payloads; traversal retains each interval's start and end. Priority
queue traversal follows the stored sequence, rather than priority order.
The result monoid passed to `fold_map` is independent of a tree's measure
monoid and must support `identity()` and `combine()` for the mapped result.

The complete Foldable object includes copying operations such as `to_vector`
and `find_first`. These require copyable payloads. A base FingerTree containing
move-only values can still support reference-reading operations such as
`fold_map` and `length`; it need not satisfy the complete object concept.

Functor and Traversable share the configured reconstruction contract below.
Both read persistent payloads by const reference, including rvalue handles.
Fresh result payloads move into their leaves after their measures are computed.
No source payload is copied into traversal's temporary position list.

## Configured reconstruction

Same-payload mapping retains the concrete tree type, measure policy type,
allocator instance, and monoid instance. All cached measures are recomputed
from the new payloads. For a count policy, type-changing mapping rebinds
`UnitMeasure<T, Tag>` to `UnitMeasure<U, Tag>` and retains the allocator and
monoid. A PMR result keeps the source resource:

```cpp
#include <beman/fingertree.hpp>
#include <memory_resource>
#include <string>

namespace ft = beman::fingertree::tree;
namespace bt = beman::transpose;

std::pmr::monotonic_buffer_resource resource;
const auto tree = ft::pmr::FingerTree<int>::from_sequence({1, 2, 3}, &resource);
auto strings = bt::fmap([](const int& n) { return std::to_string(n * 10 + 1); }, tree);
// strings is pmr::FingerTree<string>, using resource, with measure 3.
```

Other type changes require a specialization of
`beman::fingertree::finger_tree_rebind<SourceTree, U>` in the consumer program.
It supplies `type` and `static make_builder(const SourceTree&)`. The returned
owned callable accepts `vector<U>&&` and returns exactly `type`. It must be
move-constructible and callable as a named lvalue. It can have a non-const call
operator. Capture configuration by value; a deferred effect may reconstruct
after the source tree and mapper have been destroyed. An external PMR resource
must still outlive all objects allocated from it.

An undefined or malformed rebind rejects mapping and traversal through
constraints. A measure callable accepting a new payload does not declare the
new representation's measure law. A weighted tree therefore does not silently
become a count-measured tree when mapped to strings or units.

The three payload-polymorphic wrappers use their backing family's declared
rebind. They provide an explicit constructor from a configured backing tree,
`backing_tree()` returning a const reference, and `for_each` over const-reference
payloads. The interval builder owns each endpoint pair independently of the
payload effects and preserves their order. The default priority and interval
backing families declare their own rebinds while retaining the byte allocator.
Priority results must be copyable and totally ordered because their measures
store extrema. Custom backing families require their own declarations.

## Concatenation monoids

`finger_tree_monoid.hpp` registers the canonical `Monoid<FingerTree<T>>` only
for default count-measured, default-allocated trees. It delegates to native
persistent concatenation and supports move-only payloads. Element order is
the left sequence followed by the right sequence.

For other configurations, construct an explicit
`beman::fingertree::finger_tree_concat_monoid<Tree>` from a source prototype.
Its identity owns that allocator and monoid configuration. Its `combine`
rebuilds both operands under that configuration, copying their persistent
payloads and recomputing every measure. This takes O(n + m) time and requires
copyable payloads. Associativity and the identity laws apply within the
prototype's compatible measure-law domain. Native append may share cached
nodes only when the operands' measure laws and monoid state are compatible;
allocator equality alone does not establish that compatibility.

## Structural access

The sequence interface hides the tree's shape, and its operations are free
to rebalance it. `finger_tree_structure.hpp`, included by the library
umbrella, exposes the shape as it is. It names no algorithm library.

`FingerTreeLevel<Tree>` is the tree at one level of the spine, in its real
form: `EMPTY`, `SINGLE`, or `DEEP` with a left digit, the next level, and a
right digit. Level 0 is the sequence; the nodes held at level d have height
d. `FingerTreeNode<T, Monoid>` is one element: a `LEAF` holding a payload, or
a `NODE2` or `NODE3` of nodes one lower. Both report their cached measure.

```cpp
#include <beman/fingertree.hpp>

namespace ft = beman::fingertree::tree;

const auto tree = ft::FingerTree<int>::from_sequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
const ft::FingerTreeLevel<ft::FingerTree<int>> root(tree);
// root.form() is DEEP; root.left() and root.right() hold two leaves each;
// root.spine() is level 1 and holds two Node3 elements.
```

Levels and nodes are owning values that share the immutable structure.
Copying one is O(1), reading copies no payload, and a part remains valid
after the tree it was read from is destroyed. `share_value()` returns a
`shared_ptr<const T>` to a leaf's payload on the same terms.

`FingerTreeStructureBuilder<Tree>` constructs the same forms under one
allocator and one monoid instance, taken from a prototype tree or given
directly, and computes every cached measure. `leaf`, `node2` and `node3` make
nodes; `empty`, `single` and `deep` make levels. The builder's preconditions
are the representation's invariants: digits of one to four nodes of one
height, a spine one level below its digits, node children of one height.
They are checked by `assert`. Parts combined by one builder must share its
configuration; a cached measure is valid only under the monoid that computed
it.

An inner spine level is not a sequence, and no interface hands one out as a
tree. `FingerTreeLevel::sequence()` is the only conversion, and it requires a
level that is empty or holds leaves.

All four capability wrappers expose `backing_tree()`, a const reference to
the tree they are built on, which is how structural access reaches them.

The recursive algorithms of beman.tree_algorithms are connected to this
access by the optional
[structural adapter](../typeclass_tree_adapters/README.md). Fingertree itself
does not depend on that library.

## Migrating Rope traversal

Rope stores string chunks and has no representation for arbitrary mapped
payloads. Its generic Traversable registration has been removed. Traverse the
chunk vector to obtain arbitrary contextual results, and reconstruct a Rope
explicitly when the results are strings:

```cpp
#include <beman/fingertree/tree/finger_tree_rope.hpp>
#include <beman/transpose/apply.hpp>
#include <beman/transpose/traverse.hpp>
#include <optional>

namespace ft = beman::fingertree::tree;
namespace bt = beman::transpose;

const auto rope = ft::FingerTreeRope<>::from_chunks({"ab", "c"});
const auto chunks = rope.chunks();
auto lengths = bt::traverse(
    [](const std::string& s) { return std::optional{s.size()}; }, chunks);
// lengths holds vector<size_t>{2, 1}.

auto strings = bt::traverse(
    [](const std::string& s) { return std::optional{s + "!"}; }, chunks);
auto rebuilt = bt::applicative<std::optional<std::string>>.invoke(
    [](std::vector<std::string> values) {
        return ft::FingerTreeRope<>::from_chunks(std::move(values));
    }, std::move(strings));
// rebuilt holds a Rope with text "ab!c!".
```

The vector Traversable supplies native Applicative collection where available.
Reconstruction inside the context also supports deferred execution after the
source Rope and chunk vector have been destroyed. As with any deferred
operation, the mapper must own the values its returned effects need.
