// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Compile-time regression probe for the structural fingertree adapter.
//
// A finger tree's spine is as deep as the tree is large. If a layer, a
// handle or a piece changed type with spine depth, a recursive structural
// algorithm would need one instantiation per level, and a tree whose size is
// known only at run time would need an unbounded number of them: this file
// would not compile.
//
// It compiles because FingerTreePart and FingerTreeF are the same types at
// every level. One use of each verb over one tree type produces a fixed set
// of instantiations however large the tree is; the functions below take the
// size at run time to make that the only possible reading.
//
// Counting DWARF class-type entries, as for the fingertree probe:
//
//   readelf --debug-dump=info <this object file> | grep -c DW_TAG_class_type
//
// A change that makes the count grow with the constants below has
// introduced a depth-typed template.

#include <beman/typeclass_tree_adapters/finger_tree.hpp>

#include <cstddef>
#include <functional>

namespace {

namespace ta = beman::tree_algorithms;
namespace tt = beman::typeclass_tree_adapters;

using Tree = beman::fingertree::tree::FingerTree<int>;
using Part = tt::FingerTreePart<Tree>;

auto grow(std::size_t n) -> Tree {
    Tree tree;
    for (std::size_t i = 0; i < n; ++i)
        tree = tree.snoc(static_cast<int>(i));
    return tree;
}

} // namespace

auto finger_tree_adapter_probe_fold(std::size_t n) -> int {
    const auto tree = grow(n);
    return ta::fold_map<int>([](int x) { return x; }, std::plus<int>{}, 0, tree);
}

auto finger_tree_adapter_probe_round_trip(std::size_t n) -> std::size_t {
    const auto tree = grow(n);
    const auto rebuilt =
        ta::unfold_with<Part>(
            tt::finger_tree_project,
            [](auto&& fn, const auto& layer) { return ta::layer_fmap(std::forward<decltype(fn)>(fn), layer); },
            tt::FingerTreeEmbed<Tree>(tree),
            Part(tree))
            .sequence();
    return rebuilt.size();
}

auto finger_tree_adapter_probe_fix(std::size_t n) -> std::size_t {
    const auto tree  = grow(n);
    const auto fixed = tt::to_fix(tree);
    return tt::from_fix(fixed, tt::FingerTreeEmbed<Tree>(tree)).size();
}
