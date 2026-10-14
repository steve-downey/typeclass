// installtest/fingertree/test.cpp                                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree/fingertree.hpp>

#include <cstddef>
#include <iostream>

int main() {
    namespace ft = beman::fingertree::tree;

    ft::FingerTree<int> seq;
    for (int i = 0; i < 10; ++i)
        seq = seq.snoc(i);

    const auto split = seq.split_at_measure(std::size_t{4});
    if (seq.measure() != 10 || split.d_left.measure() != 4 || split.d_right.measure() != 6) {
        std::cerr << "expected 10 = 4 + 6, got " << seq.measure() << " = " << split.d_left.measure() << " + "
                  << split.d_right.measure() << '\n';
        return 1;
    }
    std::cout << "split: " << split.d_left.measure() << " + " << split.d_right.measure() << '\n';
    return 0;
}
