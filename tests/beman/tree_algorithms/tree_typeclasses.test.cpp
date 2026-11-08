// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include "../collection_test_support.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <beman/transpose/string.hpp>
#include <beman/tree_algorithms/tree_typeclasses.hpp>
#include <beman/tree_algorithms/tree_typeclasses.hpp> // Re-inclusion check.
#include <beman/tree_algorithms/functors.hpp>
#include "../../../examples/transpose/binary_tree.hpp"

namespace {
namespace ta = beman::tree_algorithms;
namespace bt = beman::transpose;
namespace ct = collection_test;
using Binary = ta::BinaryTree<int>;
using Fixed  = ta::BinaryTreeFix<int>;
using Fringe = ta::FringeTree<int>;
using Rose   = ta::RoseTreeFix<int>;

template <class T>
using Functor = std::remove_cvref_t<decltype(bt::functor<T>)>;
template <class T>
using Foldable = std::remove_cvref_t<decltype(bt::foldable<T>)>;
template <class T>
using Traversable = std::remove_cvref_t<decltype(bt::traversable<T>)>;

template <class T>
auto source() -> T;
template <>
auto source<Binary>() -> Binary {
    return Binary::from_children_ptrs(
        2,
        Binary::make_ptr(Binary::leaf(1)),
        Binary::make_ptr(Binary::from_children_ptrs(4, Binary::make_ptr(Binary::leaf(3)), nullptr)));
}
constexpr auto fixed_source() -> Fixed {
    return Fixed{ta::BinaryTreeF<int, Fixed>{
        2,
        ta::make_slot<Fixed>(Fixed{ta::BinaryTreeF<int, Fixed>{1, {}, {}}}),
        ta::make_slot<Fixed>(Fixed{ta::BinaryTreeF<int, Fixed>{
            4, ta::make_slot<Fixed>(Fixed{ta::BinaryTreeF<int, Fixed>{3, {}, {}}}), {}}})}};
}
template <>
auto source<Fixed>() -> Fixed {
    return fixed_source();
}
template <>
auto source<Fringe>() -> Fringe {
    return Fringe::branch(Fringe::branch(Fringe::leaf(1), Fringe::leaf(2)),
                          Fringe::branch(Fringe::leaf(3), Fringe::leaf(4)));
}
constexpr auto rose_source() -> Rose {
    return ta::rose<int>(1, {ta::rose<int>(2), ta::rose<int>(3, {ta::rose<int>(4)})});
}
template <>
auto source<Rose>() -> Rose {
    return rose_source();
}

template <class T>
auto show(const T& value) -> std::string {
    if constexpr (std::same_as<T, std::string>)
        return value;
    else
        return std::to_string(value);
}
struct Show {
    template <class T>
    auto operator()(const T& value) & -> std::string {
        return show(value);
    }
};
struct StringEffect {
    template <class T>
    auto operator()(const T& value) & -> std::optional<std::string> {
        return show(value);
    }
};
struct NotCallable {};
struct RvalueOnly {
    auto operator()(const int&) && -> int;
};
struct VoidResult {
    auto operator()(const int&) const -> void;
};
struct ArrayResult {
    auto operator()(const int&) const -> int (&)[2];
};
struct WrongIdentity {
    auto identity() const -> int;
    auto combine(const std::string&, const std::string&) const -> std::string;
};
struct WrongCombine {
    auto identity() const -> std::string;
    auto combine(const std::string&, const std::string&) const -> int;
};

template <class Tree, class F>
concept mappable = requires(const Tree& tree, F&& fn) { bt::fmap(std::forward<F>(fn), tree); };
template <class Tree, class F>
concept object_mappable = requires(const Tree& tree, F&& fn) { bt::functor<Tree>.fmap(std::forward<F>(fn), tree); };
template <class Tree, class F>
concept traversable = requires(const Tree& tree, F&& fn) { bt::traverse(std::forward<F>(fn), tree); };
template <class Tree, class F>
concept object_traversable = requires(const Tree& tree, F&& fn) {
    bt::traversable<Tree>.traverse(bt::applicative<std::optional<std::string>>, std::forward<F>(fn), tree);
};
template <class Tree, class F, class M>
concept foldable = requires(const Tree& tree, F&& fn, const M& monoid) {
    bt::fold_map(std::forward<F>(fn), tree, monoid);
    ta::fold_map(std::forward<F>(fn), tree, monoid);
    bt::foldable<Tree>.fold_map(std::forward<F>(fn), tree, monoid);
};
template <class Tree, class F, class M>
concept rejected_fold =
    (!requires(const Tree& tree, F&& fn, const M& monoid) { bt::fold_map(std::forward<F>(fn), tree, monoid); }) &&
    (!requires(const Tree& tree, F&& fn, const M& monoid) { ta::fold_map(std::forward<F>(fn), tree, monoid); }) &&
    (!requires(const Tree& tree, F&& fn, const M& monoid) {
        bt::foldable<Tree>.fold_map(std::forward<F>(fn), tree, monoid);
    });

// Full structural signatures distinguish sparse children and tree shape.
// Fixed signatures use the existing structural verb rather than the new
// payload recorder/reconstruction implementation.
template <class T>
auto signature(const ta::BinaryTree<T>& tree) -> std::string {
    return "(" + (tree.has_left() ? signature(tree.left()) : "-") + "," + show(tree.value()) + "," +
           (tree.has_right() ? signature(tree.right()) : "-") + ")";
}
template <class T>
auto signature(const ta::FringeTree<T>& tree) -> std::string {
    if (tree.is_empty())
        return "-";
    if (tree.is_leaf())
        return show(tree.value());
    return "(" + signature(tree.left()) + "," + signature(tree.right()) + ")";
}
template <template <class> class F>
auto signature(const ta::Fix<F>& tree) -> std::string {
    return ta::fold_fix<std::string>(
        [](const auto& layer) {
            if constexpr (requires { layer.left; }) {
                return "(" + layer.left.value_or("-") + "," + show(layer.value) + "," + layer.right.value_or("-") +
                       ")";
            } else {
                std::string out = "(" + show(layer.value);
                for (const auto& child : layer.children)
                    out += "," + child;
                return out + ")";
            }
        },
        tree);
}

// Stateful, noncommutative monoid and noncopyable, lvalue-only mapper.
struct Join {
    using value_type = std::string;
    std::string separator;
    auto        identity() const -> std::string { return {}; }
    auto        combine(const std::string& a, const std::string& b) const -> std::string {
        if (a.empty())
            return b;
        if (b.empty())
            return a;
        return a + separator + b;
    }
};
struct StatefulShow {
    int calls                         = 0;
    StatefulShow()                    = default;
    StatefulShow(const StatefulShow&) = delete;
    auto operator()(const int& value) & -> std::string {
        ++calls;
        return std::to_string(value * 10 + 1);
    }
};

constexpr auto constexpr_payload_laws() -> bool {
    auto fixed       = fixed_source();
    auto mapped      = bt::fmap([](const int& n) { return n * 10 + 1; }, fixed);
    auto effect      = bt::traverse([](const int& n) { return std::optional{n * 10 + 1}; }, fixed);
    auto digits      = [](const int& n) { return std::string(1, static_cast<char>('0' + n)); };
    auto rose        = rose_source();
    auto rose_map    = bt::fmap([](const int& n) { return n - 1; }, rose);
    auto rose_effect = bt::traverse([](const int& n) { return std::optional{n - 1}; }, rose);
    return mapped.inner.value == 21 && effect->inner.right->inner.left->inner.value == 31 &&
           bt::fold_map(digits, fixed, bt::monoid<std::string>) == "1234" &&
           ta::fold_map(digits, rose, bt::monoid<std::string>) == "1234" &&
           rose_map.inner.children[1].inner.children[0].inner.value == 3 && rose_effect->inner.value == 0 &&
           bt::replace(rose, 7).inner.value == 7 &&
           bt::length(ta::add_node(ta::const_node(1), ta::const_node(2))) == 2;
}
static_assert(constexpr_payload_laws());
} // namespace

