// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_FINGERTREE_TREE_FINGER_TREE_STRUCTURE_HPP
#define BEMAN_FINGERTREE_TREE_FINGER_TREE_STRUCTURE_HPP

// Structural access to a FingerTree: the representation as it is.
//
// The sequence interface of FingerTree deliberately hides the tree's shape;
// cons, snoc, split and append are free to rebalance it. A structural
// algorithm needs the opposite: the real Empty/Single/Deep forms of the
// spine and the real Leaf/Node2/Node3 forms of its elements, with digit
// order and spine position preserved. This header is that access and nothing
// else. It names no algorithm library, so any structural consumer can be
// written against it; beman.tree_algorithms is reached through the optional
// beman/typeclass_tree_adapters/finger_tree.hpp.
//
// The representation has two recursive sorts, and both are the same C++
// type at every depth:
//
//   FingerTreeLevel<TREE>     the tree at one level of the spine. Level 0 is
//                             the sequence; level d holds nodes of height d.
//   FingerTreeNode<T, MONOID> one element: a Leaf (height 0) or a Node2 or
//                             Node3 of nodes one lower.
//
// Both are owning values. They share the immutable structure they describe,
// so copying one is O(1), no payload is copied, and neither can outlive what
// it refers to. Reading a level or a node hands out more of the same.
//
// FingerTreeStructureBuilder<TREE> constructs the same forms under one
// allocator and one monoid instance, and computes every cached measure. A
// level that is not a sequence never escapes as a TREE: sequence() is the
// only way out, and its precondition is that the level is one.

#include <beman/fingertree/tree/finger_tree.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <inplace_vector>
#include <memory>
#include <utility>
#include <variant>

namespace beman::fingertree::tree {

namespace ft {

template <typename TREE>
inline constexpr bool is_finger_tree = false;

template <typename T, typename MONOID, typename MP, typename ALLOCATOR>
inline constexpr bool is_finger_tree<FingerTree<T, MONOID, MP, ALLOCATOR>> = true;

} // namespace ft

/** The form of the tree at one level of the spine. */
enum class FingerTreeForm : std::uint8_t { EMPTY, SINGLE, DEEP };

/** The form of one element. */
enum class FingerTreeNodeForm : std::uint8_t { LEAF, NODE2, NODE3 };

/** One element of a finger tree: a Leaf holding a payload, or a Node2 or
 * Node3 of nodes one lower. Owns a share of the immutable element, so a
 * node stays valid for as long as it exists.
 * @tparam T      payload type
 * @tparam MONOID the monoid instance type the cached measure was computed
 *                under
 */
template <typename T, typename MONOID>
class FingerTreeNode {
    using E  = ft::Elem<T, MONOID>;
    using EP = ft::ElemPtr<T, MONOID>;

    EP          d_elem;
    std::size_t d_height;

    FingerTreeNode(EP elem, std::size_t height) noexcept : d_elem(std::move(elem)), d_height(height) {}

    template <typename>
    friend class FingerTreeLevel;
    template <typename>
    friend class FingerTreeStructureBuilder;

  public:
    using value_type    = T;
    using tag_type      = ft::tag_t<MONOID>;
    using monoid_type   = MONOID;
    using children_type = std::inplace_vector<FingerTreeNode, 3>;

    [[nodiscard]] auto form() const noexcept -> FingerTreeNodeForm {
        return static_cast<FingerTreeNodeForm>(d_elem->d_data.index());
    }

    /** Levels of Node2/Node3 above the leaves: 0 for a Leaf. */
    [[nodiscard]] auto height() const noexcept -> std::size_t { return d_height; }

    /** The cached measure of everything below this node. */
    [[nodiscard]] auto measure() const noexcept -> const tag_type& { return d_elem->d_measure; }

    /** The payload. Precondition: form() == LEAF. */
    [[nodiscard]] auto value() const noexcept -> const T& {
        const auto* leaf = std::get_if<typename E::Leaf>(&d_elem->d_data);
        assert(leaf != nullptr && "FingerTreeNode::value: not a leaf");
        return leaf->d_value;
    }

    /** The payload, sharing ownership of the leaf that stores it: nothing
     * is copied, and the result outlives this node and the tree.
     * Precondition: form() == LEAF. */
    [[nodiscard]] auto share_value() const noexcept -> std::shared_ptr<const T> {
        return std::shared_ptr<const T>(d_elem, std::addressof(value()));
    }

