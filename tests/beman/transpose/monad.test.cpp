// tests/beman/transpose/monad.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/expected.hpp>
#include <beman/transpose/monad.hpp>

#include <beman/transpose/functor.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace bt = beman::transpose;

namespace {
template <class M, class F, class MA>
concept has_fmap = requires(M &m, F &f, MA &ma) { m.fmap(f, ma); };
} // namespace

TEST_CASE("monad: optional bind short-circuits on empty") {
    auto half_if_even = [](int x) {
        return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
    };
    REQUIRE(bt::mbind(std::optional<int>{8}, half_if_even) ==
            std::optional<int>{4});
    REQUIRE(bt::mbind(std::optional<int>{7}, half_if_even) ==
            std::optional<int>{});
    REQUIRE(bt::mbind(std::optional<int>{}, half_if_even) ==
            std::optional<int>{});
}

TEST_CASE("monad: join flattens nested optional") {
    std::optional<std::optional<int>> nested{std::optional<int>{5}};
    REQUIRE(bt::join(nested) == std::optional<int>{5});
}

TEST_CASE("monad: invoke synthesized from bind and pure") {
    const auto &m = bt::monad<std::optional<int>>;
    auto add3 = [](int a, int b, int c) { return a + b + c; };

    REQUIRE(m.invoke(add3, std::optional<int>{1}, std::optional<int>{2},
                     std::optional<int>{3}) == std::optional<int>{6});
    REQUIRE(m.invoke(add3, std::optional<int>{1}, std::optional<int>{},
                     std::optional<int>{3}) == std::optional<int>{});
}

TEST_CASE("monad: invoke coheres with the applicative invoke") {
    const auto &monad = bt::monad<std::optional<int>>;
    const auto &applicative = bt::applicative<std::optional<int>>;
    auto add = [](int a, int b) { return a + b; };

    REQUIRE(
        monad.invoke(add, std::optional<int>{4}, std::optional<int>{5}) ==
        applicative.invoke(add, std::optional<int>{4}, std::optional<int>{5}));
    REQUIRE(
        monad.invoke(add, std::optional<int>{}, std::optional<int>{5}) ==
        applicative.invoke(add, std::optional<int>{}, std::optional<int>{5}));
}

TEST_CASE("monad: fmap is the Functor basis grounded in bind + pure") {
    const auto &m = bt::monad<std::optional<int>>;
    auto increment = [](int x) { return x + 1; };

    REQUIRE(m.fmap(increment, std::optional<int>{41}) ==
            std::optional<int>{42});
    REQUIRE(m.fmap(increment, std::optional<int>{}) == std::optional<int>{});
}

TEST_CASE("monad: fmap deduces a changed value type") {
    const auto &m = bt::monad<std::optional<int>>;
    auto to_string = [](int x) { return std::to_string(x); };

    auto result = m.fmap(to_string, std::optional<int>{7});
    static_assert(std::same_as<decltype(result), std::optional<std::string>>);
    REQUIRE(result == std::optional<std::string>{"7"});
}

TEST_CASE("monad: fmap is not callable with a mismatched function") {
    struct not_invocable_on_int {};
    using not_applicable_t = decltype([](not_invocable_on_int) { return 0; });

    static_assert(!has_fmap<decltype(bt::monad<std::optional<int>>),
                            not_applicable_t, std::optional<int>>);
}

namespace {
// An Impl with pure + bind, and a native `join` constrained to
// std::optional<std::optional<int>> only -- a marker the bind derivation
// could never produce, and unreachable for any other element type. This is
// the per-instantiation property `using` could never express: one Impl,
// native for one instantiation, derived for another.
struct PerInstantiationJoinImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class A, class F>
    auto bind(this auto &&, const std::optional<A> &ma, F &&f)
        -> std::remove_cvref_t<std::invoke_result_t<F, const A &>> {
        using Result = std::remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma)
            return Result{};
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }

    auto join(this auto &&, const std::optional<std::optional<int>> &)
        -> std::optional<int> {
        return std::optional<int>{-1};
    }
};

struct PerInstantiationJoinMap : bt::derive_monad<PerInstantiationJoinImpl> {};

// An Impl with pure + bind, and a native `ap` that ignores both arguments
// and returns a sentinel the bind + pure derivation could never produce.
struct MarkedApImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class A, class F>
    [[maybe_unused]] auto bind(this auto &&, const std::optional<A> &ma, F &&f)
        -> std::remove_cvref_t<std::invoke_result_t<F, const A &>> {
        using Result = std::remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma)
            return Result{};
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }

    template <class MF, class MA>
    auto ap(this auto &&, MF &&, MA &&) -> std::optional<int> {
        return std::optional<int>{-1};
    }
};

struct MarkedApMap : bt::derive_monad<MarkedApImpl> {};
} // namespace

