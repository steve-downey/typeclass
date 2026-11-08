// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose.hpp>
#include "examples/transpose/binary_tree.hpp"
#include <iostream>

namespace bt = beman::transpose;
template <class T>
struct Writer {
    using value_type = T;
    std::string log;
    T           held;
};
struct WriterImpl {
    template <class V>
    auto pure(this auto&&, V&& v) {
        return Writer<std::remove_cvref_t<V>>{"", std::forward<V>(v)};
    }
    template <class F, class... C>
    auto invoke(this auto&&, F&& f, C&&... c) {
        return Writer<std::remove_cvref_t<decltype(f(std::forward<C>(c).held...))>>{(std::string{} + ... + c.log),
                                                                                    f(std::forward<C>(c).held...)};
    }
};
struct WriterMap : bt::derive_applicative<WriterImpl> {
    using WriterImpl::invoke;
    using WriterImpl::pure;
};
int main() {
    using Tree  = example::BinaryTree<int>;
    auto full   = Tree::node(2, Tree::leaf(1), Tree::leaf(3));
    auto unary  = Tree::from_children_ptrs(1, {}, Tree::make_ptr(Tree::leaf(2)));
    auto effect = [](int x) { return Writer<int>{std::to_string(x), x}; };
    auto show   = [](int x) { return std::to_string(x); };
    for (auto& t : {full, unary}) {
        auto fold   = bt::fold_map(show, t);
        auto result = bt::traversable<Tree>.traverse(WriterMap{}, effect, t);
        std::cout << "fold_map=" << fold << " traverse_effects=" << result.log << '\n';
    }
}
