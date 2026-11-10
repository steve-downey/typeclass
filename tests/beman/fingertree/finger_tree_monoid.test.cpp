// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree/tree/finger_tree_monoid.hpp>
#include <beman/fingertree/tree/finger_tree_monoid.hpp> // Re-inclusion check
#include <beman/fingertree/tree/finger_tree_pmr.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/string.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <vector>

namespace {
namespace bt = beman::transpose;
namespace bf = beman::fingertree;
namespace ft = beman::fingertree::tree;

struct Join {
    using value_type      = std::string;
    std::string separator = ":";
    auto        identity() const -> std::string { return {}; }
    auto        combine(const std::string& a, const std::string& b) const -> std::string {
        if (a.empty())
            return b;
        if (b.empty())
            return a;
        return a + separator + b;
    }
};
struct TextMeasure {
    auto operator()(const std::string& value) const -> std::string { return value; }
};
} // namespace

TEST_CASE("Default tree concatenation has one canonical monoid") {
    using Tree = ft::FingerTree<std::string>;
    STATIC_REQUIRE(bt::monoid_object<bt::Monoid<Tree>, Tree>);
    STATIC_REQUIRE(!bt::detail::has_registered_monoid<ft::pmr::FingerTree<std::string>>);
    STATIC_REQUIRE(!bt::detail::has_registered_monoid<ft::FingerTree<std::string, Join, TextMeasure>>);
    const auto  a      = Tree::from_sequence({"a", "bc"});
    const auto  b      = Tree::from_sequence({"d"});
    const auto  c      = Tree::from_sequence({"e", "f"});
    const auto& monoid = bt::monoid<Tree>;
    CHECK(monoid.combine(monoid.identity(), a).flatten() == a.flatten());
    CHECK(monoid.combine(a, monoid.identity()).flatten() == a.flatten());
    const auto left  = monoid.combine(monoid.combine(a, b), c);
    const auto right = monoid.combine(a, monoid.combine(b, c));
    CHECK(left.flatten() == (std::vector<std::string>{"a", "bc", "d", "e", "f"}));
    CHECK(left.flatten() == right.flatten());
    CHECK(left.measure() == 5);
    CHECK(left.flatten() == Tree::concat(Tree::concat(a, b), c).flatten());
    CHECK(bt::combine_all(std::vector<Tree>{a, b, c}).flatten() == left.flatten());
}

TEST_CASE("Canonical concatenation shares move-only payloads") {
    using Tree = ft::FingerTree<std::unique_ptr<int>>;
    STATIC_REQUIRE(bt::monoid_object<bt::Monoid<Tree>, Tree>);
    const auto a      = Tree{}.snoc(std::make_unique<int>(1)).snoc(std::make_unique<int>(2));
    const auto b      = Tree{}.snoc(std::make_unique<int>(3));
    const auto result = bt::monoid<Tree>.combine(a, b);
    CHECK(result.measure() == 3);
    CHECK(*result.head_ref() == 1);
    CHECK(*result.last_ref() == 3);
    CHECK(&result.head_ref() == &a.head_ref());
    CHECK(bt::monoid<Tree>.combine(bt::monoid<Tree>.identity(), a).measure() == 2);
}

TEST_CASE("Configured concatenation obeys noncommutative laws in its selected domain") {
    using Tree   = ft::pmr::FingerTree<std::string, Join, TextMeasure>;
    using Monoid = bf::finger_tree_concat_monoid<Tree>;
    STATIC_REQUIRE(bt::monoid_object<Monoid, Tree>);
    std::pmr::monotonic_buffer_resource selected;
    std::pmr::monotonic_buffer_resource foreign;
    const Tree                          prototype{&selected, Join{"/"}};
    const Monoid                        monoid{prototype};
    const auto                          a        = Tree::from_sequence({"a", "bc"}, &foreign, Join{"/"});
    const auto                          b        = Tree::from_sequence({"d"}, &foreign, Join{"/"});
    const auto                          c        = Tree::from_sequence({"e", "f"}, &foreign, Join{"/"});
    const auto                          identity = monoid.identity();
    CHECK(identity.empty());
    CHECK(identity.get_allocator().resource() == &selected);
    CHECK(identity.get_monoid().separator == "/");
    for (const auto& result : {monoid.combine(identity, a), monoid.combine(a, identity)}) {
        CHECK(result.flatten() == a.flatten());
        CHECK(result.measure() == a.measure());
        CHECK(result.get_allocator().resource() == &selected);
        CHECK(result.get_monoid().separator == "/");
    }
    const auto left  = monoid.combine(monoid.combine(a, b), c);
    const auto right = monoid.combine(a, monoid.combine(b, c));
    CHECK(left.flatten() == right.flatten());
    CHECK(left.measure() == "a/bc/d/e/f");
    CHECK(left.measure() == right.measure());
    CHECK(left.measure() == prototype.get_monoid().combine(monoid.combine(a, b).measure(), c.measure()));
    CHECK(left.get_allocator().resource() == &selected);
    CHECK(left.get_monoid().separator == "/");
    const auto reversed = left.reversed();
    CHECK(reversed.flatten() == (std::vector<std::string>{"f", "e", "d", "bc", "a"}));
    CHECK(reversed.measure() == left.measure());
    const auto ab_reversed = monoid.combine(a, b).reversed();
    const auto dual_concat = decltype(ab_reversed)::concat(b.reversed(), a.reversed());
    CHECK(ab_reversed.flatten() == dual_concat.flatten());
    CHECK(ab_reversed.measure() == dual_concat.measure());
}

TEST_CASE("Configured concatenation owns rebuilt nodes after foreign sources die") {
    using Tree = ft::pmr::FingerTree<std::string, Join, TextMeasure>;
    std::pmr::monotonic_buffer_resource selected;
    const auto                          monoid = [&] {
        const Tree prototype{&selected, Join{"/"}};
        return bf::finger_tree_concat_monoid<Tree>{prototype};
    }();
    const auto result = [&] {
        std::pmr::monotonic_buffer_resource foreign;
        return monoid.combine(Tree::from_sequence({"a", "bc"}, &foreign, Join{"/"}),
                              Tree::from_sequence({"d", "e"}, &foreign, Join{"/"}));
    }();
    CHECK(result.flatten() == (std::vector<std::string>{"a", "bc", "d", "e"}));
    CHECK(result.measure() == "a/bc/d/e");
    CHECK(result.get_allocator().resource() == &selected);
}
