# Payload typeclass support

Include `<beman/tree_algorithms/tree_typeclasses.hpp>` (or the library
umbrella) before using payload typeclasses. These adapters consume
beman.transpose and are implementation evidence; the proposed structural
algorithms retain their explicit-callable interfaces.

| Representation | Payload Functor | Foldable | Traversable | Payload order |
|---|---|---|---|---|
| `BinaryTree<T>` | yes | yes | yes | in-order |
| `BinaryTreeFix<T>` | yes | yes | yes | in-order |
| `FringeTree<T>` | yes | yes | yes | left-to-right leaves |
| `RoseTreeFix<T>` | yes | yes | yes | pre-order, children left-to-right |
| `Expr` | none | integer constants | none | left-to-right constants |
| `Nat`, arbitrary `Fix<F>` | none claimed | none claimed | none claimed | representation-specific |

Structural layer Functors still map recursive child positions. For example,
`fmap` on `BinaryTreeF<T, A>` changes `A`, while `fmap` on `BinaryTree<T>`
changes `T`. Fix metadata is declared in the consumer header, alongside the
supported registrations. A layer Functor alone supplies insufficient
information to select a tree's payload type, rebind it, and sequence payload
effects.

## Mapping and traversal

Payload mapping and traversal read every source value through `const T&`,
including when the source handle is an rvalue. A named callable is invoked
as an lvalue. Fresh result values move into a new tree; neither operation
requires copying persistent source values. Invalid callables, unstorable
results, and invalid effect policies reject through constraints.

Binary reconstruction preserves absent left/right children. Fringe
reconstruction preserves Empty/Leaf/Branch topology and computes cached leaf
counts through the ordinary factories. Rose reconstruction preserves each
node's child count and order. Result payload types may change, including to
move-only values or units. An expression language with fixed integer literals
cannot provide that generic result family, so Expr supplies only Foldable.

```cpp
#include <beman/tree_algorithms/tree_typeclasses.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ta = beman::tree_algorithms;
namespace bt = beman::transpose;

int main() {
    const auto tree = ta::rose<int>(1, {ta::rose<int>(2), ta::rose<int>(3)});
    const auto labels = bt::fmap([](const int& n) { return std::to_string(n); }, tree);
    const auto checked = bt::traverse(
        [](const int& n) -> std::optional<int> {
            if (n < 0) return std::nullopt;
            return n * 10 + 1;
        }, tree);
    return bt::to_vector(labels) == std::vector<std::string>{"1", "2", "3"}
        && checked && checked->inner.value == 11 ? 0 : 1;
}
```

Traversal records O(n) auxiliary shape information and payload positions,
then delegates one ordered sequencing pass to transpose's vector Traversable.
It prefers the selected Applicative's native `collect`, otherwise uses its
pairwise composition. Policies whose pure and composed carriers differ work
through native collection. Payload effects are composed once, in the table's
order. The mapper is called synchronously, including for deferred effects;
any payload data a deferred effect needs must be owned by that effect.

The reconstruction closure owns the recorded shape, so it can run after the
source and mapper have been destroyed. It does not capture source pointers.
`transpose` is identity traversal. Derived Traversable operations come from
transpose's existing derivation.

## Folding

`tree_algorithms::fold_map(function, tree, monoid)` is the monoid-object
bridge in the lookup tier. Foldable delegates to it, and it delegates to the
existing structural fold with the same mapper and monoid instances. The
four-argument explicit-result spelling and the mechanism-free explicit tier
remain available.

The existing structural projections and layer maps copy payloads, so this
bridge and its Foldable registrations require copy-constructible payloads.
Result carriers must be copyable because the layer folds copy and assign
them. Mapping and traversal support move-only sources independently of that
Foldable limitation. Full object concepts are tested with copyable payloads;
per-operation constraints describe the move-only cases.

The monoid's `identity()` and `combine()` determine the result algebra;
Foldable never selects a different monoid when one is passed. The derived
left/right folds, collection, length, predicates, and search operations reuse
transpose's Foldable derivation. Fold results follow the table's order;
ordinary mapper side effects follow the structural fold's evaluation, which
can differ from payload traversal order.

The independently ported BinaryTree in transpose's examples retains its own
type and shape-aware Applicative example. Tests compare the two types'
payload and effect order. No library-tree Applicative instance is implied.
