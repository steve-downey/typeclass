// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>
using namespace beman::transpose;
using namespace beman::tree_algorithms;
int main() {
    auto t = BinaryTree<int>::leaf(1);
    return fold_map([](int x) { return x; }, t, sum_monoid<int>{});
}