TEST_CASE("monad: join native preference is per instantiation") {
    PerInstantiationJoinMap m{};

    // std::optional<std::optional<int>>: the native join fires.
    REQUIRE(m.join(std::optional<std::optional<int>>{std::optional<int>{5}}) ==
            std::optional<int>{-1});

    // std::optional<std::optional<std::string>>: no native join exists for
    // this element type, so the same member falls back to the bind
    // derivation -- a property a Map-level `using` could never express,
    // since `using` selects a name for every instantiation at once.
    REQUIRE(m.join(std::optional<std::optional<std::string>>{
                std::optional<std::string>{"x"}}) ==
            std::optional<std::string>{"x"});
}

TEST_CASE("monad: ap prefers a native Impl::ap") {
    MarkedApMap m{};
    auto increment = [](int x) { return x + 1; };

    REQUIRE(m.ap(std::optional<decltype(increment)>{increment},
                 std::optional<int>{5}) == std::optional<int>{-1});
}

TEST_CASE("monad: ap falls back to the bind + pure derivation") {
    const auto &m = bt::monad<std::optional<int>>;
    auto increment = [](int x) { return x + 1; };

    REQUIRE(m.ap(std::optional<decltype(increment)>{increment},
                 std::optional<int>{5}) == std::optional<int>{6});
}

// ============================================================================
// as-functor-presentation: Monad::as_functor(), the one visible free call
// that names which functor a monad object presents. See
// docs/transpose/decisions.md#functor-monad-grounding.
// ============================================================================

TEST_CASE("monad: as_functor is usable in a constant expression and empty") {
    constexpr auto &m = bt::monad<std::optional<int>>;
    constexpr auto f = m.as_functor();
    static_assert(std::is_empty_v<decltype(f)>);
    (void)f;
}

namespace {
// The motivating scenario: an algorithm writer holding an opaque monad
// object needing to call a Functor-constrained function. as_functor() is
// what makes the call site sayable.
template <class OBJ>
    requires bt::functor_object<OBJ, std::optional<int>>
auto increment_via_functor_object(const OBJ &f, std::optional<int> value) {
    return f.fmap([](int x) { return x + 1; }, value);
}
} // namespace

TEST_CASE(
    "monad: as_functor lets a functor_object-constrained algorithm accept "
    "an opaque monad object") {
    const auto &m = bt::monad<std::optional<int>>;

    REQUIRE(
        increment_via_functor_object(m.as_functor(), std::optional<int>{41}) ==
        std::optional<int>{42});
    REQUIRE(increment_via_functor_object(
                m.as_functor(), std::optional<int>{}) == std::optional<int>{});
}

TEST_CASE("monad: as_functor's fmap computes what the monad's own fmap "
          "computes") {
    const auto &m = bt::monad<std::optional<int>>;
    auto f = m.as_functor();
    auto increment = [](int x) { return x + 1; };

    REQUIRE(f.fmap(increment, std::optional<int>{41}) ==
            m.fmap(increment, std::optional<int>{41}));
    REQUIRE(f.fmap(increment, std::optional<int>{}) ==
            m.fmap(increment, std::optional<int>{}));
}

TEST_CASE(
    "monad: as_functor's replace is the fmap-derived answer when the monad "
    "supplies no native replace") {
    const auto &m = bt::monad<std::optional<int>>;
    auto f = m.as_functor();

    REQUIRE(f.replace(std::optional<int>{5}, 9) == std::optional<int>{9});
}

namespace {
// An Impl with pure + bind (so Monad grows the Functor basis fmap), whose
// Map additionally supplies its own native `replace` -- distinguishable
// from the fmap derivation by doubling the replacement, the same marker
// shape as functor.test.cpp's MarkedReplaceImpl. as_functor() wraps this Map
// as derive_functor<ThisMap>, so Functor::replace's native-preference probe
// lands on the Map's own `replace`, not Monad's Impl.
struct NativeReplaceMonadImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class A, class F>
    [[maybe_unused]] auto bind(this auto &&, const std::optional<A> &ma, F &&f)
        -> std::remove_cvref_t<std::invoke_result_t<F, const A &>> {
        using Result = std::remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma)
            return Result{};
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }
};

struct NativeReplaceMonadMap : bt::derive_monad<NativeReplaceMonadImpl> {
    template <class T, class U>
    auto replace(this auto &&, T &&, U &&replacement) -> std::optional<int> {
        return std::optional<int>{replacement + replacement};
    }
};
} // namespace

TEST_CASE("monad: as_functor is optimization-preserving -- a native "
          "Impl::replace wins over the derivation") {
    NativeReplaceMonadMap m{};
    auto f = m.as_functor();

    REQUIRE(f.replace(std::optional<int>{5}, 3) == std::optional<int>{6});
}

