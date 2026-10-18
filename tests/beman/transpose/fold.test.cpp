// tests/beman/transpose/fold.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/fold.hpp>
#include <beman/transpose/sequence.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace bt = beman::transpose;

namespace {

template <class M, class F, class T>
concept has_fold_map = requires(M &m, F &f, T &t) { m.fold_map(f, t); };

template <class M, class F, class T, class MONOID>
concept has_fold_map_with =
    requires(M &m, F &f, T &t, MONOID mo) { m.fold_map(f, t, mo); };

template <class M, class T>
concept has_combine_all = requires(M &m, T &t) { m.combine_all(t); };

// An Impl with a native `length` returning a sentinel the fold_map
// derivation could never produce, registered through a Map with no `using`
// for it -- the ergonomics claim, exercised directly: an instance author
// adds a native member and gets it back with no Map edit.
struct MarkerLengthImpl {
    template <class T>
    auto length(this auto &&, const T &) -> std::size_t {
        return 999;
    }
};

struct MarkerLengthMap : bt::derive_foldable<MarkerLengthImpl> {};

// An Impl with a fold_map over std::vector<V>, and a native to_vector for
// std::vector<int> only -- a marker the fold_map derivation could never
// produce, and unreachable for any other element type. This is the
// per-instantiation property a Map-level `using` could never express. The
// Impl is a template over V so that it can declare element_type, which the
// derived to_vector needs to name the vector it collects into.
template <class V>
struct PerInstantiationToVectorImpl {
    using element_type = V;

    template <class FUNCTION, class MONOID>
    auto fold_map(this auto &&, FUNCTION &&function,
                  const std::vector<V> &values, MONOID monoid) {
        using Result =
            std::remove_cvref_t<std::invoke_result_t<FUNCTION &, const V &>>;
        Result accumulated = monoid.identity();
        for (const auto &value : values) {
            accumulated = monoid.combine(std::move(accumulated),
                                         std::invoke(function, value));
        }
        return accumulated;
    }

    auto to_vector(this auto &&, const std::vector<int> &) -> std::vector<int>
        requires std::same_as<V, int>
    {
        return std::vector<int>{-1};
    }
};

template <class V>
struct PerInstantiationToVectorMap
    : bt::derive_foldable<PerInstantiationToVectorImpl<V>> {};

// The hazard this step closes: an Impl providing only fold_right +
// element_type, no fold_map at all, registered through a Map with no
// using-declaration whatsoever -- the Map body below is exactly its
// definition and nothing else. Before this step, fold_map's derivation and
// fold_right's derivation were both self-routed, so a Map shaped like this
// sent the two into unbounded mutual template recursion (each round nesting
// another RightFoldProgram) rather than a clean answer. After this step,
// fold_map is available and correct because its derivation addresses Impl
// directly instead of re-entering through self.
struct FoldRightOnlyImpl {
    using element_type = int;

    // A plain const member, not an explicit-object one: MSVC's backend
    // ICEs (C1001, p2) emitting the deducing-this form of exactly this
    // body; see the twin in objects.test.cpp.
    template <class STATE, class FUNCTION>
    auto fold_right(const std::vector<int> &values, STATE initial_state,
                    FUNCTION &&function) const -> STATE {
        STATE state = std::move(initial_state);
        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            state = std::invoke(function, *it, std::move(state));
        }
        return state;
    }
};

struct FoldRightOnlyMap : bt::derive_foldable<FoldRightOnlyImpl> {};

} // namespace

TEST_CASE("fold: length prefers a native Impl::length, no Map using needed") {
    MarkerLengthMap m{};
    REQUIRE(m.length(std::vector<int>{1, 2, 3}) == 999);
}

TEST_CASE("fold: to_vector native preference is per instantiation") {
    // std::vector<int>: the native to_vector fires.
    REQUIRE(PerInstantiationToVectorMap<int>{}.to_vector(
                std::vector<int>{1, 2, 3}) == std::vector<int>{-1});

    // std::vector<std::string>: no native to_vector exists for this element
    // type, so the same member falls back to the fold_map derivation.
    REQUIRE(PerInstantiationToVectorMap<std::string>{}.to_vector(
                std::vector<std::string>{"a", "b"}) ==
            std::vector<std::string>{"a", "b"});
}

