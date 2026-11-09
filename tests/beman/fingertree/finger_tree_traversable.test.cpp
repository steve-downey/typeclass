// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/fingertree/tree/finger_tree_traversable.hpp>
#include <beman/fingertree/tree/finger_tree_traversable.hpp> // Re-inclusion check

#include <beman/fingertree/tree/finger_tree_interval_index.hpp>
#include <beman/fingertree/tree/finger_tree_priority_queue.hpp>
#include <beman/fingertree/tree/finger_tree_random_access.hpp>
#include <beman/fingertree/tree/finger_tree_rope.hpp>
#include <beman/transpose/traverse.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../collection_test_support.hpp"

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

TEST_CASE("FingerTreeTraversableTest - TraverseOptionalSucceeds") {
    using Tree  = beman::fingertree::tree::FingerTree<int>;
    auto t      = Tree::from_sequence({1, 2, 3});
    auto result = beman::transpose::traverse([](int x) { return std::optional{x * 10}; }, t);
    REQUIRE(result.has_value());
    CHECK(result->flatten() == (std::vector<int>{10, 20, 30}));
}

TEST_CASE("FingerTreeTraversableTest - TraverseOptionalFailsOnNullopt") {
    using Tree  = beman::fingertree::tree::FingerTree<int>;
    auto t      = Tree::from_sequence({1, 2, 3});
    auto result = beman::transpose::traverse(
        [](int x) -> std::optional<int> { return x == 2 ? std::nullopt : std::optional{x}; }, t);
    CHECK_FALSE(result.has_value());
}

TEST_CASE("FingerTreeTraversableTest - TraversePreservesShape") {
    using Tree  = beman::fingertree::tree::FingerTree<int>;
    auto t      = Tree::from_sequence({10, 20, 30, 40});
    auto result = beman::transpose::traverse([](int x) { return std::optional{x + 1}; }, t);
    REQUIRE(result.has_value());
    CHECK(result->flatten() == (std::vector<int>{11, 21, 31, 41}));
}

TEST_CASE("FingerTreeTraversableTest - TraverseOverLargeTree") {
    // Cover the spine-Node3 path: tree large enough that the spine has
    // structure when traverse materializes it.
    using Tree = beman::fingertree::tree::FingerTree<int>;
    auto t     = Tree{};
    for (int i = 0; i < 100; ++i)
        t = t.snoc(i);

    auto result = beman::transpose::traverse([](int x) { return std::optional{x}; }, t);
    REQUIRE(result.has_value());
    auto v = result->flatten();
    REQUIRE(v.size() == 100U);
    for (int i = 0; i < 100; ++i)
        CHECK(v[static_cast<std::size_t>(i)] == i);
}

// steve-downey/typeclass#8: each finger-tree Traversable instance's traverse
// is constrained, so a probe with a callable it cannot apply is a constraint
// that does not match, not a diagnostic from inside the deduced-return body.
namespace {

struct NotCallable {};

template <class STRUCTURE, class F>
concept has_traverse = requires(F f) {
    beman::transpose::traversable<STRUCTURE>.traverse(
        beman::transpose::applicative<std::optional<
            typename std::remove_cvref_t<decltype(beman::transpose::traversable<STRUCTURE>)>::element_type>>,
        f,
        std::declval<const STRUCTURE&>());
};

template <class STRUCTURE, class F>
concept has_for_each =
    requires(F f) { beman::transpose::traversable<STRUCTURE>.for_each(std::declval<const STRUCTURE&>(), f); };

struct Wrap {
    template <class T>
    auto operator()(const T& x) const -> std::optional<T> {
        return std::optional<T>{x};
    }
};

struct NotAContext {
    template <class T>
    auto operator()(const T&) const -> int {
        return 0;
    }
};

} // namespace

