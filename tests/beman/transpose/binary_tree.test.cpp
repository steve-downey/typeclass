// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "../../../examples/transpose/binary_tree.hpp"
#include "../collection_test_support.hpp"

#include <beman/transpose/optional.hpp>
#include <beman/transpose/string.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace {
namespace bt = beman::transpose;
namespace ct = collection_test;
using Tree = example::BinaryTree<int>;

auto trees() -> std::vector<Tree> {
    return {Tree::leaf(1), Tree::node(2, Tree::leaf(1), Tree::leaf(3)),
            Tree::from_children_ptrs(1, {}, Tree::make_ptr(Tree::leaf(2))),
            Tree::from_children_ptrs(2, Tree::make_ptr(Tree::leaf(1)), {}),
            Tree::from_children_ptrs(3,
                                     Tree::make_ptr(Tree::from_children_ptrs(
                                         1, {}, Tree::make_ptr(Tree::leaf(2)))),
                                     {})};
}

void check_shape(const Tree &source, const Tree &result) {
    CHECK(result.value() == source.value() * 10);
    REQUIRE(result.has_left() == source.has_left());
    REQUIRE(result.has_right() == source.has_right());
    if (source.has_left())
        check_shape(source.left(), result.left());
    if (source.has_right())
        check_shape(source.right(), result.right());
}
} // namespace

TEMPLATE_TEST_CASE(
    "BinaryTree traversal composes every effect once in fold order", "",
    ct::WriterMap<false>, ct::WriterMap<true>, (ct::WriterMap<true, true>)) {
    for (const auto &tree : trees()) {
        TestType::collects = 0;
        std::string calls;
        auto mapper = [&]<class T>(T &&value) {
            static_assert(std::same_as<T, const int &>);
            calls += std::to_string(value);
            return ct::Writer<
                int, std::same_as<TestType, ct::WriterMap<true, true>>>{
                value * 10, std::to_string(value)};
        };
        const auto folded = bt::foldable<Tree>.fold_map(
            [](int value) { return std::to_string(value); }, tree,
            bt::monoid<std::string>);
        auto result =
            bt::traversable<Tree>.traverse(TestType{}, mapper, Tree{tree});
        CHECK(result.log == folded);
        CHECK(calls == folded);
        CHECK(TestType::collects ==
              (std::same_as<TestType, ct::WriterMap<false>> ? 0 : 1));
        check_shape(tree, result.held);
    }
}

TEMPLATE_TEST_CASE("BinaryTree deferred traversal owns sparse topology", "",
                   ct::DeferredMap<false>, ct::DeferredMap<true>) {
    const auto cases = trees();
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto &expected = cases[i];
        std::string log;
        auto result = [&] {
            // Only the topology descriptor and value effects should outlive
            // this source.
            auto source = trees()[i];
            auto mapper = [&](const int &value) {
                return ct::Deferred<int>{[value, &log] {
                    log += std::to_string(value);
                    return value * 10;
                }};
            };
            return bt::traversable<Tree>.traverse(TestType{}, mapper, source);
        }();
        CHECK(log.empty());
        const auto rebuilt = result.run();
        const auto folded = bt::foldable<Tree>.fold_map(
            [](int value) { return std::to_string(value); }, expected,
            bt::monoid<std::string>);
        CHECK(log == folded);
        check_shape(expected, rebuilt);
    }
}

TEST_CASE("BinaryTree optional traversal preserves failure and type changing "
          "results") {
    const auto tree = Tree::node(2, Tree::leaf(1), Tree::leaf(3));
    const auto failed = bt::traverse(
        [](const int &value) -> std::optional<int> {
            return value == 2 ? std::nullopt : std::optional{value};
        },
        tree);
    CHECK_FALSE(failed.has_value());
    const auto result = bt::traverse(
        [](const int &value) { return std::optional{std::to_string(value)}; },
        tree);
    REQUIRE(result.has_value());
    CHECK(result->value() == "2");
    CHECK(result->left().value() == "1");
    CHECK(result->right().value() == "3");
}