TEST_CASE("fold: fold_map derives from a fold_right + element_type Impl, "
          "with no Map using-declaration at all") {
    FoldRightOnlyMap m{};
    std::vector<int> xs{1, 2, 3, 4};

    // fold_map itself, via the fold_right basis -- this is the shape that
    // would have recursed unboundedly before this step.
    REQUIRE(m.fold_map([](int) { return std::size_t{1}; }, xs,
                       bt::sum_monoid<std::size_t>{}) == 4);

    // The rest of the family, all routed through the same fold_map.
    REQUIRE(m.length(xs) == 4);
    REQUIRE(m.to_vector(xs) == xs);
    REQUIRE(m.fold_right(xs, std::string{}, [](int x, std::string acc) {
        return acc + std::to_string(x);
    }) == "4321");
}

TEST_CASE("fold: fold_map does not exist when Impl provides neither basis") {
    struct NoBasisImpl {};
    struct NoBasisMap : bt::derive_foldable<NoBasisImpl> {};

    static_assert(!has_fold_map<NoBasisMap, decltype([](int x) { return x; }),
                                std::vector<int>>);
    static_assert(
        !has_fold_map_with<NoBasisMap, decltype([](int x) { return x; }),
                           std::vector<int>, bt::sum_monoid<int>>);
}

TEST_CASE("fold: a fold over a type with several monoids names its instance") {
    // docs/transpose/decisions.md#monoid-selection: int carries addition,
    // product, max, min, ... so no Monoid<int> is registered, and the
    // two-argument fold_map is not a candidate -- not a hard error from inside
    // the body. The caller passes the instance. std::string has one canonical
    // monoid, so the two-argument form is available and defaults to it.
    using Map = bt::VectorFoldableMap<int>;
    using Identity = decltype([](int x) { return x; });
    static_assert(!has_fold_map<Map, Identity, std::vector<int>>);
    static_assert(has_fold_map_with<Map, Identity, std::vector<int>,
                                    bt::sum_monoid<int>>);
    static_assert(!has_combine_all<Map, std::vector<int>>);
    static_assert(has_combine_all<bt::VectorFoldableMap<std::string>,
                                  std::vector<std::string>>);

    Map m{};
    std::vector<int> xs{3, 1, 4, 1, 5};
    REQUIRE(m.fold_map([](int x) { return x; }, xs, bt::sum_monoid<int>{}) ==
            14);
    REQUIRE(m.fold_map([](int x) { return x; }, xs, bt::max_monoid<int>{}) ==
            5);
    REQUIRE(m.combine_all(xs, bt::product_monoid<int>{}) == 60);
    REQUIRE(m.fold(xs, bt::min_monoid<int>{}) == 1);

    // Instances compose, and the value type stays bare: a pair of ints
    // under sum and max at once, no wrapper on either component.
    REQUIRE(
        (m.fold_map(
             [](int x) { return std::tuple{x, x}; }, xs,
             bt::tuple_monoid<bt::sum_monoid<int>, bt::max_monoid<int>>{}) ==
         std::tuple{14, 5}));

    bt::VectorFoldableMap<std::string> ms{};
    std::vector<std::string> ss{"a", "b", "c"};
    REQUIRE(ms.combine_all(ss) == "abc");
    REQUIRE(ms.fold_map([](const std::string &s) { return s; }, ss) == "abc");
    REQUIRE(ms.combine_all(ss, bt::dual_monoid<bt::Monoid<std::string>>{}) ==
            "cba");
}

TEST_CASE("fold: length and to_vector over Sequence") {
    const auto &f = bt::foldable<bt::test::Sequence<int>>;
    bt::test::Sequence<int> seq{{10, 20, 30}};
    REQUIRE(f.length(seq) == 3);
    REQUIRE(f.to_vector(seq) == std::vector<int>{10, 20, 30});
}

