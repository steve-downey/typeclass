// tests/beman/transpose/apply.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/apply.hpp>
#include <beman/transpose/array.hpp>
#include <beman/transpose/zip_list.hpp>

#include "test_support.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

namespace {

// An Impl with pure + invoke, and a native map that ignores its arguments
// and returns a sentinel the invoke derivation could never produce.
struct MarkedMapImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class FIRST, class... REST>
    [[maybe_unused]] auto invoke(this auto &&, FUNCTION &&function,
                                 const std::optional<FIRST> &first,
                                 const std::optional<REST> &...rest)
        -> std::optional<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        if (first.has_value() && (... && rest.has_value())) {
            return std::optional<Result>{
                std::invoke(function, *first, *rest...)};
        }
        return std::optional<Result>{};
    }

    template <class FUNCTION, class ARGUMENT>
    auto map(this auto &&, FUNCTION &&, ARGUMENT &&) -> std::optional<int> {
        return std::optional<int>{-1};
    }
};

struct MarkedMapMap : bt::derive_applicative<MarkedMapImpl> {};

// Native map is constrained to std::optional<int>, so a different element
// type on the same Map still reaches the invoke derivation.
struct PerInstantiationMapImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function,
                const std::optional<FIRST> &first,
                const std::optional<REST> &...rest)
        -> std::optional<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        if (first.has_value() && (... && rest.has_value())) {
            return std::optional<Result>{
                std::invoke(function, *first, *rest...)};
        }
        return std::optional<Result>{};
    }

    template <class FUNCTION>
    auto map(this auto &&, FUNCTION &&, const std::optional<int> &)
        -> std::optional<int> {
        return std::optional<int>{-1};
    }
};

struct PerInstantiationMapMap
    : bt::derive_applicative<PerInstantiationMapImpl> {};

// An Impl with a native zip_with that ignores its arguments and returns a
// sentinel the invoke derivation could never produce.
struct MarkedZipWithImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class FIRST, class... REST>
    [[maybe_unused]] auto invoke(this auto &&, FUNCTION &&function,
                                 const std::optional<FIRST> &first,
                                 const std::optional<REST> &...rest)
        -> std::optional<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        if (first.has_value() && (... && rest.has_value())) {
            return std::optional<Result>{
                std::invoke(function, *first, *rest...)};
        }
        return std::optional<Result>{};
    }

    template <class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    auto zip_with(this auto &&, FUNCTION &&, FIRST_ARGUMENT &&,
                  SECOND_ARGUMENT &&) -> std::optional<int> {
        return std::optional<int>{-1};
    }
};

struct MarkedZipWithMap : bt::derive_applicative<MarkedZipWithImpl> {};

// An Impl with pure + invoke, and native lift/discard_first/discard_second
// that each return a sentinel their pure/invoke derivations could never
// produce.
struct AllNativeImpl {
    template <class VALUE>
    [[maybe_unused]] auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class FIRST, class... REST>
    [[maybe_unused]] auto invoke(this auto &&, FUNCTION &&function,
                                 const std::optional<FIRST> &first,
                                 const std::optional<REST> &...rest)
        -> std::optional<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        if (first.has_value() && (... && rest.has_value())) {
            return std::optional<Result>{
                std::invoke(function, *first, *rest...)};
        }
        return std::optional<Result>{};
    }

    template <class VALUE>
    auto lift(this auto &&, VALUE &&) -> std::optional<int> {
        return std::optional<int>{-1};
    }

    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    auto discard_first(this auto &&, FIRST_ARGUMENT &&, SECOND_ARGUMENT &&)
        -> std::optional<int> {
        return std::optional<int>{-2};
    }

    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    auto discard_second(this auto &&, FIRST_ARGUMENT &&, SECOND_ARGUMENT &&)
        -> std::optional<int> {
        return std::optional<int>{-3};
    }
};

struct AllNativeMap : bt::derive_applicative<AllNativeImpl> {};