TEMPLATE_TEST_CASE("Payload tree objects conform and reject unsupported calls",
                   "",
                   Binary,
                   Fixed,
                   Fringe,
                   Rose,
                   ta::BinaryTree<std::string>,
                   ta::BinaryTreeFix<std::string>,
                   ta::FringeTree<std::string>,
                   ta::RoseTreeFix<std::string>) {
    using M        = std::remove_cvref_t<decltype(bt::monoid<std::string>)>;
    using Expected = std::conditional_t<std::same_as<TestType, Binary> || std::same_as<TestType, Fixed> ||
                                            std::same_as<TestType, Fringe> || std::same_as<TestType, Rose>,
                                        int,
                                        std::string>;
    STATIC_REQUIRE(std::same_as<bt::applicative_value_t<TestType>, Expected>);
    STATIC_REQUIRE(bt::functor_object<Functor<TestType>, TestType>);
    STATIC_REQUIRE(bt::foldable_object<Foldable<TestType>, TestType>);
    STATIC_REQUIRE(bt::traversable_object<Traversable<TestType>, TestType>);
    STATIC_REQUIRE(mappable<TestType, Show>);
    STATIC_REQUIRE(object_mappable<TestType, Show>);
    STATIC_REQUIRE(traversable<TestType, StringEffect>);
    STATIC_REQUIRE(object_traversable<TestType, StringEffect>);
    STATIC_REQUIRE(foldable<TestType, Show, M>);
    STATIC_REQUIRE(!mappable<TestType, NotCallable>);
    STATIC_REQUIRE(!object_mappable<TestType, NotCallable>);
    STATIC_REQUIRE(!mappable<TestType, RvalueOnly>);
    STATIC_REQUIRE(!object_mappable<TestType, VoidResult>);
    STATIC_REQUIRE(!mappable<TestType, ArrayResult>);
    STATIC_REQUIRE(!traversable<TestType, RvalueOnly>);
    STATIC_REQUIRE(!object_traversable<TestType, NotCallable>);
    STATIC_REQUIRE(!traversable<TestType, VoidResult>);
    STATIC_REQUIRE(!object_traversable<TestType, Show>);
    STATIC_REQUIRE(rejected_fold<TestType, NotCallable, M>);
    STATIC_REQUIRE(rejected_fold<TestType, RvalueOnly, M>);
    STATIC_REQUIRE(rejected_fold<TestType, VoidResult, M>);
    STATIC_REQUIRE(rejected_fold<TestType, ArrayResult, M>);
    STATIC_REQUIRE(rejected_fold<TestType, Show, WrongIdentity>);
    STATIC_REQUIRE(rejected_fold<TestType, Show, WrongCombine>);
}

