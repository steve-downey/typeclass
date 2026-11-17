// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_TYPECLASS_TREE_ADAPTERS_FINGER_TREE_HPP
#define BEMAN_TYPECLASS_TREE_ADAPTERS_FINGER_TREE_HPP

// The structural adapter between beman.fingertree and beman.tree_algorithms.
//
// Opt-in: neither library includes this header or links its target, and
// neither depends on the other. Including it is what makes a FingerTree a
// tree the recursive algorithms can see: fold_with, unfold_with and refold
// in the tree's own representation, fold_fix and unfold_fix through its Fix
// form, and both the explicit and the lookup fold_map.
//
// What the algorithms see is the REPRESENTATION, not the sequence. One layer
// is one real node of the tree as fingertree stores it: Empty, Single or
// Deep on the spine, Leaf, Node2 or Node3 below a digit, with digit order
// and spine position preserved. A sequence operation is free to rebalance
// that shape; a structural fold reports whatever shape is there. Folding the
// elements left to right gives the sequence back, which is the law that ties
// this adapter to fingertree's own Foldable.
//
// The representation has two recursive sorts -- the tree at a spine level,
// and an element node -- and the verbs recurse through one position type.
// FingerTreePart is that type: a finite tagged value holding one or the
// other. It does not change with spine depth, so a fold over a tree of any
// size instantiates the same handful of templates. A part is both the handle
// a projection hands out and the piece an embedding assembles.
//
// Capability wrappers are reached through their backing tree, which is the
// representation they have: fold_map<Result>(f, combine, identity,
// index.backing_tree()). The payload a structural algorithm then sees is the
// backing tree's, an Interval<T> for the interval index and not its T.

#include <beman/fingertree/tree/finger_tree.hpp>
#include <beman/fingertree/tree/finger_tree_structure.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/overloaded.hpp>
#include <beman/tree_algorithms/child_slot.hpp>
#include <beman/tree_algorithms/fix.hpp>
#include <beman/tree_algorithms/fold_map_lookup.hpp>
#include <beman/tree_algorithms/recursion_schemes_lookup.hpp>

#include <cassert>
#include <concepts>
#include <functional>
#include <inplace_vector>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace beman::typeclass_tree_adapters {

// ---------------------------------------------------------------------
// The base functor: one layer of a finger tree.
// ---------------------------------------------------------------------

/** The tree at a spine level with no elements. */
struct FingerEmpty {};

/** The tree at a spine level with exactly one element. */
template <typename A>
struct FingerSingle {
    tree_algorithms::child_slot_t<A> element;
};

/** One to four element positions, in sequence order. */
template <typename A>
using FingerDigit = std::inplace_vector<tree_algorithms::child_slot_t<A>, 4>;

/** The tree at a spine level with a left digit, the next spine level, and a
 * right digit, in that sequence order. A disengaged spine is a Deep that
 * stores none, which is the representation's own form of an empty one. */
template <typename A>
struct FingerDeep {
    FingerDigit<A>                   left;
    tree_algorithms::child_slot_t<A> spine;
    FingerDigit<A>                   right;
};

/** An element holding one payload. It has no recursive position.
 * @tparam P the payload slot: the payload type itself for a layer that owns
 *           its payload, or FingerPayloadRef for one that shares it
 */
template <typename P>
struct FingerLeaf {
    P value;
};

/** An element of two elements one lower, in sequence order. */
template <typename A>
struct FingerNode2 {
    tree_algorithms::child_slot_t<A> a;
    tree_algorithms::child_slot_t<A> b;
};

/** An element of three elements one lower, in sequence order. */
template <typename A>
struct FingerNode3 {
    tree_algorithms::child_slot_t<A> a;
    tree_algorithms::child_slot_t<A> b;
    tree_algorithms::child_slot_t<A> c;
};

/** One layer of a finger tree's representation, with @p A filling the
 * recursive positions.
 *
 * The first three alternatives are the forms of the tree at a spine level;
 * the last three are the forms of an element. Which sort a position holds
 * follows from where it is: a Single's element, a Deep's digits and a
 * node's children are elements, and a Deep's spine is a tree.
 *
 * This is a class and not an alias for the variant, so the typeclass
 * instances below are registered on a type this header owns. It visits,
 * holds_alternative and get exactly as its base does.
 * @tparam P the payload slot stored by a Leaf
 * @tparam A the type in the recursive positions
 */
