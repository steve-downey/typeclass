// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_ROPE_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_ROPE_HPP

#include <beman/fingertree/tree/finger_tree.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/sequence.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::fingertree::tree {

/** Measure policy that maps a string chunk to its byte length. */
struct RopeChunkMeasure {
    auto operator()(const std::string& value) const -> std::size_t { return value.size(); }
};

/** @brief Persistent text buffer implemented as a rope over string chunks.
 *
 * @tparam Tree Backing finger tree type (must store std::string with a
 *              byte-count measure: sum_monoid<std::size_t> over
 *              RopeChunkMeasure).  Default: FingerTree — correct at all sizes.
 *
 * Foldable presents string chunks. For effectful transformations, traverse
 * chunks() explicitly and rebuild string results with from_chunks(). Rope
 * does not register a generic Traversable for arbitrary result types.
 *
 * Complexity:
 * - from_text:              O(n / chunk_size)
 * - size_bytes:             O(1)
 * - to_string / chunks:     O(n)
 * - insert / erase / replace: O(log n)
 */
template <typename Tree = FingerTree<std::string, beman::transpose::sum_monoid<std::size_t>, RopeChunkMeasure>>
class FingerTreeRope {
    Tree d_tree;

    /** Splits the rope at byte position @p pos; may split a chunk in two. */
    auto split_chars(std::size_t pos) const -> std::pair<FingerTreeRope, FingerTreeRope> {
        if (pos == 0)
            return {FingerTreeRope{}, *this};
        if (pos >= size_bytes())
            return {*this, FingerTreeRope{}};

        auto split = d_tree.split([pos](std::size_t prefix) { return prefix > pos; });
        if (!split.has_value())
            return {*this, FingerTreeRope{}};

        auto left_prefix_bytes = split->d_left.measure();
        auto local             = pos - left_prefix_bytes;

        const auto& pivot = split->d_pivot;
        assert(local < pivot.size());

        auto left = split->d_left;
        if (local > 0)
            left = left.snoc(pivot.substr(0, local));

        auto right = split->d_right;
        if (local < pivot.size())
            right = right.cons(pivot.substr(local));

        return {FingerTreeRope{std::move(left)}, FingerTreeRope{std::move(right)}};
    }

    explicit FingerTreeRope(Tree tree) : d_tree(std::move(tree)) {}

  public:
    using value_type = std::string;

    /** Constructs an empty rope. */
    FingerTreeRope() : d_tree(Tree{}) {}

    /** Builds a rope from a vector of pre-formed string chunks; O(n chunks). */
    static auto from_chunks(std::vector<std::string> chunks) -> FingerTreeRope {
        return FingerTreeRope{Tree::from_sequence(std::move(chunks))};
    }

    /** Builds a rope from a text string, splitting into chunks of at most
     *  @p chunk_size bytes; O(n / chunk_size).
     */
    static auto from_text(std::string_view text, std::size_t chunk_size = 16) -> FingerTreeRope {
        std::vector<std::string> chunks;
        chunks.reserve((text.size() / chunk_size) + 1);
        for (std::size_t i = 0; i < text.size(); i += chunk_size) {
            const auto n = std::min(chunk_size, text.size() - i);
            chunks.emplace_back(text.substr(i, n));
        }
        return from_chunks(std::move(chunks));
    }

    /** Returns the total length in bytes; O(1). */
    auto size_bytes() const -> std::size_t { return d_tree.measure(); }

    /** Concatenates all chunks into a single string; O(n). */
    auto to_string() const -> std::string {
        std::string out;
        out.reserve(size_bytes());
        std::ranges::for_each(d_tree.flatten(), [&out](const std::string& c) { out += c; });
        return out;
    }

    /** Returns a new rope with @p text inserted at byte position @p pos; O(log
     * n). */
    auto insert(std::size_t pos, std::string_view text) const -> FingerTreeRope {
        auto [left, right] = split_chars(pos);
        auto middle        = from_text(text);
        return FingerTreeRope{Tree::concat(Tree::concat(left.d_tree, middle.d_tree), right.d_tree)};
    }

    /** Returns a new rope with @p count bytes removed starting at @p pos; O(log
     * n). */
    auto erase(std::size_t pos, std::size_t count) const -> FingerTreeRope {
        auto [left, rest]  = split_chars(pos);
        auto [drop, right] = rest.split_chars(count);
        static_cast<void>(drop);
        return FingerTreeRope{Tree::concat(left.d_tree, right.d_tree)};
    }

    /** Returns a new rope with @p count bytes at @p pos replaced by @p text;
     * O(log n). */
    auto replace(std::size_t pos, std::size_t count, std::string_view text) const -> FingerTreeRope {
        return erase(pos, count).insert(pos, text);
    }

    /** Returns internal chunk vector in sequence order; O(n). */
    auto chunks() const -> std::vector<std::string> { return d_tree.flatten(); }
};

} // namespace beman::fingertree::tree

namespace beman::fingertree {

template <class Tree>
struct FingerTreeRopeFoldableImpl {
    using element_type = std::string;

    // The monoid is an instance object over the result type, passed by
    // value; the Impl never chooses one (beman.transpose
    // docs/transpose/decisions.md#monoid-selection).
    template <class F, class RESULT_MONOID>
    auto fold_map(this auto&&,
                  F&&                                                  function,
                  const beman::fingertree::tree::FingerTreeRope<Tree>& rope,
                  RESULT_MONOID monoid) -> std::remove_cvref_t<std::invoke_result_t<F&, const std::string&>>
        requires std::invocable<F&, const std::string&> &&
                 beman::transpose::monoid_object<RESULT_MONOID,
                                                 std::remove_cvref_t<std::invoke_result_t<F&, const std::string&>>>
    {
        using Result = std::remove_cvref_t<std::invoke_result_t<F&, const std::string&>>;
        return std::ranges::fold_left(rope.chunks(), Result(monoid.identity()), [&](Result acc, const auto& value) {
            return monoid.combine(std::move(acc), std::invoke(function, value));
        });
    }
};

template <class Tree>
struct FingerTreeRopeFoldableMap : beman::transpose::derive_foldable<FingerTreeRopeFoldableImpl<Tree>> {};

} // namespace beman::fingertree

namespace beman::transpose {

template <class Tree>
inline constexpr auto foldable<beman::fingertree::tree::FingerTreeRope<Tree>> =
    beman::fingertree::FingerTreeRopeFoldableMap<Tree>{};

} // namespace beman::transpose

#endif
