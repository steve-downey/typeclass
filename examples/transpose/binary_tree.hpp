// examples/transpose/binary_tree.hpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Shared example header: BinaryTree<T> data type with Foldable, Applicative,
// and Traversable typeclass instances. Not part of the proposed library.
#ifndef EXAMPLE_BINARY_TREE_HPP
#define EXAMPLE_BINARY_TREE_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace example {

/** Persistent binary tree where every node carries a value.
 * Nodes are either leaves (no children) or internal nodes with left and right
 * subtrees. Sharing is structural: subtrees are held by shared_ptr, so copies
 * are cheap and the tree is immutable once built.
 * @tparam T element type stored at every node
 */
template <class T>
class BinaryTree {
    T d_value;
    std::shared_ptr<BinaryTree> d_left;
    std::shared_ptr<BinaryTree> d_right;

  public:
    using value_type = T;

    /** Construct a leaf node holding @p value (no children). */
    static auto leaf(T value) -> BinaryTree {
        return BinaryTree(std::move(value), {}, {});
    }

    /** Construct an internal node with @p value and two children. */
    static auto node(T value, BinaryTree left, BinaryTree right) -> BinaryTree {
        return BinaryTree(std::move(value),
                          std::make_shared<BinaryTree>(std::move(left)),
                          std::make_shared<BinaryTree>(std::move(right)));
    }

    /** Alias for node(); prefer node() in new code. */
    static auto branch(T value, BinaryTree left, BinaryTree right)
        -> BinaryTree {
        return node(std::move(value), std::move(left), std::move(right));
    }

    /** Low-level constructor accepting pre-built child shared_ptrs.
     * Null pointers represent absent children.
     */
    static auto from_children_ptrs(T value, std::shared_ptr<BinaryTree> left,
                                   std::shared_ptr<BinaryTree> right)
        -> BinaryTree {
        return BinaryTree(std::move(value), std::move(left), std::move(right));
    }

    /** Heap-allocate a copy of @p tree and return the owning pointer. */
    static auto make_ptr(BinaryTree tree) -> std::shared_ptr<BinaryTree> {
        return std::make_shared<BinaryTree>(std::move(tree));
    }

    /** Return the value stored at this node. */
    auto value() const -> const T & { return d_value; }

    /** True when this node has a left child. */
    auto has_left() const -> bool { return static_cast<bool>(d_left); }
    /** True when this node has a right child. */
    auto has_right() const -> bool { return static_cast<bool>(d_right); }

    /** Return the left child; precondition: has_left(). */
    auto left() const -> const BinaryTree & {
        assert(d_left);
        return *d_left;
    }

    /** Return the right child; precondition: has_right(). */
    auto right() const -> const BinaryTree & {
        assert(d_right);
        return *d_right;
    }

    /** Shared pointer to the left child; may be null. */
    auto left_ptr() const -> const std::shared_ptr<BinaryTree> & {
        return d_left;
    }
    /** Shared pointer to the right child; may be null. */
    auto right_ptr() const -> const std::shared_ptr<BinaryTree> & {
        return d_right;
    }

  private:
    BinaryTree(T value, std::shared_ptr<BinaryTree> left,
               std::shared_ptr<BinaryTree> right)
        : d_value(std::move(value)), d_left(std::move(left)),
          d_right(std::move(right)) {}
};

// ---------------------------------------------------------------------------
// Foldable instance for BinaryTree<T>
// ---------------------------------------------------------------------------

/** Foldable typeclass instance for BinaryTree<T>.
 * fold_map applies @p function to every node value (in-order: left, root,
 * right) and combines the results under the monoid instance it is passed.
 * @tparam T element type of the tree being folded
 */
template <class T>
struct BinaryTreeFoldableImpl {
    using element_type = T;

    template <class F, class MONOID>
    auto fold_map(this auto &&self, F &&function, const BinaryTree<T> &tree,
                  MONOID monoid)
        -> std::remove_cvref_t<decltype(std::invoke(function, tree.value()))> {
        auto value_result = std::invoke(function, tree.value());
        using Result = std::remove_cvref_t<decltype(value_result)>;

        // A tree is never empty, so the identity is never needed: every
        // combine has a real operand on both sides.
        Result acc =
            tree.has_left()
                ? monoid.combine(self.fold_map(function, tree.left(), monoid),
                                 std::move(value_result))
                : std::move(value_result);

        if (tree.has_right()) {
            acc = monoid.combine(std::move(acc),
                                 self.fold_map(function, tree.right(), monoid));
        }

        return acc;
    }
};