template <typename P, typename A>
struct FingerTreeF
    : std::variant<FingerEmpty, FingerSingle<A>, FingerDeep<A>, FingerLeaf<P>, FingerNode2<A>, FingerNode3<A>> {
    using variant_type =
        std::variant<FingerEmpty, FingerSingle<A>, FingerDeep<A>, FingerLeaf<P>, FingerNode2<A>, FingerNode3<A>>;
    using variant_type::variant_type;
};

// ---------------------------------------------------------------------
// Payload slots.
// ---------------------------------------------------------------------

/** A payload that stays where the tree stores it. Shares ownership of the
 * leaf, so a projected layer copies no payload and remains valid on its
 * own, whatever happens to the tree it was read from.
 * @tparam T the payload type
 */
template <typename T>
class FingerPayloadRef {
    std::shared_ptr<const T> d_value;

  public:
    using value_type = T;

    /** Precondition: @p value is not null. */
    explicit FingerPayloadRef(std::shared_ptr<const T> value) noexcept : d_value(std::move(value)) {
        assert(d_value != nullptr && "FingerPayloadRef: null payload");
    }

    [[nodiscard]] auto get() const noexcept -> const T& { return *d_value; }
    [[nodiscard]] auto operator*() const noexcept -> const T& { return *d_value; }
    [[nodiscard]] auto operator->() const noexcept -> const T* { return d_value.get(); }
};

/** The payload in a Leaf's slot, whichever kind of slot it is. An algebra
 * written with this reads owned and shared layers alike. */
template <typename T>
[[nodiscard]] constexpr auto finger_payload(const FingerPayloadRef<T>& slot) noexcept -> const T& {
    return slot.get();
}

template <typename P>
[[nodiscard]] constexpr auto finger_payload(const P& slot) noexcept -> const P& {
    return slot;
}

namespace detail {

template <typename P>
inline constexpr bool is_payload_ref = false;
template <typename T>
inline constexpr bool is_payload_ref<FingerPayloadRef<T>> = true;

// Whether a payload of type T can be made from a slot handed over as SLOT
// (a P&& to consume, a const P& to keep). A shared slot is always copied
// from; an owned one is moved or copied as it was handed over.
template <typename SLOT, typename T>
concept payload_source =
    (is_payload_ref<std::remove_cvref_t<SLOT>> && std::same_as<typename std::remove_cvref_t<SLOT>::value_type, T> &&
     std::copy_constructible<T>) ||
    (!is_payload_ref<std::remove_cvref_t<SLOT>> && std::constructible_from<T, SLOT>);

template <typename T, typename SLOT>
constexpr auto make_payload(SLOT&& slot) -> T {
    if constexpr (is_payload_ref<std::remove_cvref_t<SLOT>>) {
        return T(slot.get());
    } else {
        return T(std::forward<SLOT>(slot));
    }
}

} // namespace detail

// ---------------------------------------------------------------------
// Functor instance.
// ---------------------------------------------------------------------

/** Functor primitive for FingerTreeF<P, A>: applies @p fn to every
 * recursive position in sequence order -- a Deep's left digit, then its
 * spine, then its right digit. Payload slots copy across untouched. */
template <typename P, typename A>
struct FingerTreeFFunctorImpl {
    template <typename Fn>
    constexpr auto fmap(this auto&&, Fn&& fn, const FingerTreeF<P, A>& layer)
        requires std::invocable<Fn&, const A&> &&
                 transpose::detail::holdable_value<std::invoke_result_t<Fn&, const A&>> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<Fn&, const A&>>,
                                         std::invoke_result_t<Fn&, const A&>> &&
                 std::move_constructible<std::remove_cvref_t<std::invoke_result_t<Fn&, const A&>>> &&
                 std::copy_constructible<P>
    {
        using B         = std::remove_cvref_t<std::invoke_result_t<Fn&, const A&>>;
        using Layer     = FingerTreeF<P, B>;
        const auto slot = [&fn](const tree_algorithms::child_slot_t<A>& source) -> tree_algorithms::child_slot_t<B> {
            if (!source)
                return tree_algorithms::child_slot_t<B>{};
            return tree_algorithms::make_slot<B>(std::invoke(fn, *source));
        };
        const auto digit = [&slot](const FingerDigit<A>& source) -> FingerDigit<B> {
            FingerDigit<B> out;
            for (const auto& position : source)
                out.push_back(slot(position));
            return out;
        };
        return std::visit(
            transpose::overloaded{
                [](const FingerEmpty&) -> Layer { return FingerEmpty{}; },
                [&](const FingerSingle<A>& s) -> Layer { return FingerSingle<B>{slot(s.element)}; },
                [&](const FingerDeep<A>& d) -> Layer {
                    return FingerDeep<B>{digit(d.left), slot(d.spine), digit(d.right)};
                },
                [](const FingerLeaf<P>& l) -> Layer { return FingerLeaf<P>{l.value}; },
                [&](const FingerNode2<A>& n) -> Layer { return FingerNode2<B>{slot(n.a), slot(n.b)}; },
                [&](const FingerNode3<A>& n) -> Layer { return FingerNode3<B>{slot(n.a), slot(n.b), slot(n.c)}; },
            },
            layer);
    }
};