// The derived operations are constant-evaluable: each is pinned by a
// static_assert over the optional instance.
TEST_CASE("monad: the derived operations are constexpr") {
    constexpr const auto &m = bt::monad<std::optional<int>>;
    constexpr auto half = [](int x) {
        return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
    };
    constexpr auto digits = [](int x, int y) { return x * 10 + y; };
    static_assert(m.fmap([](int x) { return x + 1; }, std::optional<int>{41}) ==
                  std::optional<int>{42});
    static_assert(m.invoke(digits, std::optional<int>{1},
                           std::optional<int>{2}) == std::optional<int>{12});
    static_assert(m.join(std::optional<std::optional<int>>{
                      std::optional<int>{3}}) == std::optional<int>{3});
    static_assert(m.kleisli(half, half)(8) == std::optional<int>{2});
    static_assert(m.kleisli(half, half)(6) == std::optional<int>{});
    constexpr auto plus_one = [](int x) { return x + 1; };
    static_assert(m.ap(std::optional{plus_one}, std::optional<int>{41}) ==
                  std::optional<int>{42});
    static_assert(m.bind_with(m, std::optional<int>{8}, half) ==
                  std::optional<int>{4});
    SUCCEED();
}

// steve-downey/typeclass#8: bind_with forwards to the delegate's bind, and
// is constrained on that call, so a probe agrees with a probe of bind.
namespace {

struct NotCallable {};

template <class MAP, class F>
concept has_bind =
    requires(const MAP &m, F f) { m.bind(std::optional<int>{1}, f); };

template <class MAP, class F>
concept has_bind_with =
    requires(const MAP &m, F f) { m.bind_with(m, std::optional<int>{1}, f); };

} // namespace

TEST_CASE("monad: bind_with is a soft failure when the delegate rejects "
          "the call") {
    using Map = std::remove_cvref_t<decltype(bt::monad<std::optional<int>>)>;
    auto half = [](int x) {
        return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
    };
    auto not_monadic = [](int x) { return x; };
    STATIC_REQUIRE(has_bind_with<Map, decltype(half)>);
    STATIC_REQUIRE(!has_bind_with<Map, NotCallable>);
    STATIC_REQUIRE(has_bind_with<Map, NotCallable> ==
                   has_bind<Map, NotCallable>);
    STATIC_REQUIRE(has_bind_with<Map, decltype(not_monadic)> ==
                   has_bind<Map, decltype(not_monadic)>);
    const auto &m = bt::monad<std::optional<int>>;
    REQUIRE(m.bind_with(m, std::optional<int>{8}, half) ==
            std::optional<int>{4});
}

// steve-downey/typeclass#12: the bind-basis derivations of fmap and invoke
// are constrained on the continuation they bind with, and kleisli's
// composition on the bind it makes when called.
namespace {

template <class MAP, class F>
concept has_optional_fmap =
    requires(const MAP &m, F f) { m.fmap(f, std::optional<int>{1}); };

template <class MAP, class F, class... ARGS>
concept has_invoke =
    requires(const MAP &m, F f, ARGS... args) { m.invoke(f, args...); };

template <class MAP, class F, class G>
concept has_kleisli = requires(const MAP &m, F f, G g) { m.kleisli(f, g); };

template <class COMPOSITION, class ARGUMENT>
concept callable_with = requires(const COMPOSITION &c, ARGUMENT a) { c(a); };

auto plus_one = [](int x) { return x + 1; };
auto plus = [](int x, int y) { return x + y; };
using TakesString = decltype([](const std::string &s) { return s.size(); });
auto half_of = [](int x) {
    return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
};

} // namespace

TEST_CASE("monad: fmap derived from bind rejects functions it cannot apply") {
    using Map = std::remove_cvref_t<decltype(bt::monad<std::optional<int>>)>;
    STATIC_REQUIRE(has_optional_fmap<Map, decltype(plus_one)>);
    STATIC_REQUIRE(!has_optional_fmap<Map, NotCallable>);
    STATIC_REQUIRE(!has_optional_fmap<Map, TakesString>);
    STATIC_REQUIRE(!has_optional_fmap<Map, decltype(plus)>);

    const auto &m = bt::monad<std::optional<int>>;
    REQUIRE(m.fmap(plus_one, std::optional<int>{41}) == std::optional<int>{42});
    REQUIRE(m.fmap(plus_one, std::optional<int>{}) == std::optional<int>{});
}

