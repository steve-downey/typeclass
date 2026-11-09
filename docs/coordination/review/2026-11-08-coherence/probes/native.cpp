// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/sequence.hpp>
#include <beman/fingertree.hpp>
#include <iostream>

namespace bt = beman::transpose;
namespace ft = beman::fingertree::tree;
template <class T>
struct boxed {
    using value_type = T;
    T held;
};
template <class T>
struct labelled {
    using value_type = T;
    T held;
};
struct AppImpl {
    inline static int collects = 0;
    template <class V>
    auto pure(this auto&&, V&& v) {
        return boxed<std::remove_cvref_t<V>>{std::forward<V>(v)};
    }
    template <class F, class... C>
    auto invoke(this auto&&, F&& f, C&&... c) {
#ifdef WIDEN
        return labelled<std::remove_cvref_t<decltype(f(std::forward<C>(c).held...))>>{f(std::forward<C>(c).held...)};
#else
        return boxed<std::remove_cvref_t<decltype(f(std::forward<C>(c).held...))>>{f(std::forward<C>(c).held...)};
#endif
    }
    template <class T>
    auto collect(this auto&& self,
#ifdef WIDEN
                 std::vector<labelled<T>> contexts
#else
                 std::vector<boxed<T>> contexts
#endif
    ) {
        ++collects;
        std::vector<T> values;
        for (auto& c : contexts)
            values.push_back(std::move(c.held));
#ifdef WIDEN
        return labelled<std::vector<T>>{std::move(values)};
#else
        return boxed<std::vector<T>>{std::move(values)};
#endif
    }
};
struct App : bt::derive_applicative<AppImpl> {
    using AppImpl::collect;
    using AppImpl::invoke;
    using AppImpl::pure;
};
#ifdef WIDEN
template <class T>
using Effect = labelled<T>;
#else
template <class T>
using Effect = boxed<T>;
#endif
static_assert(bt::applicative_object_for<App, Effect<int>>);
int main() {
    int& count = AppImpl::collects;
    App  app{};
    auto wrap = [](const auto& x) { return Effect<std::remove_cvref_t<decltype(x)>>{x}; };
    auto v    = bt::traversable<std::vector<int>>.traverse(app, wrap, std::vector<int>{1, 2, 3});
    std::cout << "vector collect=" << count << '\n';
    count = 0;
#ifndef STRUCTURE
    #define STRUCTURE 0
#endif
#if STRUCTURE == 0
    auto tree = ft::FingerTree<int>::from_sequence({1, 2, 3});
#elif STRUCTURE == 1
    auto tree = ft::FingerTreeRope<>::from_chunks({"a", "b"});
#elif STRUCTURE == 2
    auto tree = ft::FingerTreeRandomAccess<int>::from_sequence({1, 2, 3});
#elif STRUCTURE == 3
    auto tree = ft::FingerTreePriorityQueue<int>::from_values({1, 2, 3});
#elif STRUCTURE == 4
    auto tree = ft::FingerTreeIntervalIndex<int>::from_intervals({{0, 1, 1}, {1, 3, 2}});
#endif
    auto t = bt::traversable<decltype(tree)>.traverse(app, wrap, tree);
    std::cout << "fingertree collect=" << count << '\n';
#ifndef WIDEN
    count     = 0;
    auto rope = ft::FingerTreeRope<>::from_chunks({"a", "b"});
    auto r    = bt::traversable<decltype(rope)>.traverse(app, wrap, rope);
    std::cout << "rope collect=" << count << '\n';
    count   = 0;
    auto ra = ft::FingerTreeRandomAccess<int>::from_sequence({1, 2, 3});
    auto a  = bt::traversable<decltype(ra)>.traverse(app, wrap, ra);
    std::cout << "random_access collect=" << count << '\n';
    count   = 0;
    auto pq = ft::FingerTreePriorityQueue<int>::from_values({1, 2, 3});
    auto p  = bt::traversable<decltype(pq)>.traverse(app, wrap, pq);
    std::cout << "priority_queue collect=" << count << '\n';
    count      = 0;
    auto index = ft::FingerTreeIntervalIndex<int>::from_intervals({{0, 1, 1}, {1, 3, 2}});
    auto i     = bt::traversable<decltype(index)>.traverse(app, wrap, index);
    std::cout << "interval_index collect=" << count << '\n';
#endif
}