/** Functor map for FingerTreeF<P, A>: the fmap primitive plus the derived
 * operations from the Functor CRTP base. */
template <typename P, typename A>
struct FingerTreeFFunctorMap : transpose::derive_functor<FingerTreeFFunctorImpl<P, A>> {
    using FingerTreeFFunctorImpl<P, A>::fmap;
};

} // namespace beman::typeclass_tree_adapters

namespace beman::transpose {

// The structural Functor maps child positions, not the stored payload.
template <class P, class A>
struct applicative_value<beman::typeclass_tree_adapters::FingerTreeF<P, A>> {
    using type = A;
};

/** Registers FingerTreeFFunctorMap as the Functor instance for
 * FingerTreeF<P, A>. */
template <typename P, typename A>
inline constexpr auto functor<beman::typeclass_tree_adapters::FingerTreeF<P, A>> =
    beman::typeclass_tree_adapters::FingerTreeFFunctorMap<P, A>{};

} // namespace beman::transpose

namespace beman::typeclass_tree_adapters {

// ---------------------------------------------------------------------
// Parts: the one position type for both recursive sorts.
// ---------------------------------------------------------------------

/** One part of a finger tree: the tree at a spine level, or an element
 * node. This is the handle a projection hands out and the piece an
 * embedding assembles.
 *
 * A part owns a share of the immutable structure it denotes. Copying one is
 * O(1) and copies no payload, and a part is valid for as long as it exists.
 * @tparam TREE a FingerTree specialization
 */
template <typename TREE>
class FingerTreePart {
  public:
    using tree_type  = TREE;
    using level_type = fingertree::tree::FingerTreeLevel<TREE>;
    using node_type  = typename level_type::node_type;
    using tag_type   = typename level_type::tag_type;

    /** The whole of @p sequence: its spine at level 0. */
    explicit FingerTreePart(const TREE& sequence) : d_part(std::in_place_type<level_type>, sequence) {}

    explicit FingerTreePart(level_type level) : d_part(std::move(level)) {}
    explicit FingerTreePart(node_type node) : d_part(std::move(node)) {}

    [[nodiscard]] auto is_level() const noexcept -> bool { return std::holds_alternative<level_type>(d_part); }
    [[nodiscard]] auto is_node() const noexcept -> bool { return std::holds_alternative<node_type>(d_part); }

    /** The spine level. Precondition: is_level(). */
    [[nodiscard]] auto level() const& noexcept -> const level_type& {
        assert(is_level() && "FingerTreePart::level: the part is an element node");
        return *std::get_if<level_type>(&d_part);
    }
    [[nodiscard]] auto level() && -> level_type {
        assert(is_level() && "FingerTreePart::level: the part is an element node");
        return std::move(*std::get_if<level_type>(&d_part));
    }

    /** The element node. Precondition: is_node(). */
    [[nodiscard]] auto node() const& noexcept -> const node_type& {
        assert(is_node() && "FingerTreePart::node: the part is a spine level");
        return *std::get_if<node_type>(&d_part);
    }
    [[nodiscard]] auto node() && -> node_type {
        assert(is_node() && "FingerTreePart::node: the part is a spine level");
        return std::move(*std::get_if<node_type>(&d_part));
    }

    /** The cached measure of everything this part holds. */
    [[nodiscard]] auto measure() const -> tag_type { return is_level() ? level().measure() : node().measure(); }

    /** This part as a tree. Precondition: is_level() and
     * level().is_sequence(). */
    [[nodiscard]] auto sequence() const& -> TREE { return level().sequence(); }
    [[nodiscard]] auto sequence() && -> TREE { return std::move(*this).level().sequence(); }

