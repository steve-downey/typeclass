// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose.hpp>
#include <beman/tree_algorithms.hpp>
namespace bt = beman::transpose;
namespace ta = beman::tree_algorithms;
struct Unregistered {};
template <class A>
struct BareLayer {
    int v;
};
template <class T>
concept transpose_fold_map = requires(const T& u) { bt::fold_map([](int) { return 0; }, u, bt::sum_monoid<int>{}); };
template <class T>
concept transpose_fmap = requires(const T& u) { bt::fmap([](int) { return 0; }, u); };
template <class T>
concept bridge_fold_map = requires(const T& u) { ta::fold_map([](int) { return 0; }, u, bt::sum_monoid<int>{}); };
template <class T>
concept lookup_fold_map =
    requires(const T& u) { ta::fold_map<int>([](int) { return 0; }, [](int a, int b) { return a + b; }, 0, u); };
template <class T>
concept lookup_fold_fix = requires(const T& t) { ta::fold_fix<int>([](const BareLayer<int> & l) { return l.v; }, t); };
// transpose's operation objects and the monoid bridge: unregistered is a soft "no".
static_assert(!transpose_fold_map<Unregistered>);
static_assert(!transpose_fmap<Unregistered>);
static_assert(!bridge_fold_map<Unregistered>);
#if PROBE == 1
static_assert(!lookup_fold_map<Unregistered>);
#elif PROBE == 2
static_assert(!lookup_fold_fix<ta::Fix<BareLayer> >);
#endif
int main() {}
