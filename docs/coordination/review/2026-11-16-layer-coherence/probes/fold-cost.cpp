// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Cost of the derived Foldable queries against the container's own members.
#include <beman/transpose.hpp>
#include <beman/fingertree.hpp>
#include <chrono>
#include <cstdio>
#include <vector>
namespace bt = beman::transpose;
namespace ft = beman::fingertree::tree;
int main() {
    std::vector<int> values(2'000'000, 1);
    auto             tree = ft::FingerTree<int>::from_sequence(std::move(values));
    auto             time = [](auto&& f) {
        auto          t0 = std::chrono::steady_clock::now();
        volatile auto r  = f();
        (void)r;
        return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
    };
    std::printf("n=2e6: tree.empty() %.1fus  transpose::empty(tree) %.1fus\n",
                time([&] { return tree.empty(); }),
                time([&] { return bt::empty(tree); }));
    std::printf("n=2e6: tree.size()  %.1fus  transpose::length(tree) %.1fus\n",
                time([&] { return tree.size(); }),
                time([&] { return bt::length(tree); }));
    std::vector<int> vec(2'000'000, 1);
    std::printf("n=2e6: vec.size()   %.1fus  transpose::length(vec)  %.1fus  transpose::empty(vec) %.1fus\n",
                time([&] { return vec.size(); }),
                time([&] { return bt::length(vec); }),
                time([&] { return bt::empty(vec); }));
}