  private:
    std::variant<level_type, node_type> d_part;
};

/** The layer a projection yields for @p TREE: payloads shared, positions
 * held as parts. */
template <typename TREE>
using FingerTreeLayerOf = FingerTreeF<FingerPayloadRef<typename TREE::value_type>, FingerTreePart<TREE>>;

// ---------------------------------------------------------------------
// Projection and embedding: the direct verbs' two ingredients.
// ---------------------------------------------------------------------

namespace detail {

// One layer of @p part, with @p leaf_slot choosing how a Leaf holds its
// payload. Both projections below are this function.
template <typename P, typename TREE, typename LeafSlot>
auto project_layer(const FingerTreePart<TREE>& part, const LeafSlot& leaf_slot)
    -> FingerTreeF<P, FingerTreePart<TREE>> {
    using Part = FingerTreePart<TREE>;
    using fingertree::tree::FingerTreeForm;
    using fingertree::tree::FingerTreeNodeForm;
    const auto slot = [](auto&& piece) {
        return tree_algorithms::make_slot<Part>(Part(std::forward<decltype(piece)>(piece)));
    };

    if (part.is_node()) {
        const auto& node = part.node();
        if (node.form() == FingerTreeNodeForm::LEAF)
            return FingerLeaf<P>{leaf_slot(node)};
        auto children = node.children();
        if (node.form() == FingerTreeNodeForm::NODE2)
            return FingerNode2<Part>{slot(std::move(children[0])), slot(std::move(children[1]))};
        return FingerNode3<Part>{
            slot(std::move(children[0])), slot(std::move(children[1])), slot(std::move(children[2]))};
    }

    const auto& level = part.level();
    if (level.form() == FingerTreeForm::EMPTY)
        return FingerEmpty{};
    if (level.form() == FingerTreeForm::SINGLE)
        return FingerSingle<Part>{slot(level.element())};

    FingerDeep<Part> deep;
    for (auto& node : level.left())
        deep.left.push_back(slot(std::move(node)));
    if (auto spine = level.spine(); !spine.empty())
        deep.spine = slot(std::move(spine));
    for (auto& node : level.right())
        deep.right.push_back(slot(std::move(node)));
    return deep;
}

} // namespace detail

/** Projection for fold_with: exposes one layer of a finger tree as it is
 * stored. Positions are parts; a Leaf shares its payload. Nothing is
 * copied except reference counts.
 *
 * The pointer overload is the root of a fold in the tree's own
 * representation, the shape tree_algorithms::project registrations take.
 */
struct FingerTreeProjectFn {
    template <typename TREE>
    auto operator()(const FingerTreePart<TREE>& part) const -> FingerTreeLayerOf<TREE> {
        using Payload = FingerPayloadRef<typename TREE::value_type>;
        return detail::project_layer<Payload>(part, [](const auto& node) { return Payload(node.share_value()); });
    }

    template <typename T, typename MONOID, typename MP, typename ALLOCATOR>
    auto operator()(const fingertree::tree::FingerTree<T, MONOID, MP, ALLOCATOR>* tree) const
        -> FingerTreeLayerOf<fingertree::tree::FingerTree<T, MONOID, MP, ALLOCATOR>> {
        return (*this)(FingerTreePart<fingertree::tree::FingerTree<T, MONOID, MP, ALLOCATOR>>(*tree));
    }
};

inline constexpr FingerTreeProjectFn finger_tree_project{};

/** Projection that copies each payload into its layer. This is the
 * coalgebra of a conversion that must own what it produces, to_fix below.
 * Requires a copyable payload. */
struct FingerTreeProjectOwnedFn {
    template <typename TREE>
        requires std::copy_constructible<typename TREE::value_type>
    auto operator()(const FingerTreePart<TREE>& part) const
        -> FingerTreeF<typename TREE::value_type, FingerTreePart<TREE>> {
        using T = typename TREE::value_type;
        return detail::project_layer<T>(part, [](const auto& node) -> const T& { return node.value(); });
    }
};

inline constexpr FingerTreeProjectOwnedFn finger_tree_project_owned{};

/** Embedding for unfold_with: rebuilds one layer in the tree's own
 * representation, under one allocator and one monoid instance. Every node
 * is constructed by the tree's structural builder, which computes its
 * cached measure, so a tree an unfold builds satisfies the measure
 * invariant by construction.
 *
 * Preconditions, which the layers of a well-formed finger tree satisfy and
 * a hand-written coalgebra must: every engaged position holds the sort its
 * place requires (elements in a Single, in digits and under nodes; a tree
 * in a Deep's spine); a Single's element and every digit are engaged;
 * digits hold one to four elements of one height; a Deep's spine is one
 * level below its digits; a node's children have one height.
 * @tparam TREE the FingerTree specialization to build
 */
template <typename TREE>
class FingerTreeEmbed {
  public:
    using tree_type    = TREE;
    using value_type   = typename TREE::value_type;
    using part_type    = FingerTreePart<TREE>;
    using builder_type = fingertree::tree::FingerTreeStructureBuilder<TREE>;