TEMPLATE_TEST_CASE(
    "Payload folds agree with structural folds and retain passed state", "", Binary, Fixed, Fringe, Rose) {
    const auto   tree = source<TestType>();
    StatefulShow fn;
    const Join   monoid{"/"};
    CHECK(bt::fold_map(fn, tree, monoid) == "11/21/31/41");
    CHECK(fn.calls == 4);
    StatefulShow bridge_fn;
    CHECK(ta::fold_map(bridge_fn, tree, monoid) == "11/21/31/41");
    CHECK(bridge_fn.calls == 4);
    auto combine = [](const std::string& a, const std::string& b) { return a + b; };
    CHECK(bt::fold_map(Show{}, tree, bt::monoid<std::string>) ==
          ta::fold_map<std::string>([](const auto& n) { return show(n); }, combine, std::string{}, tree));
    CHECK(bt::fold_left(tree, std::string{}, [](std::string acc, int n) { return acc + show(n); }) == "1234");
    CHECK(bt::fold_right(tree, std::string{}, [](int n, std::string acc) { return acc + show(n); }) == "4321");
    CHECK(bt::length(tree) == 4);
    CHECK(bt::find_first(tree, [](int n) { return n > 1; }) == std::optional{2});
    CHECK(bt::any_of(tree, [](int n) { return n == 4; }));
    CHECK(bt::all_of(tree, [](int n) { return n > 0; }));
    CHECK_FALSE(bt::empty(tree));
}

TEMPLATE_TEST_CASE(
    "Payload mapping and pure traversal preserve shape and obey Functor laws", "", Binary, Fixed, Fringe, Rose) {
    const auto tree     = source<TestType>();
    const auto identity = bt::fmap(std::identity{}, tree);
    STATIC_REQUIRE(std::same_as<std::remove_cvref_t<decltype(identity)>, TestType>);
    CHECK(signature(identity) == signature(tree));
    auto f = [](int n) { return n * 10 + 1; };
    auto g = [](int n) { return std::to_string(n - 3); };
    CHECK(signature(bt::fmap(g, bt::fmap(f, tree))) == signature(bt::fmap([&](int n) { return g(f(n)); }, tree)));
    const auto effect = bt::traverse([&](const int& n) { return std::optional{f(n)}; }, tree);
    REQUIRE(effect.has_value());
    CHECK(signature(*effect) == signature(bt::fmap(f, tree)));
    auto effects    = bt::fmap([](int n) { return std::optional{n}; }, tree);
    auto transposed = bt::transpose(effects);
    auto traversed  = bt::traverse(std::identity{}, effects);
    REQUIRE(transposed.has_value());
    REQUIRE(traversed.has_value());
    CHECK(signature(*transposed) == signature(*traversed));
    CHECK(signature(*transposed) == signature(tree));
    auto units = bt::traverse([](const int&) { return std::optional{std::monostate{}}; }, tree);
    REQUIRE(units.has_value());
    CHECK(bt::length(*units) == 4);
    CHECK(bt::to_vector(bt::replace(tree, 7)) == std::vector<int>{7, 7, 7, 7});
    auto rejected = bt::traverse(
        [](int n) -> std::optional<int> {
            if (n == 3)
                return {};
            return n;
        },
        tree);
    CHECK_FALSE(rejected.has_value());
}

