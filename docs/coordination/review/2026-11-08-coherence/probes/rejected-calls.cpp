// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/tree_algorithms.hpp>
#include <beman/fingertree.hpp>
#include <beman/transpose.hpp>
#include <iostream>

namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
struct NotCallable {};
struct LvalueOnly {
    int operator()(const int& x) & { return x + 1; }
    int operator()(const int&) && = delete;
};
struct RvalueOnly {
    int operator()(const int&) & = delete;
    int operator()(const int& x) && { return x + 1; }
};
struct BadMonoid {};

#ifndef PROBE
    #define PROBE 0
#endif
#ifndef LAYER
    #define LAYER 0
#endif
#if LAYER == 0
using Layer = ta::BinaryTreeF<int, int>;
#elif LAYER == 1
using Layer = ta::FringeTreeF<int, int>;
#elif LAYER == 2
using Layer = ta::RoseF<int, int>;
#elif LAYER == 3
using Layer = ta::ExprF<int>;
#elif LAYER == 4
using Layer = ta::NatF<int>;
#endif

template <class F, class T>
concept Mappable = requires(F f, const T& t) { bt::fmap(std::move(f), t); };

template <class T, class F, class M>
concept FoldableWith = requires(F f, const T& t, M m) { bt::fold_map(f, t, m); };

template <class T, class F>
concept TraversableWith = requires(F f, const T& t) { bt::traverse(f, t); };

int main() {
#if PROBE == 0
    static_assert(bt::functor_object<std::remove_cvref_t<decltype(bt::functor<Layer>)>, Layer>);
#elif PROBE == 1
    static_assert(!Mappable<NotCallable, Layer>);
#elif PROBE == 2
    auto mapped = bt::fmap(LvalueOnly{}, Layer{});
#elif PROBE == 3
    static_assert(!Mappable<RvalueOnly, Layer>);
#elif PROBE == 4
    auto show = [](int x) { return x; };
    static_assert(!FoldableWith<ft::FingerTree<int>, decltype(show), BadMonoid>);
#elif PROBE == 5
    auto rope   = ft::FingerTreeRope<>::from_text("abc");
    auto result = bt::traverse([](const std::string&) { return std::optional{42}; }, rope);
#elif PROBE == 6
    auto tree   = ft::FingerTree<int>::from_sequence({1, 2, 3});
    auto result = bt::traverse([](int x) { return std::optional{std::make_unique<int>(x)}; }, tree);
#elif PROBE == 7
    auto tree   = ft::FingerTree<std::unique_ptr<int> >{}.snoc(std::make_unique<int>(1));
    auto result = bt::traverse([](const std::unique_ptr<int>& x) { return std::optional{*x}; }, tree);
#elif PROBE == 8
    auto tree   = ft::FingerTree<int>::from_sequence({1, 2, 3});
    auto result = bt::traverse([](int) { return std::expected<void, std::errc>{}; }, tree);
    static_assert(std::same_as<decltype(result), std::expected<ft::FingerTree<std::monostate>, std::errc> >);
#elif PROBE == 9
    auto rope   = ft::FingerTreeRope<>::from_text("abc");
    auto result = bt::traverse([](const std::string&) { return std::expected<void, std::errc>{}; }, rope);
#elif PROBE == 10
    auto effect = [](int x) { return std::expected<int, bt::error_set<> >{x}; };
    static_assert(!TraversableWith<ft::FingerTree<int>, decltype(effect)>);
#elif PROBE == 11
    struct Error {};
    using Grade = bt::error_set<Error>;
    auto tree   = ft::FingerTree<int>::from_sequence({1, 2, 3});
    auto result = bt::traverse([](int x) { return std::expected<int, Grade>{x}; }, tree);
    static_assert(std::same_as<decltype(result), std::expected<ft::FingerTree<int>, Grade> >);
#endif
}