/** Foldable map that exposes the fold family for BinaryTree<T>. */
template <class T>
struct BinaryTreeFoldableMap
    : beman::transpose::derive_foldable<BinaryTreeFoldableImpl<T>> {};

// ---------------------------------------------------------------------------
// Applicative instance for BinaryTree<T>
// ---------------------------------------------------------------------------

/** Applicative typeclass instance for BinaryTree<T> with shape-aware
 * semantics.
 *
 * pure(v) produces a single leaf. ap recurses pairwise over matching tree
 * structure: a leaf function distributes over the argument's shape; a leaf
 * argument distributes over the function's shape; when both have children,
 * only positions where both trees have a child are combined (pairwise).
 * These are monad-derived (not zip) applicative semantics.
 *
 * DELIBERATELY ap-basis: this instance is the living exemplar that ap is
 * a perfectly cromulent basis for Applicative. The base derives the
 * user-facing n-ary `invoke` from pure + ap via terminating partial
 * application, while invoke-native instances (optional, sender, std::simd)
 * go the other way. The shape-aware pairwise recursion is a case where
 * one-step application is the genuinely natural formulation: each
 * recursion step really does hold functions in the context.
 * @tparam T element type of the function tree (F is the function type stored)
 */
template <class T>
struct BinaryTreeApplicativeImpl {
    /** Lift a plain value into a single-leaf tree. */
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        using U = std::remove_cvref_t<VALUE>;
        return BinaryTree<U>::leaf(std::forward<VALUE>(value));
    }

    /**
     * @brief One-step application of a tree of functions to a tree of
     * arguments, shape-aware.
     * @param functions tree whose nodes contain callables
     * @param arguments tree whose nodes contain arguments
     * @return tree of results; shape determined by pairwise recursion rules
     */
    template <class F, class A>
    auto ap(this auto &&self, const BinaryTree<F> &functions,
            const BinaryTree<A> &arguments)
        -> BinaryTree<std::invoke_result_t<const F &, const A &>> {
        using R = std::invoke_result_t<const F &, const A &>;

        std::shared_ptr<BinaryTree<R>> left{};
        std::shared_ptr<BinaryTree<R>> right{};

        const auto function_is_leaf =
            !functions.has_left() && !functions.has_right();
        const auto argument_is_leaf =
            !arguments.has_left() && !arguments.has_right();

        if (function_is_leaf) {
            // pure(f) should distribute f over the argument shape.
            if (arguments.has_left()) {
                left = BinaryTree<R>::make_ptr(
                    self.ap(functions, arguments.left()));
            }
            if (arguments.has_right()) {
                right = BinaryTree<R>::make_ptr(
                    self.ap(functions, arguments.right()));
            }
        } else if (argument_is_leaf) {
            // A non-leaf function tree can be applied pointwise to a single
            // argument.
            if (functions.has_left()) {
                left = BinaryTree<R>::make_ptr(
                    self.ap(functions.left(), arguments));
            }
            if (functions.has_right()) {
                right = BinaryTree<R>::make_ptr(
                    self.ap(functions.right(), arguments));
            }
        } else {
            // If both have shape, recurse pairwise where both children exist.
            if (functions.has_left() && arguments.has_left()) {
                left = BinaryTree<R>::make_ptr(
                    self.ap(functions.left(), arguments.left()));
            }

            if (functions.has_right() && arguments.has_right()) {
                right = BinaryTree<R>::make_ptr(
                    self.ap(functions.right(), arguments.right()));
            }
        }

        return BinaryTree<R>::from_children_ptrs(
            functions.value()(arguments.value()), std::move(left),
            std::move(right));
    }
};

/** Applicative map for BinaryTree<T>: ap-basis; the base derives the
 * user-facing n-ary invoke. */
template <class T>
struct BinaryTreeApplicativeMap
    : beman::transpose::derive_applicative<BinaryTreeApplicativeImpl<T>> {
    using BinaryTreeApplicativeImpl<T>::ap;
    using BinaryTreeApplicativeImpl<T>::pure;
};

// ---------------------------------------------------------------------------
// Traversable instance for BinaryTree<T>
// ---------------------------------------------------------------------------

namespace detail {

struct BinaryTreeShape {
    std::optional<std::size_t> left;
    std::optional<std::size_t> right;
};

// Keep reconstruction outside the return-type probes used by Traversable.
// A named callable also owns the topology for deferred applicatives.
struct BinaryTreeBuilder {
    std::vector<BinaryTreeShape> shape;
    std::size_t root;