TEMPLATE_TEST_CASE(
    "Tree traversal composes each ordered effect once using the selected policy", "", Binary, Fixed, Fringe, Rose) {
    const auto  tree = source<TestType>();
    std::string visits;
    auto        mapper = [&](const int& n) {
        visits += show(n);
        return ct::Writer<int>{n * 10 + 1, show(n)};
    };
    using Native     = ct::WriterMap<true>;
    Native::collects = 0;
    auto native      = bt::traverse(mapper, tree, Native{});
    CHECK(Native::collects == 1);
    CHECK(visits == "1234");
    CHECK(native.log == "1234");
    visits.clear();
    auto pairwise = bt::traverse(mapper, tree, ct::WriterMap<false>{});
    CHECK(visits == "1234");
    CHECK(pairwise.log == "1234");
    CHECK(signature(native.held) == signature(pairwise.held));
    auto widening = bt::traverse(mapper, tree, ct::WriterMap<true, true>{});
    CHECK(widening.log == "1234");
    CHECK(signature(widening.held) == signature(native.held));
}

TEMPLATE_TEST_CASE(
    "Deferred tree reconstruction owns topology after source destruction", "", Binary, Fixed, Fringe, Rose) {
    std::string log;
    auto        run = [&]<bool Collect>() {
        auto result = [&] {
            auto tree = source<TestType>();
            return bt::traverse(
                [&](const int& n) {
                    return ct::Deferred<int>{[n, &log] {
                        log += show(n);
                        return n * 10 + 1;
                    }};
                },
                tree,
                ct::DeferredMap<Collect>{});
        }();
        CHECK(log.empty());
        auto actual = result.run();
        CHECK(log == "1234");
        CHECK(signature(actual) == signature(bt::fmap([](int n) { return n * 10 + 1; }, source<TestType>())));
        log.clear();
    };
    run.template operator()<false>();
    run.template operator()<true>();
}

TEMPLATE_TEST_CASE(
    "Payload transforms read move-only persistent inputs without consuming them", "", Binary, Fixed, Fringe, Rose) {
    const auto tree   = source<TestType>();
    auto       mapped = bt::fmap([](const int& n) { return std::make_unique<int>(n); }, tree);
    auto       read   = bt::fmap([](const auto& ptr) { return *ptr * 10 + 1; }, std::move(mapped));
    CHECK(bt::to_vector(read) == std::vector<int>{11, 21, 31, 41});
    auto effect = bt::traverse([](const auto& ptr) { return std::optional{std::make_unique<int>(*ptr * 10 + 1)}; },
                               std::move(mapped));
    REQUIRE(effect.has_value());
    auto restored = bt::fmap([](const auto& ptr) { return *ptr; }, *effect);
    CHECK(bt::to_vector(restored) == bt::to_vector(read));
    auto untouched = bt::fmap([](const auto& ptr) { return *ptr; }, mapped);
    CHECK(bt::to_vector(untouched) == std::vector<int>{1, 2, 3, 4});
    using MoveTree = std::remove_cvref_t<decltype(mapped)>;
    using M        = std::remove_cvref_t<decltype(bt::monoid<std::string>)>;
    STATIC_REQUIRE(rejected_fold<MoveTree, Show, M>);
}

TEST_CASE("Empty fringe transformations retain empty topology and native collection") {
    const auto tree = Fringe::empty();
    CHECK(bt::empty(tree));
    CHECK(bt::length(tree) == 0);
    CHECK(bt::fold_map(Show{}, tree, Join{"/"}).empty());
    CHECK(bt::fmap(Show{}, tree).is_empty());
    auto effect = bt::traverse(StringEffect{}, tree);
    REQUIRE(effect.has_value());
    CHECK(effect->is_empty());
    using Native     = ct::WriterMap<true>;
    Native::collects = 0;
    auto result      = bt::traverse([](int n) { return ct::Writer<int>{n, show(n)}; }, tree, Native{});
    CHECK(Native::collects == 1);
    CHECK(result.log.empty());
    CHECK(result.held.is_empty());
}