    /** The child nodes in sequence order: none for a Leaf, two for a Node2,
     * three for a Node3. Each has height() one lower than this node. */
    [[nodiscard]] auto children() const -> children_type {
        children_type out;
        if (const auto* n2 = std::get_if<typename E::Node2>(&d_elem->d_data)) {
            out.push_back(FingerTreeNode(n2->a, d_height - 1));
            out.push_back(FingerTreeNode(n2->b, d_height - 1));
        } else if (const auto* n3 = std::get_if<typename E::Node3>(&d_elem->d_data)) {
            out.push_back(FingerTreeNode(n3->a, d_height - 1));
            out.push_back(FingerTreeNode(n3->b, d_height - 1));
            out.push_back(FingerTreeNode(n3->c, d_height - 1));
        }
        return out;
    }
};

/** The tree at one level of a finger tree's spine, in its real form: Empty,
 * Single, or Deep with a left digit, the next level, and a right digit.
 * Level 0 is the sequence itself; the nodes held at level d have height d.
 * Owns a share of the immutable structure.
 * @tparam TREE a FingerTree specialization
 */
template <typename TREE>
class FingerTreeLevel {
    static_assert(ft::is_finger_tree<TREE>, "FingerTreeLevel requires a FingerTree specialization");

    TREE        d_tree;
    std::size_t d_level = 0;

    FingerTreeLevel(TREE tree, std::size_t level) : d_tree(std::move(tree)), d_level(level) {}

    template <typename>
    friend class FingerTreeStructureBuilder;

  public:
    using tree_type      = TREE;
    using value_type     = typename TREE::value_type;
    using tag_type       = typename TREE::tag_type;
    using monoid_type    = typename TREE::monoid_type;
    using allocator_type = typename TREE::allocator_type;
    using node_type      = FingerTreeNode<value_type, monoid_type>;
    using digit_type     = std::inplace_vector<node_type, 4>;

    /** Level 0 of @p sequence: shares its structure, copies no element. */
    explicit FingerTreeLevel(const TREE& sequence) : d_tree(sequence) {}

    [[nodiscard]] auto form() const noexcept -> FingerTreeForm {
        return static_cast<FingerTreeForm>(d_tree.d_repr.index());
    }

    [[nodiscard]] auto empty() const noexcept -> bool { return d_tree.empty(); }

    /** The height of the nodes this level holds. An empty level holds none
     * and fits at any depth; its level() is where it was read from or 0. */
    [[nodiscard]] auto level() const noexcept -> std::size_t { return d_level; }

    /** The cached measure of this level and everything below it. */
    [[nodiscard]] auto measure() const -> tag_type { return d_tree.measure(); }

    /** The one node of a Single. Precondition: form() == SINGLE. */
    [[nodiscard]] auto element() const -> node_type {
        const auto* single = std::get_if<typename TREE::Single>(&d_tree.d_repr);
        assert(single != nullptr && "FingerTreeLevel::element: not a Single");
        return node_type(single->d_elem, d_level);
    }

    /** The left digit of a Deep, one to four nodes in sequence order.
     * Precondition: form() == DEEP. */
    [[nodiscard]] auto left() const -> digit_type { return digit(deep().d_left); }

    /** The right digit of a Deep, one to four nodes in sequence order.
     * Precondition: form() == DEEP. */
    [[nodiscard]] auto right() const -> digit_type { return digit(deep().d_right); }

    /** The next level of a Deep, which lies between its digits. A Deep that
     * stores no spine yields an empty level. Precondition: form() == DEEP. */
    [[nodiscard]] auto spine() const -> FingerTreeLevel {
        const auto& d = deep();
        if (d.d_spine)
            return FingerTreeLevel(*d.d_spine, d_level + 1);
        return FingerTreeLevel(TREE(d_tree.get_allocator(), d_tree.get_monoid()), d_level + 1);
    }

    /** True when this level is usable as a tree: it is empty, or it holds
     * leaves. */
    [[nodiscard]] auto is_sequence() const noexcept -> bool { return empty() || d_level == 0; }

    /** This level as a tree. Precondition: is_sequence(). */
    [[nodiscard]] auto sequence() const& -> TREE {
        assert(is_sequence() && "FingerTreeLevel::sequence: an inner spine level is not a sequence");
        return d_tree;
    }

    [[nodiscard]] auto sequence() && -> TREE {
        assert(is_sequence() && "FingerTreeLevel::sequence: an inner spine level is not a sequence");
        return std::move(d_tree);
    }

  private:
    auto deep() const -> const typename TREE::Deep& {
        const auto* d = std::get_if<typename TREE::DeepPtr>(&d_tree.d_repr);
        assert(d != nullptr && "FingerTreeLevel: not a Deep");
        return **d;
    }

    auto digit(const typename TREE::Digit& source) const -> digit_type {
        digit_type out;
        for (const auto& elem : source)
            out.push_back(node_type(elem, d_level));
        return out;
    }
};

/** Constructs the real forms of a finger tree under one allocator and one
 * monoid instance, computing every cached measure as it goes.
 *
 * The parts combined by one builder must share its configuration: built by
 * it, by an equal builder, or read from a tree with an equal allocator and
 * the same measure law. A cached measure is valid only under the monoid
 * that computed it; the builder cannot check that, so it is a precondition.
 * @tparam TREE a FingerTree specialization
 */
template <typename TREE>
class FingerTreeStructureBuilder {
    static_assert(ft::is_finger_tree<TREE>, "FingerTreeStructureBuilder requires a FingerTree specialization");

