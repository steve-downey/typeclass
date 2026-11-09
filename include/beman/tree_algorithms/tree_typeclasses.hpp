// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TREE_ALGORITHMS_TREE_TYPECLASSES_HPP
#define BEMAN_TREE_ALGORITHMS_TREE_TYPECLASSES_HPP

#include <beman/tree_algorithms/config.hpp>

#if BEMAN_TREE_ALGORITHMS_USE_MODULES() && !defined(BEMAN_TREE_ALGORITHMS_INCLUDED_FROM_INTERFACE_UNIT)
import beman.tree_algorithms;
#else

    #include <beman/tree_algorithms/binary_tree.hpp>
    #include <beman/tree_algorithms/expression.hpp>
    #include <beman/tree_algorithms/fringe_tree.hpp>
    #include <beman/tree_algorithms/rose_tree.hpp>
    #include <beman/transpose/fold.hpp>
    #include <beman/transpose/sequence.hpp>
    #include <cassert>

    #if !BEMAN_TREE_ALGORITHMS_USE_MODULES()
        #include <concepts>
        #include <cstddef>
        #include <exception>
        #include <functional>
        #include <memory>
        #include <optional>
        #include <type_traits>
        #include <utility>
        #include <vector>
    #endif

// Payload instances are implementation evidence, consuming transpose's
// machinery. Structural layer Functors continue to map child positions.
namespace beman::tree_algorithms::detail {

struct binary_payload_shape {
    std::optional<std::size_t> left, right;
};

// Concrete reconstruction operations keep storage choices in their own
// representation: shared_ptr children for BinaryTree, Box at the Fix knot.
template <class T>
struct binary_payload_model {
    using element_type = T;
    template <class U>
    using rebind = BinaryTree<U>;
    static constexpr auto value(const BinaryTree<T>& tree) -> const T& { return tree.value(); }
    static constexpr auto left(const BinaryTree<T>& tree) -> const BinaryTree<T>* {
        return tree.has_left() ? &tree.left() : nullptr;
    }
    static constexpr auto right(const BinaryTree<T>& tree) -> const BinaryTree<T>* {
        return tree.has_right() ? &tree.right() : nullptr;
    }
    template <class U>
    static constexpr auto node(U value, std::optional<rebind<U> > left, std::optional<rebind<U> > right) -> rebind<U> {
        return rebind<U>::from_children_ptrs(std::move(value),
                                             left ? rebind<U>::make_ptr(std::move(*left)) : nullptr,
                                             right ? rebind<U>::make_ptr(std::move(*right)) : nullptr);
    }
};

template <class T, class Tree>
struct fixed_binary_payload_model {
    using element_type = T;
    template <class U>
    using rebind = BinaryTreeFix<U>;
    static constexpr auto value(const Tree& tree) -> const T& { return tree.inner.value; }
    static constexpr auto left(const Tree& tree) -> const Tree* {
        return tree.inner.left ? std::addressof(*tree.inner.left) : nullptr;
    }
    static constexpr auto right(const Tree& tree) -> const Tree* {
        return tree.inner.right ? std::addressof(*tree.inner.right) : nullptr;
    }
    template <class U>
    static constexpr auto node(U value, std::optional<rebind<U> > left, std::optional<rebind<U> > right) -> rebind<U> {
        using Result = rebind<U>;
        return Result{BinaryTreeF<U, Result>{std::move(value),
                                             left ? make_slot<Result>(std::move(*left)) : child_slot_t<Result>{},
                                             right ? make_slot<Result>(std::move(*right)) : child_slot_t<Result>{}}};
    }
};

template <class Model>
struct binary_payload_builder {
    std::vector<binary_payload_shape> shape;
    std::size_t                       root;
    template <class U>
    constexpr auto operator()(std::vector<U> values) const -> typename Model::template rebind<U> {
        assert(values.size() == shape.size());
        using Result = typename Model::template rebind<U>;
        auto rebuild = [&](auto&& self, std::size_t position) -> Result {
            std::optional<Result> left, right;
            if (shape[position].left)
                left.emplace(self(self, *shape[position].left));
            if (shape[position].right)
                right.emplace(self(self, *shape[position].right));
            return Model::template node<U>(std::move(values[position]), std::move(left), std::move(right));
        };
        return rebuild(rebuild, root);
    }
};

template <class Model, class Tree>
constexpr auto record_binary(const Tree& source) {
    std::vector<const typename Model::element_type*> positions;
    std::vector<binary_payload_shape>                shape;
    auto                                             record = [&](auto&& self, const Tree& tree) -> std::size_t {
        std::optional<std::size_t> left;
        if (auto child = Model::left(tree))
            left = self(self, *child);
        auto index = positions.size();
        positions.push_back(std::addressof(Model::value(tree)));
        shape.push_back({left, {}});
        if (auto child = Model::right(tree)) {
            // Recursion may reallocate shape; finish it before indexing.
            const auto right   = self(self, *child);
            shape[index].right = right;
        }
        return index;
    };
    const auto root = record(record, source);
    return std::pair{std::move(positions), binary_payload_builder<Model>{std::move(shape), root}};
}

template <class T>
struct fringe_payload_model {
    using element_type = T;
    // Pre-order node kinds encode the complete topology, including empties.
    enum class kind { empty, leaf, branch };
    struct builder {
        std::vector<kind> shape;
        template <class U>
        constexpr auto operator()(std::vector<U> values) const -> FringeTree<U> {
            std::size_t node = 0, payload = 0;
            auto        rebuild = [&](auto&& self) -> FringeTree<U> {
                switch (shape[node++]) {
                case kind::empty:
                    return FringeTree<U>::empty();
                case kind::leaf:
                    return FringeTree<U>::leaf(std::move(values[payload++]));
                case kind::branch: {
                    auto left  = self(self);
                    auto right = self(self);
                    return FringeTree<U>::branch(std::move(left), std::move(right));
                }
                }
                std::terminate();
            };
            auto result = rebuild(rebuild);
            assert(node == shape.size() && payload == values.size());
            return result;
        }
    };
    static constexpr auto record(const FringeTree<T>& source) {
        std::vector<const T*> positions;
        std::vector<kind>     shape;
        auto                  visit = [&](auto&& self, const FringeTree<T>& tree) -> void {
            if (tree.is_empty())
                shape.push_back(kind::empty);
            else if (tree.is_leaf()) {
                shape.push_back(kind::leaf);
                positions.push_back(std::addressof(tree.value()));
            } else {
                shape.push_back(kind::branch);
                self(self, tree.left());
                self(self, tree.right());
            }
        };
        visit(visit, source);
        return std::pair{std::move(positions), builder{std::move(shape)}};
    }
};

template <class T, class Tree>
struct rose_payload_model {
    using element_type = T;
    struct builder {
        std::vector<std::size_t> arities;
        template <class U>
        constexpr auto operator()(std::vector<U> values) const -> RoseTreeFix<U> {
            assert(values.size() == arities.size());
            std::size_t position = 0;
            auto        rebuild  = [&](auto&& self) -> RoseTreeFix<U> {
                const auto                   index = position++;
                std::vector<RoseTreeFix<U> > children;
                children.reserve(arities[index]);
                for (std::size_t i = 0; i < arities[index]; ++i)
                    children.push_back(self(self));
                return rose<U>(std::move(values[index]), std::move(children));
            };
            return rebuild(rebuild);
        }
    };
    static constexpr auto record(const Tree& source) {
        std::vector<const T*>    positions;
        std::vector<std::size_t> arities;
        auto                     visit = [&](auto&& self, const Tree& tree) -> void {
            positions.push_back(std::addressof(tree.inner.value));
            arities.push_back(tree.inner.children.size());
            for (const auto& child : tree.inner.children)
                self(self, child);
        };
        visit(visit, source);
        return std::pair{std::move(positions), builder{std::move(arities)}};
    }
};

// This closed family recognition is intentionally confined to the consumer
// header. A structural Functor alone does not declare payload traversal.
template <class Layer, class Tree>
struct fixed_payload_model {};
template <class T, class A, class Tree>
    requires std::same_as<Tree, BinaryTreeFix<T> >
struct fixed_payload_model<BinaryTreeF<T, A>, Tree> : fixed_binary_payload_model<T, Tree> {
    static constexpr auto record(const Tree& tree) {
        return record_binary<fixed_binary_payload_model<T, Tree> >(tree);
    }
};
template <class T, class A, class Tree>
    requires std::same_as<Tree, RoseTreeFix<T> >
struct fixed_payload_model<RoseF<T, A>, Tree> : rose_payload_model<T, Tree> {};

template <class Tree>
struct payload_model {};
template <class T>
struct payload_model<BinaryTree<T> > : binary_payload_model<T> {
    static constexpr auto record(const BinaryTree<T>& tree) { return record_binary<binary_payload_model<T> >(tree); }
};
template <class T>
struct payload_model<FringeTree<T> > : fringe_payload_model<T> {};
template <template <class> class F>
    requires requires { typename fixed_payload_model<F<Fix<F> >, Fix<F> >::element_type; }
struct payload_model<Fix<F> > : fixed_payload_model<F<Fix<F> >, Fix<F> > {};

template <class Tree>
concept payload_tree = requires { typename payload_model<Tree>::element_type; };

template <class F, class T>
concept payload_mapper =
    std::invocable<F&, const T&> && transpose::detail::holdable_value<std::invoke_result_t<F&, const T&> > &&
    std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, const T&> >,
                            std::invoke_result_t<F&, const T&> > &&
    std::move_constructible<std::remove_cvref_t<std::invoke_result_t<F&, const T&> > >;

template <class Tree>
struct PayloadFunctorImpl {
    using element_type = typename payload_model<Tree>::element_type;
    template <class F>
        requires payload_mapper<F, element_type>
    constexpr auto fmap(this auto&&, F&& function, const Tree& tree) {
        auto [positions, builder] = payload_model<Tree>::record(tree);
        using U                   = std::remove_cvref_t<std::invoke_result_t<F&, const element_type&> >;
        std::vector<U> values;
        values.reserve(positions.size());
        for (const auto* value : positions)
            values.emplace_back(std::invoke(function, *value));
        return builder(std::move(values));
    }
};
template <class Tree>
struct PayloadFunctorMap : transpose::derive_functor<PayloadFunctorImpl<Tree> > {};

template <class Tree, class T = transpose::applicative_value_t<Tree> >
struct PayloadFoldableImpl {
    using element_type = T;
    template <class F, class Monoid>
        requires requires(F&& function, const Tree& tree, const Monoid& monoid) {
            tree_algorithms::fold_map(std::forward<F>(function), tree, monoid);
        }
    constexpr auto fold_map(this auto&&, F&& function, const Tree& tree, const Monoid& monoid) {
        return tree_algorithms::fold_map(std::forward<F>(function), tree, monoid);
    }
};
template <class Tree>
struct PayloadFoldableMap : transpose::derive_foldable<PayloadFoldableImpl<Tree> > {};

template <class Tree>
struct PayloadTraversableImpl {
    using element_type = typename payload_model<Tree>::element_type;
    template <class Applicative, class F>
        requires payload_mapper<F, element_type> &&
                 transpose::applicative_object_for<
                     Applicative,
                     std::remove_cvref_t<std::invoke_result_t<F&, const element_type&> > > &&
                 transpose::detail::holdable_value<transpose::applicative_value_t<
                     std::remove_cvref_t<std::invoke_result_t<F&, const element_type&> > > > &&
                 std::move_constructible<transpose::applicative_value_t<
                     std::remove_cvref_t<std::invoke_result_t<F&, const element_type&> > > >
    constexpr auto traverse(this auto&&, const Applicative& applicative, F&& function, const Tree& tree) {
        using Context             = std::remove_cvref_t<std::invoke_result_t<F&, const element_type&> >;
        using U                   = transpose::applicative_value_t<Context>;
        auto [positions, builder] = payload_model<Tree>::record(tree);
        // These pointers are consumed synchronously by the mapper; deferred
        // effects own their captures, and the final closure owns only shape.
        auto accumulated = transpose::traversable<decltype(positions)>.traverse(
            applicative,
            [&](const element_type* value) -> Context { return std::invoke(function, *value); },
            positions);
        return applicative.invoke(
            [builder = std::move(builder)](std::vector<U> values) { return builder(std::move(values)); },
            std::move(accumulated));
    }
};
template <class Tree>
struct PayloadTraversableMap : transpose::derive_traversable<PayloadTraversableImpl<Tree> > {};

} // namespace beman::tree_algorithms::detail

namespace beman::transpose {

template <template <class> class F>
    requires tree_algorithms::detail::payload_tree<tree_algorithms::Fix<F> >
struct applicative_value<tree_algorithms::Fix<F> > {
    using type = typename tree_algorithms::detail::payload_model<tree_algorithms::Fix<F> >::element_type;
};
template <>
struct applicative_value<tree_algorithms::Expr> {
    using type = int;
};

template <class Tree>
    requires tree_algorithms::detail::payload_tree<Tree>
inline constexpr auto functor<Tree> = tree_algorithms::detail::PayloadFunctorMap<Tree>{};
template <class Tree>
    requires tree_algorithms::detail::payload_tree<Tree>
inline constexpr auto foldable<Tree> = tree_algorithms::detail::PayloadFoldableMap<Tree>{};
template <>
inline constexpr auto foldable<tree_algorithms::Expr> =
    tree_algorithms::detail::PayloadFoldableMap<tree_algorithms::Expr>{};
template <class Tree>
    requires tree_algorithms::detail::payload_tree<Tree>
inline constexpr auto traversable<Tree> = tree_algorithms::detail::PayloadTraversableMap<Tree>{};

} // namespace beman::transpose

#endif
#endif
