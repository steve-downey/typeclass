// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// A policy whose native collect returns a carrier its own invoke does not map over.
#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>
#include <beman/fingertree.hpp>
#include <cstdio>
namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
namespace ft = beman::fingertree::tree;
template <class T>
struct eff {
    using value_type = T;
    T held;
};
template <class T>
struct done {
    using value_type = T;
    T held;
};
template <class C>
inline constexpr bool is_eff = false;
template <class T>
inline constexpr bool is_eff<eff<T>> = true;
struct AppImpl {
    template <class V>
    auto pure(this auto&&, V&& v) {
        return eff<std::remove_cvref_t<V>>{std::forward<V>(v)};
    }
    template <class F, class... C>
        requires(is_eff<std::remove_cvref_t<C>> && ...) &&
                std::invocable<F&, typename std::remove_cvref_t<C>::value_type...>
    auto invoke(this auto&&, F&& f, C&&... c) {
        return eff<std::remove_cvref_t<std::invoke_result_t<F&, typename std::remove_cvref_t<C>::value_type...>>>{
            f(std::forward<C>(c).held...)};
    }
    template <class T>
    auto collect(this auto&&, std::vector<eff<T>> contexts) {
        std::vector<T> values;
        for (auto& c : contexts)
            values.push_back(std::move(c.held));
        return done<std::vector<T>>{std::move(values)};
    }
};
struct App : bt::derive_applicative<AppImpl> {
    using AppImpl::collect;
    using AppImpl::invoke;
    using AppImpl::pure;
};
static_assert(bt::applicative_object_for<App, eff<int>>);
inline constexpr auto wrap = [](const int& x) { return eff<int>{x}; };
template <class S>
concept traversable_with_app = requires(const S& s) { bt::traverse(wrap, s, App{}); };
int main() {
    // The reference instance: fine, returns the collected carrier.
    auto v = bt::traverse(wrap, std::vector<int>{1, 2, 3}, App{});
    static_assert(std::same_as<decltype(v), done<std::vector<int>>>);
    std::printf("vector: ok, %zu collected\n", v.held.size());
#if WHICH_S == 1
    std::printf("FingerTree admitted by constraint: %d\n", traversable_with_app<ft::FingerTree<int>>);
#elif WHICH_S == 2
    std::printf("BinaryTree admitted by constraint: %d\n", traversable_with_app<ta::BinaryTree<int>>);
#endif
}