// An Impl with pure + ap and no invoke at all -- the mirror of AllNativeImpl,
// leaning entirely on the base's synthesis rather than supplying any
// operation natively. This is the regression witness for
// derived-op-native-preference: before this step, map, zip_with,
// discard_first and discard_second each vanished from overload resolution
// for this Impl, because their second alternative probed Impl for an
// invoke it can never have -- invoke here only ever exists as something the
// base synthesizes from ap.
struct ApOnlyImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return std::optional<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class ARGUMENT>
    auto ap(this auto &&, const std::optional<FUNCTION> &function,
            const std::optional<ARGUMENT> &argument)
        -> std::optional<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const ARGUMENT &>>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const ARGUMENT &>>;
        if (function.has_value() && argument.has_value()) {
            return std::optional<Result>{std::invoke(*function, *argument)};
        }
        return std::optional<Result>{};
    }
};

struct ApOnlyMap : bt::derive_applicative<ApOnlyImpl> {};

// An ap-only Impl over std::vector with the list (cartesian) ap: one held
// function is called once for every argument element. The witness for the
// invoke-from-ap currying binding by copy -- a partial that moved its bound
// arguments into the next one would hand every call after the first a
// moved-from value.
struct VectorApOnlyImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::vector<std::remove_cvref_t<VALUE>> {
        return std::vector<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class ARGUMENT>
    auto ap(this auto &&, const std::vector<FUNCTION> &functions,
            const std::vector<ARGUMENT> &arguments)
        -> std::vector<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const ARGUMENT &>>> {
        std::vector<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const ARGUMENT &>>>
            results;
        for (auto function : functions) {
            for (const auto &argument : arguments) {
                results.push_back(std::invoke(function, argument));
            }
        }
        return results;
    }
};

struct VectorApOnlyMap : bt::derive_applicative<VectorApOnlyImpl> {};

template <class MAP, class F, class... ARGS>
concept has_invoke = requires(const MAP &m, const F &f, const ARGS &...args) {
    m.invoke(f, args...);
};

struct NotCallable {};

template <class MAP, class APPLICATIVE_MAP, class F, class... ARGS>
concept has_invoke_with =
    requires(const MAP &m, const APPLICATIVE_MAP &delegate, const F &f,
             const ARGS &...args) { m.invoke_with(delegate, f, args...); };

template <class F, class... ARGS>
concept has_invoke_object =
    std::invocable<decltype(bt::invoke) const &, F, ARGS...>;

// Three program-defined contexts, one per way an instance can be written,
// so each gets its own applicative<> specialization. The instances are
// steve-downey/typeclass#6 and #7's reproductions. invoke_box's object
// supplies invoke and does not re-expose it; reexposed_box's re-exposes it
// with a using-declaration, the way the library's own instances do;
// ap_box's supplies only ap. Re-exposing must be an optimization, never the
// thing that keeps a probe soft.
template <class V>
struct invoke_box {
    using value_type = V;
    V v;
};
template <class V>
struct reexposed_box {
    using value_type = V;
    V v;
};
template <class V>
struct ap_box {
    using value_type = V;
    V v;
};

template <template <class> class BOX>
struct box_invoke_impl {
    template <class X>
    constexpr auto pure(this auto &&, X &&x) -> BOX<std::remove_cvref_t<X>> {
        return {std::forward<X>(x)};
    }
    template <class F, class A>
    constexpr auto invoke(this auto &&, F &&f, const BOX<A> &a)
        -> BOX<std::remove_cvref_t<std::invoke_result_t<F &, const A &>>> {
        return {f(a.v)};
    }
};

struct box_ap_impl {
    template <class X>
    constexpr auto pure(this auto &&, X &&x) -> ap_box<std::remove_cvref_t<X>> {
        return {std::forward<X>(x)};
    }
    template <class F, class A>
    constexpr auto ap(this auto &&, const ap_box<F> &f, const ap_box<A> &a)
        -> ap_box<
            std::remove_cvref_t<std::invoke_result_t<const F &, const A &>>> {
        return {f.v(a.v)};
    }
};