TEMPLATE_TEST_CASE("FingerTreeTraversableTest - InvalidTraverseIsASoftFailure",
                   "",
                   beman::fingertree::tree::FingerTree<int>,
                   beman::fingertree::tree::FingerTreeIntervalIndex<std::string>,
                   beman::fingertree::tree::FingerTreePriorityQueue<int>,
                   beman::fingertree::tree::FingerTreeRandomAccess<int>) {
    STATIC_REQUIRE(has_traverse<TestType, Wrap>);
    STATIC_REQUIRE(!has_traverse<TestType, NotCallable>);
    STATIC_REQUIRE(!has_traverse<TestType, NotAContext>);
    STATIC_REQUIRE(has_for_each<TestType, Wrap>);
    STATIC_REQUIRE(!has_for_each<TestType, NotCallable>);
}

namespace {
namespace ft = beman::fingertree::tree;
namespace ct = collection_test;

template <class POLICY, class TREE, class VALUES>
void check_collection(const TREE& tree, const VALUES& expected, auto read) {
    POLICY::collects = 0;
    auto mapper      = []<class T>(T&& value) {
        static_assert(std::is_const_v<std::remove_reference_t<T>>);
        static_assert(std::is_lvalue_reference_v<T>);
        using U = std::remove_cvref_t<T>;
        std::string log;
        if constexpr (std::same_as<U, std::string>)
            log = value;
        else
            log = std::to_string(value);
        return ct::Writer<U, std::same_as<POLICY, ct::WriterMap<true, true>>>{value, std::move(log)};
    };
    // A persistent rvalue still presents const lvalue elements.
    auto result = beman::transpose::traversable<TREE>.traverse(POLICY{}, mapper, TREE{tree});
    CHECK(read(result.held) == expected);
    CHECK(result.log == (expected.empty() ? "" : "123"));
    CHECK(POLICY::collects == (std::same_as<POLICY, ct::WriterMap<false>> ? 0 : 1));
}

template <class POLICY>
void check_wrappers() {
    for (bool empty : {false, true}) {
        const auto values = empty ? std::vector<int>{} : std::vector<int>{1, 2, 3};
        check_collection<POLICY>(
            ft::FingerTree<int>::from_sequence(values), values, [](const auto& t) { return t.flatten(); });
        check_collection<POLICY>(ft::FingerTreeRandomAccess<int>::from_sequence(values), values, [](const auto& t) {
            return t.to_vector();
        });
        check_collection<POLICY>(ft::FingerTreePriorityQueue<int>::from_values(values), values, [](const auto& t) {
            return t.to_vector();
        });
        const auto chunks  = ft::FingerTreeRope<>::from_chunks(empty ? std::vector<std::string>{}
                                                                     : std::vector<std::string>{"1", "2", "3"})
                                 .chunks();
        POLICY::collects   = 0;
        auto chunk_effects = beman::transpose::traversable<std::vector<std::string>>.traverse(
            POLICY{},
            [](const std::string& chunk) {
                return ct::Writer<std::string, std::same_as<POLICY, ct::WriterMap<true, true>>>{chunk, chunk};
            },
            chunks);
        auto rope_effect = POLICY{}.invoke(
            [](std::vector<std::string> values) { return ft::FingerTreeRope<>::from_chunks(std::move(values)); },
            std::move(chunk_effects));
        CHECK(rope_effect.held.chunks() == chunks);
        CHECK(rope_effect.log == (empty ? "" : "123"));
        CHECK(POLICY::collects == (std::same_as<POLICY, ct::WriterMap<false>> ? 0 : 1));
        const auto entries = empty ? std::vector<ft::Interval<int>>{}
                                   : std::vector<ft::Interval<int>>{{7, 20, 1}, {9, 30, 2}, {12, 40, 3}};
        check_collection<POLICY>(
            ft::FingerTreeIntervalIndex<int>::from_intervals(entries), values, [&](const auto& t) {
                const auto actual = t.entries();
                REQUIRE(actual.size() == entries.size());
                std::vector<int> payloads;
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    CHECK(actual[i].d_start == entries[i].d_start);
                    CHECK(actual[i].d_end == entries[i].d_end);
                    payloads.push_back(actual[i].d_payload);
                }
                return payloads;
            });
    }
}
} // namespace

TEST_CASE("FingerTreeTraversableTest - NativeCollectionPreservesOrderAndEndpoints") {
    check_wrappers<ct::WriterMap<true>>();
}

