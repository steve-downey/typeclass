// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>
#include <beman/fingertree.hpp>
#include <cstdio>
#include <memory>
namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
struct Counted {
    inline static int copies = 0;
    int               v;
    Counted(int x) : v(x) {}
    Counted(const Counted& o) : v(o.v) { ++copies; }
    Counted(Counted&&)                 = default;
    Counted& operator=(const Counted&) = default;
    Counted& operator=(Counted&&)      = default;
};
using P = std::unique_ptr<int>;
template <class S>
concept has_length = requires(const S& s) { bt::length(s); };
template <class S>
concept has_any_of = requires(const S& s) { bt::any_of(s, [](const P&) { return true; }); };
template <class S>
concept has_fmap = requires(const S& s) { bt::fmap([](const P & p) { return *p; }, s); };
template <class S>
concept has_traverse = requires(const S& s) { bt::traverse([](const P & p) { return std::optional<int>{*p}; }, s); };
#define SHOW(S)                                                    \
    std::printf("%-28s length=%d any_of=%d fmap=%d traverse=%d\n", \
                #S,                                                \
                has_length<S>,                                     \
                has_any_of<S>,                                     \
                has_fmap<S>,                                       \
                has_traverse<S>)
int main() {
    std::puts("-- move-only payload: which typeclass operations are available");
    SHOW(std::vector<P>);
    SHOW(ft::FingerTree<P>);
    SHOW(ta::BinaryTree<P>);
    SHOW(ta::BinaryTreeFix<P>);
    SHOW(ta::FringeTree<P>);
    SHOW(ta::RoseTreeFix<P>);
    std::puts("-- payload copies made by a read-only length() over 3 elements");
    using T         = ta::BinaryTree<Counted>;
    auto t          = T::node(Counted{2}, T::leaf(Counted{1}), T::leaf(Counted{3}));
    Counted::copies = 0;
    auto n          = bt::length(t);
    std::printf("BinaryTree    length=%zu copies=%d\n", n, Counted::copies);
    auto fx         = ta::to_fix(t);
    Counted::copies = 0;
    n               = bt::length(fx);
    std::printf("BinaryTreeFix length=%zu copies=%d\n", n, Counted::copies);
    auto fr         = ta::FringeTree<Counted>::from_sequence({Counted{1}, Counted{2}, Counted{3}});
    Counted::copies = 0;
    n               = bt::length(fr);
    std::printf("FringeTree    length=%zu copies=%d\n", n, Counted::copies);
    auto rose       = ta::rose<Counted>(Counted{1}, {ta::rose<Counted>(Counted{2}), ta::rose<Counted>(Counted{3})});
    Counted::copies = 0;
    n               = bt::length(rose);
    std::printf("RoseTreeFix   length=%zu copies=%d\n", n, Counted::copies);
    auto fg         = ft::FingerTree<Counted>{}.snoc(Counted{1}).snoc(Counted{2}).snoc(Counted{3});
    Counted::copies = 0;
    n               = bt::length(fg);
    std::printf("FingerTree    length=%zu copies=%d\n", n, Counted::copies);
    std::vector<Counted> v{Counted{1}, Counted{2}, Counted{3}};
    Counted::copies = 0;
    n               = bt::length(v);
    std::printf("vector        length=%zu copies=%d\n", n, Counted::copies);
}
