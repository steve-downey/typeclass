# Fingertree typeclass interfaces

The instances use the Functor, Applicative, Foldable, and Traversable machinery
from beman.transpose. Include the corresponding consumer header before the
first lookup. Persistent inputs present their payloads by const reference,
including when the tree handle is an rvalue.

| Representation | Element | Foldable | Generic Traversable |
|---|---|---|---|
| FingerTree | T | finger_tree_foldable.hpp | finger_tree_traversable.hpp |
| FingerTreeRandomAccess | T | wrapper header | wrapper header |
| FingerTreePriorityQueue | T | wrapper header | wrapper header |
| FingerTreeIntervalIndex | payload T | wrapper header | wrapper header |
| FingerTreeRope | string chunk | wrapper header | traverse chunks explicitly |

Folds and traversals use left-to-right sequence order. Interval-index folds
visit payloads; traversal retains each interval's start and end. Priority
queue traversal follows the stored sequence, rather than priority order.
The result monoid passed to `fold_map` is independent of a tree's measure
monoid and must support `identity()` and `combine()` for the mapped result.

The complete Foldable object includes copying operations such as `to_vector`
and `find_first`. These require copyable payloads. A base FingerTree containing
move-only values can still support reference-reading operations such as
`fold_map` and `length`; it need not satisfy the complete object concept.

Current generic traversal rebuilds a default-configured result tree.
Configuration-preserving reconstruction for custom measures and allocators
is a separate implementation stage.

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