TEST_CASE("FingerTreeTraversableTest - PairwiseCompositionPreservesOrderAndEndpoints") {
    check_wrappers<ct::WriterMap<false>>();
}

TEST_CASE("FingerTreeTraversableTest - CollectionAllowsChangingCarrier") {
    STATIC_REQUIRE(beman::transpose::applicative_object_for<ct::WriterMap<true, true>, ct::Writer<int, true>>);
    check_wrappers<ct::WriterMap<true, true>>();
}

TEMPLATE_TEST_CASE("FingerTreeTraversableTest - DeferredReconstructionOwnsEndpoints",
                   "",
                   ct::DeferredMap<false>,
                   ct::DeferredMap<true>) {
    std::string log;
    auto        result = [&] {
        const auto index  = ft::FingerTreeIntervalIndex<int>::from_intervals({{7, 20, 1}, {9, 30, 2}, {12, 40, 3}});
        auto       mapper = [&](const int& value) {
            return ct::Deferred<int>{[value, &log] {
                log += std::to_string(value);
                return value * 10;
            }};
        };
        return beman::transpose::traversable<std::remove_cvref_t<decltype(index)>>.traverse(TestType{}, mapper, index);
    }();
    CHECK(log.empty());
    const auto entries = result.run().entries();
    CHECK(log == "123");
    REQUIRE(entries.size() == 3);
    CHECK(entries[0].d_start == 7);
    CHECK(entries[0].d_end == 20);
    CHECK(entries[0].d_payload == 10);
    CHECK(entries[1].d_start == 9);
    CHECK(entries[1].d_end == 30);
    CHECK(entries[1].d_payload == 20);
    CHECK(entries[2].d_start == 12);
    CHECK(entries[2].d_end == 40);
    CHECK(entries[2].d_payload == 30);
}

TEST_CASE("Rope chunk traversal supports arbitrary results and explicit rebuilding") {
    const auto rope   = ft::FingerTreeRope<>::from_chunks({"a", "bc", "def"});
    const auto chunks = rope.chunks();
    const auto lengths =
        beman::transpose::traverse([](const std::string& chunk) { return std::optional{chunk.size()}; }, chunks);
    REQUIRE(lengths.has_value());
    CHECK(*lengths == (std::vector<std::size_t>{1, 2, 3}));
    const auto unit_positions =
        beman::transpose::traverse([](const std::string&) { return std::optional{std::monostate{}}; }, chunks);
    REQUIRE(unit_positions.has_value());
    CHECK(unit_positions->size() == chunks.size());
    const auto units = beman::transpose::traversable<std::vector<std::string>>.for_each(
        chunks, [](const std::string&) { return std::optional{42}; });
    CHECK(units.has_value());
    const auto strings =
        beman::transpose::traverse([](const std::string& chunk) { return std::optional{chunk + "!"}; }, chunks);
    const auto rebuilt = beman::transpose::applicative<std::optional<std::string>>.invoke(
        [](std::vector<std::string> values) { return ft::FingerTreeRope<>::from_chunks(std::move(values)); }, strings);
    REQUIRE(rebuilt.has_value());
    CHECK(rebuilt->to_string() == "a!bc!def!");
}

TEMPLATE_TEST_CASE("Deferred Rope chunk traversal owns its reconstruction",
                   "",
                   ct::DeferredMap<false>,
                   ct::DeferredMap<true>) {
    std::string log;
    auto        result = [&] {
        const auto rope    = ft::FingerTreeRope<>::from_chunks({"1", "2", "3"});
        const auto chunks  = rope.chunks();
        auto       effects = beman::transpose::traversable<std::vector<std::string>>.traverse(
            TestType{},
            [&](const std::string& chunk) {
                return ct::Deferred<std::string>{[chunk, &log] {
                    log += chunk;
                    return chunk + "!";
                }};
            },
            chunks);
        return TestType{}.invoke(
            [](std::vector<std::string> values) { return ft::FingerTreeRope<>::from_chunks(std::move(values)); },
            std::move(effects));
    }();
    CHECK(log.empty());
    CHECK(result.run().to_string() == "1!2!3!");
    CHECK(log == "123");
}
