// tests/beman/transpose/functor.test.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/functor.hpp>
#include <beman/transpose/optional.hpp>
#include <beman/transpose/sequence.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace bt = beman::transpose;

TEST_CASE("functor: fmap over optional") {
    const auto &f = bt::functor<std::optional<int>>;
    REQUIRE(f.fmap([](int x) { return x + 1; }, std::optional<int>{41}) ==
            std::optional<int>{42});
    REQUIRE(f.fmap([](int x) { return x + 1; }, std::optional<int>{}) ==
            std::optional<int>{});
}

TEST_CASE("functor: fmap over vector") {
    const auto &f = bt::functor<std::vector<int>>;
    REQUIRE(f.fmap([](int x) { return x * 2; }, std::vector<int>{1, 2, 3}) ==
            std::vector<int>{2, 4, 6});
}

TEST_CASE("functor: replace is derived from fmap") {
    const auto &f = bt::functor<std::vector<int>>;
    REQUIRE(f.replace(std::vector<int>{1, 2, 3}, 0) ==
            std::vector<int>{0, 0, 0});
}

namespace {
// A Functor Impl with a native `replace` that is observably not the
// derivation: it replaces with the replacement *twice over*, so a test can
// tell which member ran.
struct MarkedReplaceImpl {
    template <class F>
    [[maybe_unused]] auto fmap(this auto &&, F &&function,
                               const std::vector<int> &values) {
        std::vector<int> output;
        output.reserve(values.size());
        for (const auto &v : values) {
            output.push_back(function(v));
        }
        return output;
    }

    template <class T, class U>
    auto replace(this auto &&, T &&value, U &&replacement) {
        return std::vector<int>(value.size(), replacement + replacement);
    }
};

struct MarkedReplaceMap : bt::derive_functor<MarkedReplaceImpl> {};
} // namespace

TEST_CASE("functor: replace prefers a native Impl::replace") {
    MarkedReplaceMap m{};
    REQUIRE(m.replace(std::vector<int>{1, 2, 3}, 5) ==
            std::vector<int>{10, 10, 10});
}

// The derived operations are constant-evaluable: each is pinned by a
// static_assert, over both standard instances.
TEST_CASE("functor: the derived operations are constexpr") {
    static_assert(bt::functor<std::optional<int>>.fmap(
                      [](int x) { return x + 1; }, std::optional<int>{41}) ==
                  std::optional<int>{42});
    static_assert(bt::functor<std::optional<int>>.replace(
                      std::optional<int>{41}, 7) == std::optional<int>{7});
    static_assert(
        bt::functor<std::vector<int>>.fmap([](int x) { return x * 10; },
                                           std::vector<int>{1, 2, 3}) ==
        std::vector<int>{10, 20, 30});
    static_assert(bt::functor<std::vector<int>>.replace(
                      std::vector<int>{1, 2}, 0) == std::vector<int>{0, 0});
    SUCCEED();
}

// steve-downey/typeclass#10: the optional and vector instances' fmap is
// constrained on the function taking the element, so a probe of it -- or of
// the operation object, or of the derived replace -- is a soft failure.
namespace {

struct NotCallable {};

template <class F, class C>
concept has_fmap_object = requires(F f, C c) { bt::fmap(f, c); };

template <class MAP, class F, class C>
concept has_fmap_member = requires(const MAP &m, F f, C c) { m.fmap(f, c); };

using PlusOne = decltype([](int x) { return x + 1; });
using TakesString = decltype([](const std::string &s) { return s.size(); });
using AddTwo = decltype([](int x, int y) { return x + y; });

} // namespace

TEST_CASE("functor: optional and vector fmap reject functions that cannot "
          "take the element") {
    using O = std::optional<int>;
    using V = std::vector<int>;
    using OMap = std::remove_cvref_t<decltype(bt::functor<O>)>;
    using VMap = std::remove_cvref_t<decltype(bt::functor<V>)>;

    STATIC_REQUIRE(has_fmap_object<PlusOne, O>);
    STATIC_REQUIRE(!has_fmap_object<NotCallable, O>);
    STATIC_REQUIRE(!has_fmap_object<TakesString, O>);
    STATIC_REQUIRE(!has_fmap_object<AddTwo, O>);
    STATIC_REQUIRE(has_fmap_member<OMap, PlusOne, O>);
    STATIC_REQUIRE(!has_fmap_member<OMap, NotCallable, O>);

    STATIC_REQUIRE(has_fmap_object<PlusOne, V>);
    STATIC_REQUIRE(!has_fmap_object<NotCallable, V>);
    STATIC_REQUIRE(!has_fmap_object<TakesString, V>);
    STATIC_REQUIRE(!has_fmap_object<AddTwo, V>);
    STATIC_REQUIRE(has_fmap_member<VMap, PlusOne, V>);
    STATIC_REQUIRE(!has_fmap_member<VMap, NotCallable, V>);

    REQUIRE(bt::fmap([](int x) { return x + 1; }, O{41}) == O{42});
    REQUIRE(bt::fmap([](int x) { return x + 1; }, V{1, 2}) == V{2, 3});
    REQUIRE(bt::functor<V>.replace(V{1, 2}, 7) == V{7, 7});
}

// steve-downey/typeclass#18: fmap rejects a callable whose result the
// context cannot hold, such as one returning void.
TEST_CASE("functor: optional and vector fmap reject a callable returning "
          "void") {
    using ReturnsVoid = decltype([](int) {});
    STATIC_REQUIRE(!has_fmap_object<ReturnsVoid, std::optional<int>>);
    STATIC_REQUIRE(!has_fmap_object<ReturnsVoid, std::vector<int>>);
    STATIC_REQUIRE(!has_fmap_object<decltype([](int) { return std::nullopt; }),
                                    std::optional<int>>);
}