TEST_CASE("fold: fold_left and fold_right accumulate") {
    const auto &f = bt::foldable<bt::test::Sequence<int>>;
    bt::test::Sequence<int> seq{{1, 2, 3, 4}};
    REQUIRE(f.fold_left(seq, 0, [](int acc, int x) { return acc + x; }) == 10);
    REQUIRE(f.fold_right(seq, std::string{}, [](int x, std::string acc) {
        return acc + std::to_string(x);
    }) == "4321");
}

TEST_CASE("fold: any_of, all_of, find_first") {
    const auto &f = bt::foldable<bt::test::Sequence<int>>;
    bt::test::Sequence<int> seq{{2, 4, 6, 7}};
    REQUIRE(f.any_of(seq, [](int x) { return x % 2 == 1; }));
    REQUIRE_FALSE(f.all_of(seq, [](int x) { return x % 2 == 0; }));
    REQUIRE(f.find_first(seq, [](int x) { return x > 5; }) == 6);
}

// The derived operations are constant-evaluable: each is pinned by a
// static_assert. The folds are order-sensitive (digits accumulate left to
// right), so a fold in the wrong direction gives a different number.
TEST_CASE("fold: the derived operations are constexpr") {
    constexpr const auto &m = bt::foldable<std::vector<int>>;
    static_assert(m.fold_map([](int x) { return x; }, std::vector<int>{1, 2, 3},
                             bt::sum_monoid<int>{}) == 6);
    static_assert(m.fold_map([](int x) { return std::vector<int>{x}; },
                             std::vector<int>{1, 2, 3}) ==
                  std::vector<int>{1, 2, 3});
    static_assert(m.length(std::vector<int>{1, 2, 3}) == 3);
    // fold_left and fold_right are not pinned here: derived through fold_map,
    // they compose std::function endomorphisms, which cannot be constant
    // evaluated. An instance with a native fold_left or fold_right reaches it
    // through a constexpr forwarder; see the next test case.
    static_assert(m.combine_all(std::vector<int>{2, 3, 4},
                                bt::product_monoid<int>{}) == 24);
    constexpr const auto &mm = bt::foldable<std::vector<std::vector<int>>>;
    static_assert(mm.combine_all(std::vector<std::vector<int>>{{1}, {2, 3}}) ==
                  std::vector<int>{1, 2, 3});
    static_assert(m.fold(std::vector<int>{2, 3, 4}, bt::sum_monoid<int>{}) ==
                  9);
    static_assert(mm.fold(std::vector<std::vector<int>>{{1}, {2, 3}}) ==
                  std::vector<int>{1, 2, 3});
    static_assert(
        m.any_of(std::vector<int>{1, 2, 3}, [](int x) { return x == 2; }));
    static_assert(
        !m.all_of(std::vector<int>{1, 2, 3}, [](int x) { return x < 3; }));
    static_assert(m.empty(std::vector<int>{}));
    static_assert(m.to_vector(std::vector<int>{1, 2, 3}) ==
                  std::vector<int>{1, 2, 3});
    static_assert(m.find_first(std::vector<int>{1, 2, 3, 4}, [](int x) {
        return x % 2 == 0;
    }) == std::optional<int>{2});
    SUCCEED();
}

namespace {
// A Foldable Impl with native fold_left and fold_right, so the derived
// operations reach them through derive_foldable's forwarders rather than
// through the std::function derivation.
struct NativeFoldsImpl {
    using element_type = int;
    template <class STATE, class F>
    constexpr auto fold_left(const std::vector<int> &values, STATE state,
                             F &&function) const -> STATE {
        for (int value : values) {
            state = function(std::move(state), value);
        }
        return state;
    }
    template <class STATE, class F>
    constexpr auto fold_right(const std::vector<int> &values, STATE state,
                              F &&function) const -> STATE {
        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            state = function(*it, std::move(state));
        }
        return state;
    }
};
struct NativeFoldsMap : bt::derive_foldable<NativeFoldsImpl> {};
} // namespace