struct invoke_box_map : bt::derive_applicative<box_invoke_impl<invoke_box>> {};
struct reexposed_box_map
    : bt::derive_applicative<box_invoke_impl<reexposed_box>> {
    using box_invoke_impl<reexposed_box>::invoke;
    using box_invoke_impl<reexposed_box>::pure;
};
struct ap_box_map : bt::derive_applicative<box_ap_impl> {
    using box_ap_impl::pure;
};

template <class MAP, class F, class A>
concept has_map =
    requires(const MAP &m, const F &f, const A &a) { m.map(f, a); };

template <class MAP, class F, class A, class B>
concept has_zip_with = requires(const MAP &m, const F &f, const A &a,
                                const B &b) { m.zip_with(f, a, b); };

// Applicable to std::string but not to int -- witnesses that the derived
// probe on the shipped invoke-basis object is a real constraint, not one
// vacuously satisfied by substitution alone.
struct StringOnlyCallable {
    auto operator()(const std::string &s) const -> std::string {
        return s + "!";
    }
};

} // namespace

template <class V>
inline constexpr auto bt::applicative<invoke_box<V>> = invoke_box_map{};
template <class V>
inline constexpr auto bt::applicative<reexposed_box<V>> = reexposed_box_map{};
template <class V>
inline constexpr auto bt::applicative<ap_box<V>> = ap_box_map{};

TEST_CASE("apply: optional invoke combines effectful arguments") {
    const auto &app = bt::applicative<std::optional<int>>;
    auto add = [](int a, int b, int c) { return a + b + c; };
    REQUIRE(app.invoke(add, std::optional<int>{1}, std::optional<int>{2},
                       std::optional<int>{3}) == std::optional<int>{6});
    REQUIRE(app.invoke(add, std::optional<int>{1}, std::optional<int>{},
                       std::optional<int>{3}) == std::optional<int>{});
}

TEST_CASE("apply: ap is a supported basis and secondary operation") {
    // invoke is the user-facing interface; ap -- one-step application of a
    // callable-in-context -- is the classic basis, derived here from
    // optional's invoke basis and available as a secondary operation.
    const auto &app = bt::applicative<std::optional<int>>;
    auto lifted = app.pure([](int x) { return x * 10; });
    REQUIRE(app.ap(lifted, std::optional<int>{5}) == std::optional<int>{50});
    REQUIRE(app.ap(app.pure([](int x) { return x * 10; }),
                   std::optional<int>{}) == std::optional<int>{});

    // ap agrees with the invoke spelling of the same application.
    auto call = [](const auto &f, int x) { return f(x); };
    REQUIRE(app.ap(lifted, std::optional<int>{5}) ==
            app.invoke(call, lifted, std::optional<int>{5}));

    using Map = std::remove_cvref_t<decltype(app)>;
    STATIC_REQUIRE(bt::test::has_apply_form<Map, std::optional<int (*)(int)>,
                                            std::optional<int>>);

    // The detail:: free-function form works too.
    REQUIRE(bt::detail::ap(app, lifted, std::optional<int>{5}) ==
            std::optional<int>{50});
}

TEST_CASE("apply: identity and homomorphism laws on optional") {
    using bt::test::check_applicative_homomorphism_law;
    using bt::test::check_applicative_identity_law;
    REQUIRE(check_applicative_identity_law(std::optional<int>{7}));
    REQUIRE(check_applicative_homomorphism_law<std::optional<int>>(
        [](int x) { return x + 1; }, 41));
    // Generalized to the n-ary core: invoke(f, pure(x)...) == pure(f(x...)).
    REQUIRE(check_applicative_homomorphism_law<std::optional<int>>(
        [](int a, int b) { return a + b; }, 20, 22));
    REQUIRE(check_applicative_homomorphism_law<std::optional<int>>(
        [](int a, int b, int c) { return a * b * c; }, 2, 3, 7));
}

TEST_CASE("apply: interchange and composition laws on optional") {
    using bt::test::check_applicative_interchange_law;
    using bt::test::check_functor_composition_law;
    std::optional functions{[](int x) { return x - 5; }};
    REQUIRE(check_applicative_interchange_law(functions, 12));
    REQUIRE(check_functor_composition_law([](int x) { return x * 2; },
                                          [](int x) { return x + 1; },
                                          std::optional<int>{20}));
    REQUIRE(check_functor_composition_law([](int x) { return x * 2; },
                                          [](int x) { return x + 1; },
                                          std::optional<int>{}));
}

