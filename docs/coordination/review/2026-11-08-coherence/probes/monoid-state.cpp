// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree.hpp>
#include <beman/transpose.hpp>
#include <iostream>
#include <memory_resource>
namespace bt = beman::transpose;
namespace ft = beman::fingertree::tree;
struct ModSum {
    std::size_t modulus = 100;
    std::size_t identity() const { return 0; }
    std::size_t combine(std::size_t a, std::size_t b) const { return (a + b) % modulus; }
};
struct Weight {
    std::size_t operator()(int x) const { return x; }
};
int main() {
    using Alloc = std::pmr::polymorphic_allocator<std::byte>;
    using Tree  = ft::FingerTree<int, ModSum, Weight, Alloc>;
    std::pmr::monotonic_buffer_resource a, b;
    auto                                left   = Tree::from_sequence({1}, Alloc{&a}, ModSum{5});
    auto                                right  = Tree::from_sequence({4, 4}, Alloc{&b}, ModSum{5});
    auto                                joined = left.append(right);
    std::cout << "append: expected_modulus=5 actual=" << joined.get_monoid().modulus
              << " expected_measure=4 actual=" << joined.measure() << '\n';
}
