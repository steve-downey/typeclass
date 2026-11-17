// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// A finger tree seen by the recursive tree algorithms.
//
// beman.fingertree hides its shape behind a sequence interface, and
// beman.tree_algorithms knows nothing about finger trees. The optional
// adapter header is what joins them: it describes one layer of the tree's
// real representation, and from that one description every structural verb
// follows.

#include <beman/fingertree.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/string.hpp>
#include <beman/tree_algorithms.hpp>
#include <beman/typeclass_tree_adapters/finger_tree.hpp>

#include <iostream>
#include <string>
#include <variant>

namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
namespace tt = beman::typeclass_tree_adapters;

using Tree = ft::FingerTree<int>;
using Part = tt::FingerTreePart<Tree>;

// An algebra over one layer: what to make of a node whose children have
// already been turned into strings. It is not recursive; the fold is.
auto draw(const tt::FingerTreeF<tt::FingerPayloadRef<int>, std::string>& layer) -> std::string {
    return std::visit(
        bt::overloaded{
            [](const tt::FingerEmpty&) -> std::string { return "()"; },
            [](const tt::FingerSingle<std::string>& s) -> std::string { return "(" + *s.element + ")"; },
            [](const tt::FingerDeep<std::string>& d) -> std::string {
                std::string out = "[";
                for (const auto& position : d.left)
                    out += *position + " ";
                out += "| " + (d.spine ? *d.spine : std::string("-")) + " |";
                for (const auto& position : d.right)
                    out += " " + *position;
                return out + "]";
            },
            [](const tt::FingerLeaf<tt::FingerPayloadRef<int>>& l) -> std::string { return std::to_string(*l.value); },
            [](const tt::FingerNode2<std::string>& n) -> std::string { return "<" + *n.a + " " + *n.b + ">"; },
            [](const tt::FingerNode3<std::string>& n) -> std::string {
                return "<" + *n.a + " " + *n.b + " " + *n.c + ">";
            },
        },
        layer);
}

int main() {
    Tree tree;
    for (int i = 0; i < 12; ++i)
        tree = tree.snoc(i);

    // A structural fold: the tree's real shape, digits, spine and nodes.
    const auto shape = ta::fold_with<std::string>(draw, bt::fmap, tt::finger_tree_project, &tree);
    std::cout << "shape:    " << shape << '\n';

    // An elementwise fold through lookup: the adapter registered the
    // projection, the layer fold and the layer's Functor.
    const auto comma  = [](const int& value) { return std::to_string(value) + ","; };
    const auto concat = [](const std::string& a, const std::string& b) { return a + b; };
    const auto listed = ta::fold_map<std::string>(comma, concat, std::string{}, tree);
    std::cout << "elements: " << listed << '\n';

    // It agrees with the Foldable fingertree registers for itself, which
    // never looks at the shape.
    const auto native = bt::fold_map(comma, tree, bt::monoid<std::string>);

    // Take the tree apart layer by layer and put it back together. The
    // embedding rebuilds every node and computes every measure again.
    const auto rebuilt =
        ta::unfold_with<Part>(tt::finger_tree_project, bt::fmap, tt::FingerTreeEmbed<Tree>(tree), Part(tree))
            .sequence();
    std::cout << "rebuilt:  " << (rebuilt == tree ? "equal" : "DIFFERENT") << ", measure " << rebuilt.measure()
              << '\n';

    const bool ok = shape == "[0 | [<1 2 3> | - | <4 5 6> <7 8 9>] | 10 11]" && listed == native && rebuilt == tree &&
                    rebuilt.measure() == tree.measure();
    return ok ? 0 : 1;
}