TEST_CASE("apply: laws hold for the Identity context") {
    using bt::test::check_applicative_homomorphism_law;
    using bt::test::check_applicative_identity_law;
    using bt::test::check_functor_composition_law;
    using Identity = bt::test::Identity<int>;
    REQUIRE(check_applicative_identity_law(Identity{9}));
    REQUIRE(check_applicative_homomorphism_law<Identity>(
        [](int a, int b) { return a - b; }, 50, 8));
    REQUIRE(check_functor_composition_law(
        [](int x) { return x / 2; }, [](int x) { return x + 6; }, Identity{4}));
}

TEST_CASE("apply: invoke_with delegates to another applicative map") {
    const auto &optional_map = bt::applicative<std::optional<int>>;
    const auto &zip_map = bt::applicative<bt::zip_list<int>>;
    auto result = optional_map.invoke_with(
        zip_map, [](int a, int b) { return a + b; },
        bt::zip_list<int>{{1, 2, 3}}, bt::zip_list<int>{{10, 20, 30}});
    REQUIRE(result.data == std::vector<int>{11, 22, 33});
}

TEST_CASE("apply: map prefers a native Impl::map") {
    MarkedMapMap m{};
    REQUIRE(m.map([](int x) { return x + 1; }, std::optional<int>{5}) ==
            std::optional<int>{-1});
}

TEST_CASE("apply: map falls back to the invoke derivation") {
    const auto &m = bt::applicative<std::optional<int>>;
    REQUIRE(m.map([](int x) { return x + 1; }, std::optional<int>{5}) ==
            std::optional<int>{6});
}

TEST_CASE("apply: map native preference is per instantiation") {
    PerInstantiationMapMap m{};

    // std::optional<int>: the native map fires.
    REQUIRE(m.map([](int x) { return x + 1; }, std::optional<int>{5}) ==
            std::optional<int>{-1});

    // std::optional<std::string>: no native map exists for this element
    // type, so the same member falls back to the invoke derivation -- a
    // property a Map-level `using` could never express, since `using`
    // selects a name for every instantiation at once.
    REQUIRE(m.map([](const std::string &s) { return s + "!"; },
                  std::optional<std::string>{"x"}) ==
            std::optional<std::string>{"x!"});
}

TEST_CASE("apply: zip_with prefers a native Impl::zip_with") {
    MarkedZipWithMap m{};
    REQUIRE(m.zip_with([](int a, int b) { return a + b; },
                       std::optional<int>{2},
                       std::optional<int>{3}) == std::optional<int>{-1});
}

TEST_CASE("apply: zip_with falls back to the invoke derivation") {
    const auto &m = bt::applicative<std::optional<int>>;
    REQUIRE(m.zip_with([](int a, int b) { return a + b; },
                       std::optional<int>{2},
                       std::optional<int>{3}) == std::optional<int>{5});
}

TEST_CASE("apply: lift prefers a native Impl::lift") {
    AllNativeMap m{};
    REQUIRE(m.lift(7) == std::optional<int>{-1});
}

TEST_CASE("apply: lift falls back to the pure derivation") {
    const auto &m = bt::applicative<std::optional<int>>;
    REQUIRE(m.lift(7) == std::optional<int>{7});
}

TEST_CASE("apply: discard_first prefers a native Impl::discard_first") {
    AllNativeMap m{};
    REQUIRE(m.discard_first(std::optional<int>{2}, std::optional<int>{3}) ==
            std::optional<int>{-2});
}

TEST_CASE("apply: discard_first falls back to the invoke derivation") {
    const auto &m = bt::applicative<std::optional<int>>;
    REQUIRE(m.discard_first(std::optional<int>{2}, std::optional<int>{3}) ==
            std::optional<int>{3});
}

