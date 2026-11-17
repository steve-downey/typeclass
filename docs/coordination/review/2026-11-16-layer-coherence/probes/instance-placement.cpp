// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/fold.hpp>
#if WHICH == 0
    #include <beman/tree_algorithms/binary_tree.hpp>
using Tree = beman::tree_algorithms::BinaryTree<int>;
#else
    #include <beman/fingertree/tree/finger_tree.hpp>
using Tree = beman::fingertree::tree::FingerTree<int>;
#endif
namespace bt = beman::transpose;
// The type's own header is included; is its Foldable instance visible?
constexpr bool before = bt::has_instance_v<decltype(bt::foldable<Tree>)>;
#ifdef LATE
    #if WHICH == 0
        #include <beman/tree_algorithms/tree_typeclasses.hpp>
    #else
        #include <beman/fingertree/tree/finger_tree_foldable.hpp>
    #endif
constexpr bool after = bt::has_instance_v<decltype(bt::foldable<Tree>)>;
    #include <cstdio>
int main() { std::printf("before=%d after=%d\n", before, after); }
#else
static_assert(!before, "instance is NOT visible from the type's header");
int main() {}
#endif
