// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// What the deep object concepts certify for a measured tree whose measure
// only accepts the original element type.
#include <beman/transpose.hpp>
#include <beman/fingertree.hpp>
#include <cstdio>
#include <optional>
#include <string>
namespace bt = beman::transpose;
namespace ft = beman::fingertree::tree;
struct Weight {
    auto operator()(int v) const -> std::size_t { return static_cast<std::size_t>(v); }
};
using W = ft::FingerTree<int, bt::sum_monoid<std::size_t>, Weight>;
template <class S>
concept maps_to_string = requires(const S& s) { bt::fmap([](int) { return std::string{}; }, s); };
template <class S>
concept maps_to_int = requires(const S& s) { bt::fmap([](int x) { return x + 1; }, s); };
template <class S>
concept traverses_to_string =
    requires(const S& s) { bt::traverse([](int) { return std::optional<std::string>{}; }, s); };
int main() {
    using FunctorObj = std::remove_cvref_t<decltype(bt::functor<W>)>;
    using TravObj    = std::remove_cvref_t<decltype(bt::traversable<W>)>;
    std::printf("weighted tree: functor_object=%d traversable_object=%d fmap int->int=%d fmap int->string=%d "
                "traverse->optional<string>=%d\n",
                bt::functor_object<FunctorObj, W>,
                bt::traversable_object<TravObj, W>,
                maps_to_int<W>,
                maps_to_string<W>,
                traverses_to_string<W>);
    std::printf("rope: functor registered=%d traversable registered=%d foldable registered=%d\n",
                bt::has_instance_v<decltype(bt::functor<ft::FingerTreeRope<>>)>,
                bt::has_instance_v<decltype(bt::traversable<ft::FingerTreeRope<>>)>,
                bt::has_instance_v<decltype(bt::foldable<ft::FingerTreeRope<>>)>);
}