TEST_CASE("apply: discard_second prefers a native Impl::discard_second") {
    AllNativeMap m{};
    REQUIRE(m.discard_second(std::optional<int>{2}, std::optional<int>{3}) ==
            std::optional<int>{-3});
}

TEST_CASE("apply: discard_second falls back to the invoke derivation") {
    const auto &m = bt::applicative<std::optional<int>>;
    REQUIRE(m.discard_second(std::optional<int>{2}, std::optional<int>{3}) ==
            std::optional<int>{2});
}

TEST_CASE("apply: an ap-only Impl still derives the full surface") {
    // No native invoke anywhere on ApOnlyImpl -- invoke and everything
    // derived from it comes entirely from the ap basis.
    ApOnlyMap m{};
    REQUIRE(m.invoke([](int a, int b) { return a * b; }, std::optional<int>{6},
                     std::optional<int>{14}) == std::optional<int>{84});
    REQUIRE(m.ap(m.pure([](int x) { return x * 2; }), std::optional<int>{21}) ==
            std::optional<int>{42});

    REQUIRE(m.map([](int x) { return x + 1; }, std::optional<int>{41}) ==
            std::optional<int>{42});
    REQUIRE(m.map([](int x) { return x + 1; }, std::optional<int>{}) ==
            std::optional<int>{});

    REQUIRE(m.lift(7) == std::optional<int>{7});

    REQUIRE(m.zip_with([](int a, int b) { return a + b; },
                       std::optional<int>{2},
                       std::optional<int>{3}) == std::optional<int>{5});
    REQUIRE(m.zip_with([](int a, int b) { return a + b; }, std::optional<int>{},
                       std::optional<int>{3}) == std::optional<int>{});

    REQUIRE(m.discard_first(std::optional<int>{2}, std::optional<int>{3}) ==
            std::optional<int>{3});
    REQUIRE(m.discard_second(std::optional<int>{2}, std::optional<int>{3}) ==
            std::optional<int>{2});
}

TEST_CASE("apply: an inapplicable callable still makes map and zip_with "
          "disappear, not hard-error") {
    using Map =
        std::remove_cvref_t<decltype(bt::applicative<std::optional<int>>)>;
    static_assert(!has_map<Map, StringOnlyCallable, std::optional<int>>);
    static_assert(!has_zip_with<Map, StringOnlyCallable, std::optional<int>,
                                std::optional<int>>);
}

TEST_CASE("apply: an ap-only invoke rejects a callable that cannot take "
          "the arguments") {
    // invoke derived from ap curries through a partial. Its arity is the
    // number of arguments invoke was given, so a callable that cannot take
    // them -- of the wrong type, or wanting more or fewer -- leaves invoke
    // out of overload resolution instead of yielding a half-applied
    // function in the context.
    using O = std::optional<int>;
    auto unary = [](int a) { return a; };
    auto binary = [](int a, int b) { return a + b; };
    STATIC_REQUIRE(!has_invoke<ApOnlyMap, NotCallable, O>);
    STATIC_REQUIRE(!has_invoke<ApOnlyMap, NotCallable, O, O>);
    STATIC_REQUIRE(!has_invoke<ApOnlyMap, StringOnlyCallable, O>);
    STATIC_REQUIRE(!has_invoke<ApOnlyMap, decltype(binary), O>);
    STATIC_REQUIRE(!has_invoke<ApOnlyMap, decltype(unary), O, O>);
    STATIC_REQUIRE(has_invoke<ApOnlyMap, decltype(unary), O>);
    STATIC_REQUIRE(has_invoke<ApOnlyMap, decltype(binary), O, O>);

    // map and zip_with probe through self.invoke, so they inherit the
    // constraint rather than being vacuously satisfied.
    STATIC_REQUIRE(!has_map<ApOnlyMap, NotCallable, O>);
    STATIC_REQUIRE(!has_map<ApOnlyMap, decltype(binary), O>);
    STATIC_REQUIRE(!has_zip_with<ApOnlyMap, decltype(unary), O, O>);
    STATIC_REQUIRE(has_zip_with<ApOnlyMap, decltype(binary), O, O>);

    // The result is the saturated application, never a partial.
    ApOnlyMap m{};
    STATIC_REQUIRE(std::is_same_v<decltype(m.invoke(binary, O{1}, O{2})), O>);
    REQUIRE(m.invoke(binary, O{1}, O{2}) == O{3});
}