TEST_CASE("monad: invoke derived from bind rejects functions it cannot "
          "apply") {
    using Map = std::remove_cvref_t<decltype(bt::monad<std::optional<int>>)>;
    using O = std::optional<int>;
    STATIC_REQUIRE(has_invoke<Map, decltype(plus_one), O>);
    STATIC_REQUIRE(has_invoke<Map, decltype(plus), O, O>);
    STATIC_REQUIRE(!has_invoke<Map, NotCallable, O>);
    STATIC_REQUIRE(!has_invoke<Map, NotCallable, O, O>);
    // Too few, or too many, arguments for the function.
    STATIC_REQUIRE(!has_invoke<Map, decltype(plus), O>);
    STATIC_REQUIRE(!has_invoke<Map, decltype(plus_one), O, O>);
    STATIC_REQUIRE(!has_invoke<Map, TakesString, O>);

    const auto &m = bt::monad<std::optional<int>>;
    REQUIRE(m.invoke(plus, O{40}, O{2}) == O{42});
    REQUIRE(m.invoke(plus, O{40}, O{}) == O{});
}

TEST_CASE("monad: kleisli's composition is checked when it is called") {
    using Map = std::remove_cvref_t<decltype(bt::monad<std::optional<int>>)>;
    const auto &m = bt::monad<std::optional<int>>;

    // Building a composition cannot check f and g -- the argument type is
    // not known yet -- so it needs only that they can be moved in.
    STATIC_REQUIRE(has_kleisli<Map, decltype(half_of), decltype(half_of)>);
    STATIC_REQUIRE(has_kleisli<Map, NotCallable, NotCallable>);

    using Good = decltype(m.kleisli(half_of, half_of));
    STATIC_REQUIRE(callable_with<Good, int>);
    STATIC_REQUIRE(!callable_with<Good, std::string>);
    STATIC_REQUIRE(
        !callable_with<decltype(m.kleisli(NotCallable{}, NotCallable{})), int>);
    // f is fine, but g cannot continue its result.
    STATIC_REQUIRE(
        !callable_with<decltype(m.kleisli(half_of, NotCallable{})), int>);
    // f's result is not monadic, so there is nothing for g to continue.
    STATIC_REQUIRE(!callable_with<decltype(m.kleisli(plus_one, half_of)), int>);

    REQUIRE(m.kleisli(half_of, half_of)(8) == std::optional<int>{2});
    REQUIRE(m.kleisli(half_of, half_of)(6) == std::optional<int>{});
}

// steve-downey/typeclass#13: optional's bind requires the continuation to
// return an optional, so a -> b is rejected where a -> m b is meant, and
// join rejects a value that is not nested. expected's bind already required
// an expected (with the same error type, or through the graded bind, one
// whose error grade joins with it); it is probed here alongside.
namespace {

template <class MA, class F>
concept has_bind_object = requires(MA ma, F f) { bt::bind(ma, f); };

template <class MMA>
concept has_join_object = requires(MMA mma) { bt::join(mma); };

using NotMonadic = decltype([](int x) { return x + 1; });
using ToOptional = decltype([](int x) { return std::optional<int>{x}; });
using ToExpected =
    decltype([](int x) { return std::expected<int, std::string>{x}; });
using ToOtherError = decltype([](int x) { return std::expected<int, int>{x}; });

} // namespace

TEST_CASE("monad: optional bind rejects a continuation that does not return "
          "an optional") {
    using O = std::optional<int>;
    STATIC_REQUIRE(has_bind_object<O, ToOptional>);
    STATIC_REQUIRE(!has_bind_object<O, NotMonadic>);
    STATIC_REQUIRE(!has_bind_object<O, ToExpected>);
    STATIC_REQUIRE(!has_bind_object<O, NotCallable>);

    using Map = std::remove_cvref_t<decltype(bt::monad<O>)>;
    STATIC_REQUIRE(has_bind<Map, ToOptional>);
    STATIC_REQUIRE(!has_bind<Map, NotMonadic>);
    STATIC_REQUIRE(!has_bind_with<Map, NotMonadic>);

    STATIC_REQUIRE(has_join_object<std::optional<O>>);
    STATIC_REQUIRE(!has_join_object<O>);

    REQUIRE(bt::bind(O{}, ToOptional{}) == O{});
    REQUIRE(bt::bind(O{1}, ToOptional{}) == O{1});
    REQUIRE(bt::join(std::optional<O>{O{2}}) == O{2});
    REQUIRE(bt::join(std::optional<O>{}) == O{});
}

TEST_CASE("monad: expected bind rejects a continuation that does not return "
          "an expected with the same error") {
    using E = std::expected<int, std::string>;
    STATIC_REQUIRE(has_bind_object<E, ToExpected>);
    STATIC_REQUIRE(!has_bind_object<E, NotMonadic>);
    STATIC_REQUIRE(!has_bind_object<E, ToOptional>);
    // A different error type is not a different context: the graded bind
    // joins the two into an error set.
    STATIC_REQUIRE(has_bind_object<E, ToOtherError>);
    STATIC_REQUIRE(has_join_object<std::expected<E, std::string>>);
    STATIC_REQUIRE(!has_join_object<E>);
}