TEST_CASE("fold: fold_left and fold_right forward to a native Impl in a "
          "constant expression") {
    constexpr NativeFoldsMap m{};
    static_assert(m.fold_left(std::vector<int>{1, 2, 3}, 0, [](int acc, int x) {
        return acc * 10 + x;
    }) == 123);
    static_assert(m.fold_right(std::vector<int>{1, 2, 3}, 0,
                               [](int x, int acc) { return acc * 10 + x; }) ==
                  321);
    SUCCEED();
}

// steve-downey/typeclass#12: each derived member that applies the caller's
// callable is constrained on that callable, so a probe of it answers false,
// softly, for a callable the derivation could not have applied.
namespace {

struct NotCallable {};
struct NotBool {};

template <class MAP, class STATE, class F>
concept has_fold_left = requires(const MAP &m, STATE s, F f) {
    m.fold_left(std::vector<int>{1}, s, f);
};

template <class MAP, class STATE, class F>
concept has_fold_right = requires(const MAP &m, STATE s, F f) {
    m.fold_right(std::vector<int>{1}, s, f);
};

template <class MAP, class P>
concept has_any_of =
    requires(const MAP &m, P p) { m.any_of(std::vector<int>{1}, p); };

template <class MAP, class P>
concept has_all_of =
    requires(const MAP &m, P p) { m.all_of(std::vector<int>{1}, p); };

template <class MAP, class P>
concept has_find_first =
    requires(const MAP &m, P p) { m.find_first(std::vector<int>{1}, p); };

template <class MAP, class F>
concept has_fold_map_sum = requires(const MAP &m, F f) {
    m.fold_map(f, std::vector<int>{1}, bt::sum_monoid<int>{});
};

template <class STATE, class F>
concept has_fold_left_object =
    requires(STATE s, F f) { bt::fold_left(std::vector<int>{1}, s, f); };

template <class STATE, class F>
concept has_fold_right_object =
    requires(STATE s, F f) { bt::fold_right(std::vector<int>{1}, s, f); };

template <class P>
concept has_any_of_object =
    requires(P p) { bt::any_of(std::vector<int>{1}, p); };

template <class P>
concept has_all_of_object =
    requires(P p) { bt::all_of(std::vector<int>{1}, p); };

template <class P>
concept has_find_first_object =
    requires(P p) { bt::find_first(std::vector<int>{1}, p); };

auto left_step = [](int acc, int x) { return acc + x; };
using RightStep = decltype([](int x, int acc) { return acc + x; });
using ToNotBool = decltype([](int) { return NotBool{}; });
auto is_even = [](int x) { return x % 2 == 0; };
// A step whose result cannot become the next state.
using StatelessStep = decltype([](int, int) { return NotBool{}; });
// A step that takes its operands in fold_right's order, which fold_left
// cannot call; and the reverse.
using StringLeftStep = decltype([](std::string acc, int) { return acc; });
using StringRightStep = decltype([](int, std::string acc) { return acc; });

} // namespace