TEST_CASE("apply: an ap-only invoke agrees with the native invoke on what "
          "it accepts") {
    using O = std::optional<int>;
    using Native = std::remove_cvref_t<decltype(bt::applicative<O>)>;
    auto unary = [](int a) { return a; };
    auto binary = [](int a, int b) { return a + b; };
    STATIC_REQUIRE(has_invoke<Native, decltype(unary), O> ==
                   has_invoke<ApOnlyMap, decltype(unary), O>);
    STATIC_REQUIRE(has_invoke<Native, decltype(binary), O> ==
                   has_invoke<ApOnlyMap, decltype(binary), O>);
    STATIC_REQUIRE(has_invoke<Native, decltype(unary), O, O> ==
                   has_invoke<ApOnlyMap, decltype(unary), O, O>);
    STATIC_REQUIRE(has_invoke<Native, NotCallable, O> ==
                   has_invoke<ApOnlyMap, NotCallable, O>);
}

// A probe of invoke must be false, not a hard error, whatever the instance
// leaves out: steve-downey/typeclass#6 (an invoke-basis object without the
// using-declaration) and #7 (an ap-only object, where the probe said yes).
TEMPLATE_TEST_CASE("apply: an invalid invoke is a soft failure for every "
                   "way of writing an instance",
                   "", invoke_box<int>, reexposed_box<int>, ap_box<int>) {
    using Box = TestType;
    using Map = std::remove_cvref_t<decltype(bt::applicative<Box>)>;
    auto increment = [](int x) { return x + 1; };
    using Increment = decltype(increment);

    STATIC_REQUIRE(has_invoke<Map, Increment, Box>);
    STATIC_REQUIRE(has_invoke_object<Increment, Box>);
    STATIC_REQUIRE(has_invoke_with<Map, Map, Increment, Box>);

    STATIC_REQUIRE(!has_invoke<Map, NotCallable, Box>);
    STATIC_REQUIRE(!has_invoke_object<NotCallable, Box>);
    STATIC_REQUIRE(!has_invoke_with<Map, Map, NotCallable, Box>);

    // A foreign context, and something that is not a context at all.
    STATIC_REQUIRE(!has_invoke<Map, Increment, std::optional<int>>);
    STATIC_REQUIRE(!has_invoke<Map, Increment, int>);

    REQUIRE(bt::applicative<Box>.invoke(increment, Box{1}).v == 2);
    REQUIRE(bt::invoke(increment, Box{1}).v == 2);
}

TEST_CASE("apply: invoke_with is a soft failure when the delegate rejects "
          "the call") {
    // invoke_with forwards to the delegate's invoke. Its constraint is that
    // call, so even the shipped optional object -- whose own invoke is
    // constrained -- does not turn an invoke_with probe into a hard error.
    using O = std::optional<int>;
    using Map = std::remove_cvref_t<decltype(bt::applicative<O>)>;
    auto increment = [](int x) { return x + 1; };
    STATIC_REQUIRE(has_invoke_with<Map, Map, decltype(increment), O>);
    STATIC_REQUIRE(!has_invoke_with<Map, Map, NotCallable, O>);
    STATIC_REQUIRE(!has_invoke_with<Map, Map, StringOnlyCallable, O>);
    const auto &app = bt::applicative<O>;
    REQUIRE(app.invoke_with(app, increment, O{41}) == O{42});
}

TEST_CASE("apply: an ap-only invoke reuses its bound arguments across calls") {
    // The list ap calls each held partial once per argument element, so the
    // bound strings must survive the first call.
    VectorApOnlyMap m{};
    using V = std::vector<std::string>;
    auto concat = [](const std::string &a, const std::string &b,
                     const std::string &c) { return a + b + c; };
    REQUIRE(m.invoke(concat, V{"a", "b"}, V{"x", "y"}, V{"1", "2"}) ==
            V{"ax1", "ax2", "ay1", "ay2", "bx1", "bx2", "by1", "by2"});
    STATIC_REQUIRE(!has_invoke<VectorApOnlyMap, NotCallable, V>);
}

