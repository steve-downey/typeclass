// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Which routes connect a FingerTree to the tree algorithms.
#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>
#include <beman/fingertree.hpp>
#include <cstdio>
#include <optional>
namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
template <class T>
concept bridge_fold_map = requires(const T& t) { ta::fold_map([](int x) { return x; }, t, bt::sum_monoid<int>{}); };
template <class T>
concept transpose_fold_map = requires(const T& t) { bt::fold_map([](int x) { return x; }, t, bt::sum_monoid<int>{}); };
template <class T>
concept transpose_traverse = requires(const T& t) { bt::traverse([](int x) { return std::optional<int>{x}; }, t); };
#define SHOW(S)                                                                                               \
    std::printf("%-22s project=%d  ta::fold_map bridge=%d  transpose::fold_map=%d  transpose::traverse=%d\n", \
                #S,                                                                                           \
                bt::has_instance_v<decltype(ta::project<S>)>,                                                 \
                bridge_fold_map<S>,                                                                           \
                transpose_fold_map<S>,                                                                        \
                transpose_traverse<S>)
int main() {
    SHOW(ta::BinaryTree<int>);
    SHOW(ta::FringeTree<int>);
    SHOW(ft::FingerTree<int>);
}