  public:
    using tree_type      = TREE;
    using value_type     = typename TREE::value_type;
    using monoid_type    = typename TREE::monoid_type;
    using allocator_type = typename TREE::allocator_type;
    using level_type     = FingerTreeLevel<TREE>;
    using node_type      = typename level_type::node_type;
    using digit_type     = typename level_type::digit_type;

    /** A builder with the allocator and monoid instances of @p prototype. */
    explicit FingerTreeStructureBuilder(const TREE& prototype)
        : d_alloc(prototype.get_allocator()), d_monoid(prototype.get_monoid()) {}

    explicit FingerTreeStructureBuilder(const allocator_type& alloc  = allocator_type{},
                                        const monoid_type&    monoid = monoid_type{})
        : d_alloc(alloc), d_monoid(monoid) {}

    [[nodiscard]] auto get_allocator() const noexcept -> allocator_type { return d_alloc; }
    [[nodiscard]] auto get_monoid() const noexcept -> monoid_type { return d_monoid; }

    /** A Leaf holding @p value, measured by the tree's measure policy. */
    [[nodiscard]] auto leaf(value_type value) const -> node_type {
        return node_type(ft::make_leaf<value_type, monoid_type>(d_alloc, TREE::meas_fn(), std::move(value)), 0);
    }

    /** A Node2 of @p a then @p b. Precondition: equal heights. */
    [[nodiscard]] auto node2(node_type a, node_type b) const -> node_type {
        assert(a.d_height == b.d_height && "FingerTreeStructureBuilder::node2: children differ in height");
        const auto height = a.d_height + 1;
        return node_type(
            ft::make_node2<value_type, monoid_type>(d_alloc, d_monoid, std::move(a.d_elem), std::move(b.d_elem)),
            height);
    }

    /** A Node3 of @p a, @p b, then @p c. Precondition: equal heights. */
    [[nodiscard]] auto node3(node_type a, node_type b, node_type c) const -> node_type {
        assert(a.d_height == b.d_height && b.d_height == c.d_height &&
               "FingerTreeStructureBuilder::node3: children differ in height");
        const auto height = a.d_height + 1;
        return node_type(ft::make_node3<value_type, monoid_type>(
                             d_alloc, d_monoid, std::move(a.d_elem), std::move(b.d_elem), std::move(c.d_elem)),
                         height);
    }

    /** The empty level. It fits at any depth. */
    [[nodiscard]] auto empty() const -> level_type { return level_type(TREE(d_alloc, d_monoid), 0); }

    /** A Single holding @p element; its level is the element's height. */
    [[nodiscard]] auto single(node_type element) const -> level_type {
        const auto level = element.d_height;
        return level_type(TREE::make_single(d_alloc, d_monoid, std::move(element.d_elem)), level);
    }

    /** A Deep of @p left, @p spine, then @p right.
     *
     * Preconditions: each digit holds one to four nodes; every node of both
     * digits has one height h; @p spine is empty or is at level h + 1 and
     * was allocated by an equal allocator. An empty @p spine is stored as
     * no spine, which is the representation's own form.
     */
    [[nodiscard]] auto deep(digit_type left, level_type spine, digit_type right) const -> level_type {
        assert(!left.empty() && !right.empty() && "FingerTreeStructureBuilder::deep: empty digit");
        const auto level = left.front().d_height;
        assert(uniform(left, level) && uniform(right, level) &&
               "FingerTreeStructureBuilder::deep: digit nodes differ in height");
        assert((spine.empty() || spine.d_level == level + 1) &&
               "FingerTreeStructureBuilder::deep: spine is not one level below the digits");

        typename TREE::SpinePtr next;
        if (!spine.empty()) {
            assert(TREE::alloc_equal(d_alloc, spine.d_tree.get_allocator()) &&
                   "FingerTreeStructureBuilder::deep: spine was allocated by another resource");
            next = TREE::allocate_spine(d_alloc, std::move(spine.d_tree));
        }
        return level_type(
            TREE::make_deep(d_alloc, d_monoid, elems(std::move(left)), std::move(next), elems(std::move(right))),
            level);
    }

  private:
    [[no_unique_address]] allocator_type d_alloc;
    [[no_unique_address]] monoid_type    d_monoid;

    static auto uniform(const digit_type& digit, std::size_t height) noexcept -> bool {
        for (const auto& node : digit) {
            if (node.d_height != height)
                return false;
        }
        return true;
    }

    static auto elems(digit_type&& digit) -> typename TREE::Digit {
        typename TREE::Digit out;
        for (auto& node : digit)
            out.push_back(std::move(node.d_elem));
        return out;
    }
};

} // namespace beman::fingertree::tree

#endif
