// installtest/fingertree/test.cpp                                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree.hpp>

#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <optional>
#include <string>
#include <vector>

int main() {
    namespace ft = beman::fingertree::tree;
    namespace bt = beman::transpose;

    ft::FingerTree<int> seq;
    for (int i = 0; i < 10; ++i)
        seq = seq.snoc(i);

    // The element whose prefix measure first reaches the threshold leads
    // d_right, so a threshold of 5 leaves four elements on the left.
    const auto split = seq.split_at_measure(std::size_t{5});
    if (seq.measure() != 10 || split.d_left.measure() != 4 || split.d_right.measure() != 6) {
        std::cerr << "expected 10 = 4 + 6, got " << seq.measure() << " = " << split.d_left.measure() << " + "
                  << split.d_right.measure() << '\n';
        return 1;
    }
    std::cout << "split: " << split.d_left.measure() << " + " << split.d_right.measure() << '\n';

    // Exercise the installed reconstruction and monoid headers through the
    // umbrella, including configuration retained across a payload change.
    std::pmr::monotonic_buffer_resource resource;
    const auto                          configured = ft::pmr::FingerTree<int>::from_sequence({2, 3, 7}, &resource);
    const auto mapped    = bt::fmap([](const int& value) { return std::to_string(value); }, configured);
    const auto traversed = bt::traverse([](const int& value) { return std::optional{value * 10 + 1}; }, configured);
    const beman::fingertree::finger_tree_concat_monoid<ft::pmr::FingerTree<std::string>> configured_concat(mapped);
    const auto doubled   = configured_concat.combine(mapped, mapped);
    const auto canonical = bt::monoid<ft::FingerTree<int>>.combine(split.d_left, split.d_right);
    if (mapped.get_allocator().resource() != &resource ||
        mapped.flatten() != std::vector<std::string>{"2", "3", "7"} || !traversed ||
        traversed->get_allocator().resource() != &resource || traversed->flatten() != std::vector<int>{21, 31, 71} ||
        doubled.measure() != 6 || canonical.flatten() != seq.flatten()) {
        std::cerr << "installed configured reconstruction or concatenation failed\n";
        return 1;
    }
    return 0;
}