TEST_CASE("fold: derived members over a fold_map basis reject callables the "
          "derivation cannot apply") {
    using Map = std::remove_cvref_t<decltype(bt::foldable<std::vector<int>>)>;

    STATIC_REQUIRE(has_fold_left<Map, int, decltype(left_step)>);
    STATIC_REQUIRE(!has_fold_left<Map, int, NotCallable>);
    STATIC_REQUIRE(!has_fold_left<Map, int, StatelessStep>);
    STATIC_REQUIRE(has_fold_left<Map, std::string, StringLeftStep>);
    STATIC_REQUIRE(!has_fold_left<Map, std::string, StringRightStep>);

    STATIC_REQUIRE(has_fold_right<Map, int, RightStep>);
    STATIC_REQUIRE(!has_fold_right<Map, int, NotCallable>);
    STATIC_REQUIRE(!has_fold_right<Map, int, StatelessStep>);
    STATIC_REQUIRE(has_fold_right<Map, std::string, StringRightStep>);
    STATIC_REQUIRE(!has_fold_right<Map, std::string, StringLeftStep>);

    STATIC_REQUIRE(has_any_of<Map, decltype(is_even)>);
    STATIC_REQUIRE(!has_any_of<Map, NotCallable>);
    STATIC_REQUIRE(!has_any_of<Map, ToNotBool>);

    STATIC_REQUIRE(has_all_of<Map, decltype(is_even)>);
    STATIC_REQUIRE(!has_all_of<Map, NotCallable>);
    STATIC_REQUIRE(!has_all_of<Map, ToNotBool>);

    STATIC_REQUIRE(has_find_first<Map, decltype(is_even)>);
    STATIC_REQUIRE(!has_find_first<Map, NotCallable>);
    STATIC_REQUIRE(!has_find_first<Map, ToNotBool>);

    // empty derives through any_of with a predicate of its own, so it is
    // unaffected.
    const auto &m = bt::foldable<std::vector<int>>;
    REQUIRE(m.fold_left(std::vector<int>{1, 2, 3}, 0, left_step) == 6);
    REQUIRE(m.any_of(std::vector<int>{1, 2, 3}, is_even));
    REQUIRE(!m.all_of(std::vector<int>{1, 2, 3}, is_even));
    REQUIRE(m.find_first(std::vector<int>{1, 2, 3}, is_even) ==
            std::optional<int>{2});
    REQUIRE(!m.empty(std::vector<int>{1}));
}

TEST_CASE("fold: the operation objects inherit the derived members' answers") {
    STATIC_REQUIRE(has_fold_left_object<int, decltype(left_step)>);
    STATIC_REQUIRE(!has_fold_left_object<int, NotCallable>);
    STATIC_REQUIRE(has_fold_right_object<int, RightStep>);
    STATIC_REQUIRE(!has_fold_right_object<int, NotCallable>);
    STATIC_REQUIRE(has_any_of_object<decltype(is_even)>);
    STATIC_REQUIRE(!has_any_of_object<NotCallable>);
    STATIC_REQUIRE(!has_any_of_object<ToNotBool>);
    STATIC_REQUIRE(has_all_of_object<decltype(is_even)>);
    STATIC_REQUIRE(!has_all_of_object<NotCallable>);
    STATIC_REQUIRE(has_find_first_object<decltype(is_even)>);
    STATIC_REQUIRE(!has_find_first_object<NotCallable>);
}

TEST_CASE("fold: derived members over a fold_right basis reject callables "
          "the derivation cannot apply") {
    // fold_map derives from fold_right here, and every other member derives
    // through that fold_map, so the callable has to be checked in fold_map's
    // own fold_right alternative.
    STATIC_REQUIRE(
        has_fold_map_sum<FoldRightOnlyMap, decltype([](int x) { return x; })>);
    STATIC_REQUIRE(!has_fold_map_sum<FoldRightOnlyMap, NotCallable>);
    STATIC_REQUIRE(!has_fold_map_sum<FoldRightOnlyMap, ToNotBool>);

    STATIC_REQUIRE(has_fold_left<FoldRightOnlyMap, int, decltype(left_step)>);
    STATIC_REQUIRE(!has_fold_left<FoldRightOnlyMap, int, NotCallable>);
    STATIC_REQUIRE(has_any_of<FoldRightOnlyMap, decltype(is_even)>);
    STATIC_REQUIRE(!has_any_of<FoldRightOnlyMap, NotCallable>);
    STATIC_REQUIRE(!has_all_of<FoldRightOnlyMap, ToNotBool>);
    STATIC_REQUIRE(has_find_first<FoldRightOnlyMap, decltype(is_even)>);
    STATIC_REQUIRE(!has_find_first<FoldRightOnlyMap, NotCallable>);

    FoldRightOnlyMap m{};
    REQUIRE(m.fold_left(std::vector<int>{1, 2, 3}, 0, left_step) == 6);
    REQUIRE(m.find_first(std::vector<int>{1, 2, 3}, is_even) ==
            std::optional<int>{2});
}
