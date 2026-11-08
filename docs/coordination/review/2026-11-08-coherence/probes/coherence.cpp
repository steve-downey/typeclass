// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>
#include <beman/fingertree.hpp>
#include <iostream>
#include <memory_resource>
#include <type_traits>

namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;

struct Weight {
    auto operator()(int x) const -> std::size_t { return x; }
};

template <class T>
void matrix(const char* name) {
    using Fold = std::remove_cvref_t<decltype(bt::foldable<T>)>;
    using Trav = std::remove_cvref_t<decltype(bt::traversable<T>)>;
    std::cout << name << " value_trait=" << requires { typename bt::applicative_value<T>::type; } << " functor_object="
              << bt::functor_object<std::remove_cvref_t<decltype(bt::functor<T>)>, T> << " functor="
              << bt::has_instance_v<decltype(bt::functor<T>)> << " foldable="
              << bt::has_instance_v<decltype(bt::foldable<T>)> << " foldable_object="
              << bt::foldable_object<Fold, T> << " traversable="
              << bt::has_instance_v<decltype(bt::traversable<T>)> << " traversable_object="
              << bt::traversable_object<Trav, T> << " project="
              << bt::has_instance_v<decltype(ta::project<T>)> << '\n';
}

int main() {
    matrix<ta::BinaryTree<int>>("BinaryTree");
    matrix<ta::BinaryTreeF<int, int>>("BinaryTreeF");
    matrix<ta::FringeTreeF<int, int>>("FringeTreeF");
    matrix<ta::RoseF<int, int>>("RoseF");
    matrix<ta::ExprF<int>>("ExprF");
    matrix<ta::NatF<int>>("NatF");
    matrix<ta::FringeTree<int>>("FringeTree");
    matrix<ta::RoseTreeFix<int>>("RoseTreeFix");
    matrix<ft::FingerTree<int>>("FingerTree");
    matrix<ft::FingerTreeRandomAccess<int>>("RandomAccess");
    matrix<ft::FingerTreePriorityQueue<int>>("PriorityQueue");
    matrix<ft::FingerTreeRope<>>("Rope");
    matrix<ft::FingerTreeIntervalIndex<int>>("IntervalIndex");

    using Weighted = ft::FingerTree<int, bt::sum_monoid<std::size_t>, Weight>;
    auto weighted  = Weighted::from_sequence({2, 3, 7});
    auto same      = bt::traverse([](int x) { return std::optional{x}; }, weighted);
    std::cout << "weighted traverse same_type="
              << std::is_same_v<typename decltype(same)::value_type, Weighted> << " before_measure="
              << weighted.measure() << " after_measure=" << same->measure() << '\n';

    using Alloc = std::pmr::polymorphic_allocator<std::byte>;
    using Pmr   = ft::FingerTree<int, bt::sum_monoid<std::size_t>, ft::UnitMeasure<int, std::size_t>, Alloc>;
    std::pmr::monotonic_buffer_resource resource;
    auto                                pmr    = Pmr::from_sequence({1, 2, 3}, Alloc{&resource});
    auto                                result = bt::traverse([](int x) { return std::optional{x}; }, pmr);
    std::cout << "pmr traverse same_type=" << std::is_same_v<typename decltype(result)::value_type, Pmr> << '\n';

    auto        binary = ta::BinaryTree<int>::node(2, ta::BinaryTree<int>::leaf(1), ta::BinaryTree<int>::leaf(3));
    std::string visits;
    auto        folded = ta::fold_map<std::string>(
        [&](int x) {
            visits += char('0' + x);
            return std::to_string(x);
        },
        [](auto a, auto b) { return a + b; },
        std::string{},
        binary);
    std::cout << "binary result=" << folded << " mapper_visits=" << visits << '\n';
    auto rose = ta::rose<int>(1, {ta::rose<int>(2), ta::rose<int>(3)});
    visits.clear();
    folded = ta::fold_map<std::string>(
        [&](int x) {
            visits += char('0' + x);
            return std::to_string(x);
        },
        [](auto a, auto b) { return a + b; },
        std::string{},
        rose);
    std::cout << "rose result=" << folded << " mapper_visits=" << visits << '\n';
}