// The derived operations are constant-evaluable: each is pinned by a
// static_assert over the optional instance. The callables are
// order-sensitive, so swapped operands give a different answer.
TEST_CASE("apply: the derived operations are constexpr") {
    constexpr const auto &a = bt::applicative<std::optional<int>>;
    constexpr auto digits = [](int x, int y) { return x * 10 + y; };
    static_assert(a.invoke(digits, std::optional<int>{1},
                           std::optional<int>{2}) == std::optional<int>{12});
    static_assert(a.invoke(digits, std::optional<int>{1},
                           std::optional<int>{}) == std::optional<int>{});
    static_assert(a.map([](int x) { return x + 1; }, std::optional<int>{41}) ==
                  std::optional<int>{42});
    static_assert(a.lift(7) == std::optional<int>{7});
    static_assert(a.zip_with(digits, std::optional<int>{3},
                             std::optional<int>{4}) == std::optional<int>{34});
    static_assert(
        a.discard_first(std::optional<int>{1}, std::optional<int>{2}) ==
        std::optional<int>{2});
    static_assert(
        a.discard_second(std::optional<int>{1}, std::optional<int>{2}) ==
        std::optional<int>{1});
    constexpr auto plus_one = [](int x) { return x + 1; };
    static_assert(a.ap(std::optional{plus_one}, std::optional<int>{41}) ==
                  std::optional<int>{42});
    static_assert(a.invoke_with(a, digits, std::optional<int>{5},
                                std::optional<int>{6}) ==
                  std::optional<int>{56});
    SUCCEED();
}

// steve-downey/typeclass#11: the array instance's invoke is constrained on
// the function taking one element of each operand, not only on the
// operands' shape, so a probe of it is a soft failure.
namespace {

template <class F, class... ARGS>
concept has_array_invoke = requires(F f, ARGS &&...args) {
    bt::invoke(f, std::forward<ARGS>(args)...);
};

using Array3 = std::array<int, 3>;
using MoveOnlyArray = std::array<std::unique_ptr<int>, 2>;
using AddTwo = decltype([](int x, int y) { return x + y; });
using Negate = decltype([](int x) { return -x; });
using TakesString = decltype([](const std::string &s) { return s.size(); });
// Accepts its element only as an rvalue, so only an rvalue operand works.
using Consumes =
    decltype([](std::unique_ptr<int> &&p) { return std::move(p); });

} // namespace

TEST_CASE("apply: array invoke rejects functions that cannot take the "
          "elements") {
    STATIC_REQUIRE(has_array_invoke<Negate, Array3>);
    STATIC_REQUIRE(has_array_invoke<AddTwo, Array3, Array3>);
    STATIC_REQUIRE(!has_array_invoke<NotCallable, Array3>);
    STATIC_REQUIRE(!has_array_invoke<NotCallable, Array3, Array3>);
    STATIC_REQUIRE(!has_array_invoke<AddTwo, Array3>);
    STATIC_REQUIRE(!has_array_invoke<Negate, Array3, Array3>);
    STATIC_REQUIRE(!has_array_invoke<TakesString, Array3>);

    // The element is passed in the operand's value category.
    STATIC_REQUIRE(has_array_invoke<Consumes, MoveOnlyArray>);
    STATIC_REQUIRE(!has_array_invoke<Consumes, MoveOnlyArray &>);

    const auto &app = bt::applicative<Array3>;
    STATIC_REQUIRE(
        has_invoke<std::remove_cvref_t<decltype(app)>, AddTwo, Array3, Array3>);
    STATIC_REQUIRE(
        !has_invoke<std::remove_cvref_t<decltype(app)>, NotCallable, Array3>);

    REQUIRE(bt::invoke([](int x, int y) { return x + y; }, Array3{1, 2, 3},
                       Array3{10, 20, 30}) == Array3{11, 22, 33});
}