    /** Build with the allocator and monoid instances of @p prototype. */
    explicit FingerTreeEmbed(const TREE& prototype) : d_builder(prototype) {}

    explicit FingerTreeEmbed(builder_type builder = builder_type{}) : d_builder(std::move(builder)) {}

    [[nodiscard]] auto builder() const noexcept -> const builder_type& { return d_builder; }

    /** Embed a layer the caller keeps: positions are shared, and an owned
     * payload is copied. */
    template <typename P>
        requires detail::payload_source<const P&, value_type>
    auto operator()(const FingerTreeF<P, part_type>& layer) const -> part_type {
        return embed<P>(layer);
    }

    /** Embed a layer the caller gives up: an owned payload is moved. */
    template <typename P>
        requires detail::payload_source<P&&, value_type>
    auto operator()(FingerTreeF<P, part_type>&& layer) const -> part_type {
        return embed<P>(std::move(layer));
    }

  private:
    builder_type d_builder;

    using slot_type  = tree_algorithms::child_slot_t<part_type>;
    using level_type = typename part_type::level_type;
    using node_type  = typename part_type::node_type;

    template <typename SLOT>
    static auto node(SLOT&& slot) -> node_type {
        assert(slot && slot->is_node() && "FingerTreeEmbed: an element position does not hold an element");
        return std::forward_like<SLOT>(*slot).node();
    }

    template <typename DIGIT>
    static auto digit(DIGIT&& source) -> typename level_type::digit_type {
        typename level_type::digit_type out;
        for (auto& slot : source)
            out.push_back(node(std::forward_like<DIGIT>(slot)));
        return out;
    }

