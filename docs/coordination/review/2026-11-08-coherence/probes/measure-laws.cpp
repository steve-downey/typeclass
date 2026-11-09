// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree.hpp>
#include <beman/transpose/string.hpp>
#include <cassert>
#include <iostream>

namespace bt = beman::transpose;
namespace ft = beman::fingertree::tree;
struct Digits {
    std::string operator()(int x) const { return std::to_string(x) + ","; }
};
int main() {
    using Tree = ft::FingerTree<int, bt::Monoid<std::string>, Digits>;
    std::vector<int> values;
    for (int i = 0; i < 100; ++i)
        values.push_back(i);
    auto t = Tree::from_sequence(values);
    assert(bt::fold_map(Digits{}, t, t.get_monoid()) == t.measure());
    auto r = t.reversed();
    assert(r.measure() == t.measure());
    assert(bt::fold_map(Digits{}, r, r.get_monoid()) == r.measure());
    auto split  = t.split_at([size = std::size_t{53}](const std::string& s) { return s.size() > size; });
    auto joined = split.d_left.append(split.d_right);
    assert(joined.measure() == t.measure());
    assert(joined.flatten() == t.flatten());
    std::cout << "noncommutative measure, dual reversal, split/append laws pass\n";
}
