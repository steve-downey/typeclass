// installtest/tree_algorithms/test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Both tiers through the installed package: the explicit tier (fold_fix with
// the layer's fmap passed in) and the lookup tier (eval, which resolves the
// layer's Functor instance through beman.transpose).

#include <beman/tree_algorithms/expression.hpp>
#include <beman/tree_algorithms/recursion_schemes.hpp>
#include <beman/tree_algorithms/recursion_schemes_lookup.hpp>
#include <beman/transpose/functor.hpp>

#include <iostream>

int main() {
    namespace ta = beman::tree_algorithms;

    // 2 + 3 * 4
    const ta::Expr expr =
        ta::add_node(ta::const_node(2), ta::mul_node(ta::const_node(3), ta::const_node(4)));

    const int looked_up = ta::eval(expr);
    const int explicit_fmap = ta::fold_fix<int>(ta::eval_algebra, beman::transpose::fmap, expr);

    if (looked_up != 14 || explicit_fmap != 14) {
        std::cerr << "expected 14, got " << looked_up << " and " << explicit_fmap << '\n';
        return 1;
    }
    std::cout << "eval: " << looked_up << '\n';
    return 0;
}