    template <class U>
    auto operator()(std::vector<U> values) const -> BinaryTree<U> {
        using ResultTree = BinaryTree<U>;
        auto rebuild = [&](auto &&self, std::size_t position) -> ResultTree {
            std::shared_ptr<ResultTree> left, right;
            if (shape[position].left)
                left = ResultTree::make_ptr(self(self, *shape[position].left));
            if (shape[position].right)
                right =
                    ResultTree::make_ptr(self(self, *shape[position].right));
            return ResultTree::from_children_ptrs(
                std::move(values[position]), std::move(left), std::move(right));
        };
        return rebuild(rebuild, root);
    }
};

} // namespace detail

/** Traversable typeclass instance for BinaryTree<T>.
 * traverse maps each node value into an applicative context and rebuilds a
 * BinaryTree inside that context, preserving the original tree's shape.
 * Values and effects are visited in-order, once per node. Sequencing uses
 * O(n) auxiliary node/shape storage; reconstruction owns its shape metadata
 * so deferred effects can outlive the source tree.
 * @tparam T element type of the tree being traversed
 */
template <class T>
struct BinaryTreeTraversableImpl {
    using element_type = T;

    // No `consumes_rvalue_structure`, so this object presents `const`
    // lvalue elements whatever category it was handed the tree in, and
    // `traverse` infers its context accordingly. That is not an omission.
    // BinaryTree is persistent and shares its subtrees, so an rvalue tree
    // conveys the handle and not exclusive ownership of the values inside
    // it: the only consuming traversal it could offer would copy each
    // element into a temporary and then move that, costing a move per
    // element more than this overload, which copies straight into the
    // result. A structure that shares its parts declines the declaration
    // and pays nothing for declining.
    //
    // The constraint is what makes an availability probe answer rather than
    // diagnose: the return type is deduced, so a callable this overload
    // cannot invoke would otherwise reach the body.
    template <class APPLICATIVE, class F>
        requires std::invocable<F &, const T &> &&
                 beman::transpose::applicative_object_for<
                     APPLICATIVE,
                     std::remove_cvref_t<std::invoke_result_t<F &, const T &>>>
    auto traverse(this auto &&, const APPLICATIVE &applicative, F &&function,
                  const BinaryTree<T> &tree) {
        std::vector<detail::BinaryTreeShape> shape;
        std::vector<const BinaryTree<T> *> nodes;
        auto record = [&](auto &&self,
                          const BinaryTree<T> &node) -> std::size_t {
            std::optional<std::size_t> left;
            if (node.has_left())
                left = self(self, node.left());
            const auto position = nodes.size();
            nodes.push_back(&node);
            shape.push_back({left, {}});
            if (node.has_right()) {
                const auto right = self(self, node.right());
                shape[position].right = right;
            }
            return position;
        };
        const auto root = record(record, tree);

        // One whole-tree sequencing pass admits native runtime collection,
        // including policies whose pure and composed carriers differ.
        const auto &ordered_nodes = nodes;
        auto accumulated =
            beman::transpose::traversable<std::vector<const BinaryTree<T> *>>.traverse(
                applicative,
                [&](const BinaryTree<T> *node) {
                    return std::invoke(function, node->value());
                },
                ordered_nodes);
        return applicative.invoke(
            detail::BinaryTreeBuilder{std::move(shape), root},
            std::move(accumulated));
    }
};

/** Traversable map that exposes traverse for BinaryTree<T>. */
template <class T>
struct BinaryTreeTraversableMap
    : beman::transpose::derive_traversable<BinaryTreeTraversableImpl<T>> {
    using BinaryTreeTraversableImpl<T>::traverse;
};

} // namespace example

namespace beman::transpose {

// ---------------------------------------------------------------------------
// Typeclass registrations for example::BinaryTree<T>
// ---------------------------------------------------------------------------

/** Registers BinaryTreeFoldableMap as the Foldable instance for
 * example::BinaryTree<T>. */
template <class T>
inline constexpr auto foldable<example::BinaryTree<T>> =
    example::BinaryTreeFoldableMap<T>{};

/** Registers BinaryTreeApplicativeMap as the Applicative instance for
 * example::BinaryTree<T>. */
template <class T>
inline constexpr auto applicative<example::BinaryTree<T>> =
    example::BinaryTreeApplicativeMap<T>{};

/** Registers BinaryTreeTraversableMap as the Traversable instance for
 * example::BinaryTree<T>. */
template <class T>
inline constexpr auto traversable<example::BinaryTree<T>> =
    example::BinaryTreeTraversableMap<T>{};

} // namespace beman::transpose

#endif // EXAMPLE_BINARY_TREE_HPP
