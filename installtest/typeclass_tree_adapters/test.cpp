// installtest/typeclass_tree_adapters/test.cpp                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// The optional adapter through its installed package alone. Linking the one
// target must bring in both libraries it joins and the mechanism under them:
// this file includes headers from all four and names no other package.

#include <beman/fingertree.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/string.hpp>
#include <beman/tree_algorithms.hpp>
#include <beman/typeclass_tree_adapters/finger_tree.hpp>

#include <iostream>
#include <memory_resource>
#include <string>

int main() {
    namespace bt = beman::transpose;
    namespace ta = beman::tree_algorithms;
    namespace ft = beman::fingertree::tree;
    namespace tt = beman::typeclass_tree_adapters;

    using Tree = ft::pmr::FingerTree<int>;
    using Part = tt::FingerTreePart<Tree>;

    std::pmr::monotonic_buffer_resource resource;
    Tree                                tree(&resource);
    for (int i = 0; i < 40; ++i)
        tree = tree.snoc(i);

    const auto text   = [](const int& value) { return std::to_string(value) + ","; };
    const auto concat = [](const std::string& a, const std::string& b) { return a + b; };

    // Lookup fold, the monoid bridge, and fingertree's own Foldable agree.
    const auto structural = ta::fold_map<std::string>(text, concat, std::string{}, tree);
    const auto bridged    = ta::fold_map(text, tree, bt::monoid<std::string>);
    const auto native     = bt::fold_map(text, tree, bt::monoid<std::string>);
    if (structural != native || bridged != native) {
        std::cerr << "installed structural folds disagree with the native Foldable\n";
        return 1;
    }

    // A round trip through the embedding keeps the sequence, the measure
    // and the resource; the Fix form converts and comes back.
    const tt::FingerTreeEmbed<Tree> embed(tree);
    const auto rebuilt = ta::unfold_with<Part>(tt::finger_tree_project, bt::fmap, embed, Part(tree)).sequence();
    const auto through = tt::from_fix(tt::to_fix(tree), embed);
    if (!(rebuilt == tree) || rebuilt.measure() != 40 || rebuilt.get_allocator().resource() != &resource ||
        !(through == tree)) {
        std::cerr << "installed structural round trip failed\n";
        return 1;
    }
    std::cout << "structural: " << rebuilt.measure() << " elements\n";
    return 0;
}