TEST_CASE("Expr folds constants without claiming arbitrary payload construction") {
    STATIC_REQUIRE(bt::foldable_object<Foldable<ta::Expr>, ta::Expr>);
    STATIC_REQUIRE(!bt::has_instance_v<decltype(bt::functor<ta::Expr>)>);
    STATIC_REQUIRE(!bt::has_instance_v<decltype(bt::traversable<ta::Expr>)>);
    STATIC_REQUIRE(!bt::has_instance_v<decltype(bt::foldable<ta::Nat>)>);
    STATIC_REQUIRE(!bt::has_instance_v<decltype(bt::traversable<ta::Nat>)>);
    const auto tree = ta::mul_node(ta::add_node(ta::const_node(1), ta::const_node(2)), ta::const_node(3));
    CHECK(bt::fold_map(Show{}, tree, Join{"/"}) == "1/2/3");
    CHECK(bt::length(tree) == 3);
    CHECK(bt::to_vector(tree) == std::vector<int>{1, 2, 3});
    CHECK(ta::eval(tree) == 9);
}

TEST_CASE("Independent binary ports agree on sparse topology and ordered effects") {
    using Example       = example::BinaryTree<int>;
    const auto original = Example::from_children_ptrs(
        2,
        Example::make_ptr(Example::leaf(1)),
        Example::make_ptr(Example::from_children_ptrs(4, Example::make_ptr(Example::leaf(3)), nullptr)));
    const auto library = source<Binary>();
    auto       mapper  = [](int n) { return ct::Writer<int>{n * 10 + 1, show(n)}; };
    auto       a       = bt::traverse(mapper, original, ct::WriterMap<true>{});
    auto       b       = bt::traverse(mapper, library, ct::WriterMap<true>{});
    CHECK(a.log == b.log);
    CHECK(a.log == "1234");
    CHECK(bt::to_vector(a.held) == bt::to_vector(b.held));
    CHECK(a.held.has_left() == b.held.has_left());
    CHECK(a.held.right().has_left() == b.held.right().has_left());
    CHECK_FALSE(a.held.right().has_right());
    CHECK_FALSE(b.held.right().has_right());
}

namespace {
struct Tracked {
    inline static int copies = 0;
    int               value;
    explicit Tracked(int n) : value(n) {}
    Tracked(const Tracked& other) : value(other.value) { ++copies; }
    Tracked(Tracked&& other) noexcept : value(std::exchange(other.value, -1)) {}
};
template <class A>
struct UnregisteredLayer {
    A value;
};
using UnregisteredFix = ta::Fix<UnregisteredLayer>;
static_assert(!bt::has_instance_v<decltype(bt::functor<UnregisteredFix>)>);
static_assert(!bt::has_instance_v<decltype(bt::foldable<UnregisteredFix>)>);
static_assert(!bt::has_instance_v<decltype(bt::traversable<UnregisteredFix>)>);
} // namespace

TEMPLATE_TEST_CASE(
    "Payload reconstruction moves fresh values and never copies persistent sources", "", Binary, Fixed, Fringe, Rose) {
    const auto source_tree = source<TestType>();
    Tracked::copies        = 0;
    const auto tree        = bt::fmap([](int n) { return Tracked{n}; }, source_tree);
    auto       mapped      = bt::fmap([](const Tracked& n) { return Tracked{n.value * 10 + 1}; }, tree);
    auto traversed = bt::traverse([](const Tracked& n) { return std::optional{Tracked{n.value * 10 + 1}}; }, tree);
    REQUIRE(traversed.has_value());
    CHECK(Tracked::copies == 0);
    auto unwrap = [](const Tracked& n) { return n.value; };
    CHECK(bt::to_vector(bt::fmap(unwrap, mapped)) == std::vector<int>{11, 21, 31, 41});
    CHECK(bt::to_vector(bt::fmap(unwrap, *traversed)) == std::vector<int>{11, 21, 31, 41});
    CHECK(bt::to_vector(bt::fmap(unwrap, tree)) == std::vector<int>{1, 2, 3, 4});
}
