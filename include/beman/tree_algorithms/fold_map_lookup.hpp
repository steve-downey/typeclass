// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_TREE_ALGORITHMS_FOLD_MAP_LOOKUP_HPP
#define BEMAN_TREE_ALGORITHMS_FOLD_MAP_LOOKUP_HPP

#include <beman/tree_algorithms/config.hpp>

#if BEMAN_TREE_ALGORITHMS_USE_MODULES() && !defined(BEMAN_TREE_ALGORITHMS_INCLUDED_FROM_INTERFACE_UNIT)

import beman.tree_algorithms;

#else

    #include <beman/transpose/functor.hpp>
    #include <beman/transpose/monoid.hpp>
    #include <beman/tree_algorithms/fold_map.hpp>
    #include <beman/tree_algorithms/recursion_schemes_lookup.hpp>

    #if !BEMAN_TREE_ALGORITHMS_USE_MODULES()
        #include <concepts>
        #include <functional>
        #include <type_traits>
        #include <utility>
    #endif

namespace beman::tree_algorithms {

// Lookup-based overloads of fold_map (Decision 4, as amended 2026-07-14).
//
// The everyday spelling: fold_map<Result>(map_fn, combine, identity, t)
// resolves the structural ingredients — the layer fold, the layer fmap,
// and (for trees in their own representation) the projection — through
// typeclass-object lookup, so call sites pass only the elementwise
// pieces and the data. The fully-explicit forms in fold_map.hpp remain
// the escape hatch and the surface with no mechanism dependency.
//
// The lookup variables here follow the pattern of beman.transpose, the
// Paper A (P3200) bundled-customization facility, whose functor<Layer>
// supplies the layer fmap; this header consumes the mechanism and never
// respecifies it.

// 94d2dd48-8976-43fc-943d-0dd27177e41a
/** Typeclass lookup variable for elementwise layer folds, keyed on the
 * concrete layer type. An instance is a callable
 * (map_fn, combine, identity, F<Result>) -> Result folding one layer in
 * the representation's contractual order. The unspecialized default is
 * std::false_type so a missing instance is detectable.
 */
template <class Layer>
inline constexpr auto layer_fold = std::false_type{};

/** Typeclass lookup variable for projections, keyed on the tree type.
 * An instance is a callable taking a `const Tree*` handle and returning
 * one F<handle> layer — the same shape fold_with's project parameter
 * takes; registered projections deal in raw non-owning pointer handles.
 */
template <class Tree>
inline constexpr auto project = std::false_type{};
// 94d2dd48-8976-43fc-943d-0dd27177e41a end

/** Look up the layer_fold instance for the concrete layer type
 * of @p layer and apply it. The single point where the fold_map lookup
 * overloads bridge into layer-fold dispatch, mirroring layer_fmap. */
template <class MapFn, class Combine, class Result, class Layer>
constexpr auto layer_fold_map(const MapFn& map_fn, const Combine& combine, const Result& identity, const Layer& layer)
    -> Result {
    static_assert(transpose::has_instance_v<decltype(layer_fold<std::remove_cvref_t<Layer> >)>,
                  "no layer-fold instance: specialize "
                  "beman::tree_algorithms::layer_fold for this layer type, or use "
                  "the explicit forms in fold_map.hpp");
    return layer_fold<std::remove_cvref_t<Layer> >(map_fn, combine, identity, layer);
}

/** Elementwise fold over a Fix<F> tree, resolving the layer fold and the
 * layer fmap through typeclass lookup. Equivalent to the explicit form
 * with the looked-up ingredients.
 * @tparam Result the fold carrier; must be given explicitly
 */
// e4beac7f-257b-4f2d-bfff-6b3b5daf61ad
template <typename Result, template <typename> class F, typename MapFn, typename Combine>
constexpr auto fold_map(const MapFn& map_fn, const Combine& combine, const Result& identity, const Fix<F>& tree)
    -> Result {
    auto algebra = [&](const auto& layer) -> Result { return layer_fold_map(map_fn, combine, identity, layer); };
    return fold_fix<Result>(algebra, tree);
}
// e4beac7f-257b-4f2d-bfff-6b3b5daf61ad end

/** Elementwise fold over a tree in its own representation, resolving the
 * projection (keyed on the tree type), the layer fold, and the layer
 * fmap through typeclass lookup. No Fix is materialized.
 * @tparam Result the fold carrier; must be given explicitly
 */
// 7f08b12d-3d26-4b5e-bf79-66d38f6fee0d
template <typename Result, typename MapFn, typename Combine, typename Tree>
constexpr auto fold_map(const MapFn& map_fn, const Combine& combine, const Result& identity, const Tree& tree)
    -> Result {
    static_assert(transpose::has_instance_v<decltype(project<Tree>)>,
                  "no projection instance: specialize "
                  "beman::tree_algorithms::project for this tree type, or use the "
                  "explicit forms in fold_map.hpp");
    auto algebra = [&](const auto& layer) -> Result { return layer_fold_map(map_fn, combine, identity, layer); };
    auto fmap_fn = [](auto&& fn, const auto& layer) { return layer_fmap(std::forward<decltype(fn)>(fn), layer); };
    return fold_with<Result>(algebra, fmap_fn, project<Tree>, &tree);
}
// 7f08b12d-3d26-4b5e-bf79-66d38f6fee0d end

namespace detail {
template <class Tree>
struct fold_map_source_layer {};

template <class Tree>
    requires std::invocable<decltype(project<Tree>), const Tree*>
struct fold_map_source_layer<Tree> {
    using type = std::invoke_result_t<decltype(project<Tree>), const Tree*>;
};

template <template <class> class F>
struct fold_map_source_layer<Fix<F> > {
    using type = F<Fix<F> >;
};

template <class Tree>
concept registered_fold_source =
    requires { typename fold_map_source_layer<Tree>::type; } &&
    transpose::has_instance_v<decltype(layer_fold<typename fold_map_source_layer<Tree>::type>)> &&
    transpose::has_instance_v<decltype(transpose::functor<typename fold_map_source_layer<Tree>::type>)>;
} // namespace detail

/** Monoid-object bridge to the structural fold. This consumer overload
 * retains the passed callable and monoid, including their state. Existing
 * projections and layer maps copy payloads; the carrier must be copyable.
 */
template <class F, class Tree, class Monoid>
    requires detail::registered_fold_source<Tree> && std::copy_constructible<transpose::applicative_value_t<Tree> > &&
             std::invocable<F&, const transpose::applicative_value_t<Tree>&> &&
             std::copyable<
                 std::remove_cvref_t<std::invoke_result_t<F&, const transpose::applicative_value_t<Tree>&> > > &&
             transpose::monoid_object<
                 Monoid,
                 std::remove_cvref_t<std::invoke_result_t<F&, const transpose::applicative_value_t<Tree>&> > >
constexpr auto fold_map(F&& function, const Tree& tree, const Monoid& monoid) {
    using Result = std::remove_cvref_t<std::invoke_result_t<F&, const transpose::applicative_value_t<Tree>&> >;
    auto mapper  = [&](const auto& value) -> Result { return std::invoke(function, value); };
    auto combine = [&](const Result& left, const Result& right) -> Result { return monoid.combine(left, right); };
    return tree_algorithms::fold_map<Result>(mapper, combine, Result(monoid.identity()), tree);
}

} // namespace beman::tree_algorithms

#endif // BEMAN_TREE_ALGORITHMS_USE_MODULES() &&
       // !defined(BEMAN_TREE_ALGORITHMS_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_TREE_ALGORITHMS_FOLD_MAP_LOOKUP_HPP