    // An if-chain and not a visit: the alternatives are forwarded as the
    // layer was handed over, and transpose::overloaded cannot rank a
    // forwarding lambda against its own catch-all.
    template <typename P, typename LAYER>
    auto embed(LAYER&& layer) const -> part_type {
        if (auto* single = std::get_if<FingerSingle<part_type>>(&layer))
            return part_type(d_builder.single(node(std::forward_like<LAYER>(single->element))));
        if (auto* deep = std::get_if<FingerDeep<part_type>>(&layer)) {
            auto left = digit(std::forward_like<LAYER>(deep->left));
            auto next = d_builder.empty();
            if (deep->spine) {
                assert(deep->spine->is_level() && "FingerTreeEmbed: a spine position does not hold a tree");
                next = std::forward_like<LAYER>(*deep->spine).level();
            }
            auto right = digit(std::forward_like<LAYER>(deep->right));
            return part_type(d_builder.deep(std::move(left), std::move(next), std::move(right)));
        }
        if (auto* leaf = std::get_if<FingerLeaf<P>>(&layer))
            return part_type(d_builder.leaf(detail::make_payload<value_type>(std::forward_like<LAYER>(leaf->value))));
        if (auto* node2 = std::get_if<FingerNode2<part_type>>(&layer))
            return part_type(
                d_builder.node2(node(std::forward_like<LAYER>(node2->a)), node(std::forward_like<LAYER>(node2->b))));
        if (auto* node3 = std::get_if<FingerNode3<part_type>>(&layer))
            return part_type(d_builder.node3(node(std::forward_like<LAYER>(node3->a)),
                                             node(std::forward_like<LAYER>(node3->b)),
                                             node(std::forward_like<LAYER>(node3->c))));
        return part_type(d_builder.empty());
    }
};

// ---------------------------------------------------------------------
// Elementwise layer fold (for fold_map).
// ---------------------------------------------------------------------

/** Folds one FingerTreeF layer elementwise. The contractual order is the
 * sequence order: a Deep combines its left digit, its spine, then its
 * right digit, each left to right, and a node combines its children left
 * to right. A Leaf is one mapped payload. Only an Empty layer yields the
 * identity; no other form combines with it. */
struct FingerTreeLayerFoldMap {
    template <typename MapFn, typename Combine, typename Result, typename P>
    constexpr auto operator()(const MapFn&                  map_fn,
                              const Combine&                combine,
                              const Result&                 identity,
                              const FingerTreeF<P, Result>& layer) const -> Result {
        return std::visit(
            transpose::overloaded{
                [&](const FingerEmpty&) -> Result { return identity; },
                [&](const FingerSingle<Result>& s) -> Result { return *s.element; },
                [&](const FingerDeep<Result>& d) -> Result {
                    std::optional<Result> acc;
                    const auto            step = [&](const Result& next) {
                        if (acc)
                            acc.emplace(combine(*acc, next));
                        else
                            acc.emplace(next);
                    };
                    for (const auto& position : d.left)
                        step(*position);
                    if (d.spine)
                        step(*d.spine);
                    for (const auto& position : d.right)
                        step(*position);
                    return acc ? std::move(*acc) : identity;
                },
                [&](const FingerLeaf<P>& l) -> Result { return map_fn(finger_payload(l.value)); },
                [&](const FingerNode2<Result>& n) -> Result { return combine(*n.a, *n.b); },
                [&](const FingerNode3<Result>& n) -> Result { return combine(combine(*n.a, *n.b), *n.c); },
            },
            layer);
    }
};

inline constexpr FingerTreeLayerFoldMap finger_tree_layer_fold_map{};

// ---------------------------------------------------------------------
// The Fix form.
// ---------------------------------------------------------------------

/** Binds the payload type of FingerTreeF, leaving the unary-in-A alias
 * template @c F that Fix and the verbs require. The layers of the Fix form
 * own their payloads. */
template <typename T>
struct FingerTreeLayer {
    template <typename A>
    using F = FingerTreeF<T, A>;
};

/** The Fix form of a finger tree over @p T: the same layers, tied by Fix
 * and owning their structure. The measure, the monoid and the allocator are
 * not part of it; from_fix supplies them again through its embedding. */
template <typename T>
using FingerTreeFix = tree_algorithms::Fix<FingerTreeLayer<T>::template F>;

/** Convert a finger tree to its Fix form, layer by layer. Deep copy: the
 * result owns its payloads and shares nothing with @p tree. */
template <typename T, typename MONOID, typename MP, typename ALLOCATOR>
    requires std::copy_constructible<T>
auto to_fix(const fingertree::tree::FingerTree<T, MONOID, MP, ALLOCATOR>& tree) -> FingerTreeFix<T> {
    using Tree = fingertree::tree::FingerTree<T, MONOID, MP, ALLOCATOR>;
    return tree_algorithms::unfold_fix<FingerTreeLayer<T>::template F>(finger_tree_project_owned,
                                                                       FingerTreePart<Tree>(tree));
}

/** Rebuild a finger tree from its Fix form with @p embed, which supplies
 * the allocator and monoid and recomputes every measure. A fold: the
 * algebra is the embedding. Precondition: @p fixed is the Fix form of a
 * whole tree, as to_fix produces. */
template <typename TREE>
    requires std::copy_constructible<typename TREE::value_type>
auto from_fix(const FingerTreeFix<typename TREE::value_type>& fixed, const FingerTreeEmbed<TREE>& embed) -> TREE {
    return tree_algorithms::fold_fix<FingerTreePart<TREE>>(embed, fixed).sequence();
}

} // namespace beman::typeclass_tree_adapters

namespace beman::tree_algorithms {

/** Lookup registrations: the projection keyed on the tree type, the layer
 * fold keyed on the layer type. */
template <typename T, typename MONOID, typename MP, typename ALLOCATOR>
inline constexpr auto project<beman::fingertree::tree::FingerTree<T, MONOID, MP, ALLOCATOR>> =
    beman::typeclass_tree_adapters::finger_tree_project;

template <typename P, typename A>
inline constexpr auto layer_fold<beman::typeclass_tree_adapters::FingerTreeF<P, A>> =
    beman::typeclass_tree_adapters::finger_tree_layer_fold_map;

} // namespace beman::tree_algorithms

#endif // BEMAN_TYPECLASS_TREE_ADAPTERS_FINGER_TREE_HPP