// steve-downey/typeclass#18: a callable that returns void -- written for its
// side effect -- gives a result no context here can hold, so invoke and map
// reject it softly instead of naming optional<void> and the like outside the
// immediate context. steve-downey/typeclass#17: zip_list's invoke copies the
// callable, and its constraint says so.
namespace {

template <class MAP, class F, class... ARGS>
concept has_forwarded_invoke = requires(const MAP &m, F &&f, ARGS &&...args) {
    m.invoke(std::forward<F>(f), std::forward<ARGS>(args)...);
};

template <class MAP, class F, class ARG>
concept has_lifted_map = requires(const MAP &m, F f, ARG a) { m.map(f, a); };

using ReturnsVoid = decltype([](int) {});
using ReturnsNullopt = decltype([](int) { return std::nullopt; });

// A move-only callable: an rvalue can be moved into the copy zip_list's
// invoke makes, an lvalue cannot be copied into it.
struct MoveOnlyNegate {
    std::unique_ptr<int> state = std::make_unique<int>(0);
    auto operator()(int x) const -> int { return -x; }
};

struct Counting {
    int calls = 0;
    auto operator()(int x) -> int {
        ++calls;
        return x;
    }
};

} // namespace

TEST_CASE("apply: invoke and map reject a callable returning void") {
    using OMap =
        std::remove_cvref_t<decltype(bt::applicative<std::optional<int>>)>;
    using AMap = std::remove_cvref_t<decltype(bt::applicative<Array3>)>;
    using ZMap =
        std::remove_cvref_t<decltype(bt::applicative<bt::zip_list<int>>)>;
    using O = std::optional<int>;
    using Z = bt::zip_list<int>;

    STATIC_REQUIRE(has_forwarded_invoke<OMap, Negate, O>);
    STATIC_REQUIRE(!has_forwarded_invoke<OMap, ReturnsVoid, O>);
    STATIC_REQUIRE(!has_forwarded_invoke<OMap, ReturnsNullopt, O>);
    STATIC_REQUIRE(has_lifted_map<OMap, Negate, O>);
    STATIC_REQUIRE(!has_lifted_map<OMap, ReturnsVoid, O>);

    STATIC_REQUIRE(has_forwarded_invoke<AMap, Negate, Array3>);
    STATIC_REQUIRE(!has_forwarded_invoke<AMap, ReturnsVoid, Array3>);
    STATIC_REQUIRE(!has_array_invoke<ReturnsVoid, Array3>);

    STATIC_REQUIRE(has_forwarded_invoke<ZMap, Negate, Z>);
    STATIC_REQUIRE(!has_forwarded_invoke<ZMap, ReturnsVoid, Z>);
    STATIC_REQUIRE(!has_forwarded_invoke<ZMap, NotCallable, Z>);
}

TEST_CASE("apply: zip_list invoke states the copy it makes of the callable") {
    using ZMap =
        std::remove_cvref_t<decltype(bt::applicative<bt::zip_list<int>>)>;
    using Z = bt::zip_list<int>;

    STATIC_REQUIRE(has_forwarded_invoke<ZMap, MoveOnlyNegate, Z>);
    STATIC_REQUIRE(!has_forwarded_invoke<ZMap, MoveOnlyNegate &, Z>);
    STATIC_REQUIRE(
        has_forwarded_invoke<ZMap, std::reference_wrapper<MoveOnlyNegate>, Z>);

    // The copy is what runs; std::ref is how a caller keeps its own object.
    const auto &app = bt::applicative<Z>;
    Counting counting;
    std::ignore = app.invoke(counting, Z{std::vector<int>{1, 2, 3}});
    REQUIRE(counting.calls == 0);
    std::ignore = app.invoke(std::ref(counting), Z{std::vector<int>{1, 2, 3}});
    REQUIRE(counting.calls == 3);

    MoveOnlyNegate negate;
    REQUIRE(app.invoke(std::ref(negate), Z{std::vector<int>{1, 2}}) ==
            Z{std::vector<int>{-1, -2}});
}
