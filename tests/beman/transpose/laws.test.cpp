// tests/beman/transpose/laws.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "laws.hpp"

#include "test_support.hpp"

#include <beman/transpose/array.hpp>
#include <beman/transpose/error_set.hpp>
#include <beman/transpose/expected.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/grade.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/optional.hpp>
#include <beman/transpose/zip_list.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <expected>
#include <future>
#include <memory>
#include <optional>
#include <system_error>
#include <tuple>
#include <type_traits>

namespace bt = beman::transpose;
namespace laws = beman::transpose::test::laws;

// =========================================================================
// GRADE PROBES: three distinct named std enum types.
//
// Inside the interim type-ordering fallback's restrictions BY CONSTRUCTION
// -- named, external linkage, stable spelling -- so the harness never
// depends on the P2830 stand-in behaving outside what error_set.hpp
// documents. Three generators give a free semilattice of eight elements,
// which is the smallest sample that exercises every law shape: the unit at
// ∅, idempotence on the diagonal, commutativity off it, and associativity
// on triples that are not all comparable.
// =========================================================================

using empty_grade = bt::error_set<>;
using e_c = bt::error_set<std::errc>;
using e_i = bt::error_set<std::io_errc>;
using e_f = bt::error_set<std::future_errc>;
using e_ci = bt::error_set<std::errc, std::io_errc>;
using e_cf = bt::error_set<std::errc, std::future_errc>;
using e_if = bt::error_set<std::io_errc, std::future_errc>;
using e_cif = bt::error_set<std::errc, std::io_errc, std::future_errc>;

// The eight elements really are eight, not fewer: the interim ordering
// separates the three generators, so no two subsets collapsed into one type.
static_assert(!std::is_same_v<e_c, e_i>);
static_assert(!std::is_same_v<e_c, e_f>);
static_assert(!std::is_same_v<e_i, e_f>);
static_assert(!std::is_same_v<e_ci, e_cf>);
static_assert(!std::is_same_v<e_ci, e_if>);
static_assert(!std::is_same_v<e_cf, e_if>);
static_assert(!std::is_same_v<e_ci, e_cif>);

/** The shipped model, as a harness subject. */
struct error_set_model {
    using grades =
        laws::grade_sample<empty_grade, e_c, e_i, e_f, e_ci, e_cf, e_if, e_cif>;
    using values =
        laws::value_sample<int, std::optional<int>, std::unique_ptr<int>>;
};

static_assert(laws::check_graded_laws<error_set_model>());

// =========================================================================
// THE SECOND MODEL: the Boolean semilattice {⊥, ⊤}.
//
// TEST-ONLY. Where it lives long-term -- here, or shipped as std::optional's
// official grade registration -- is the still-open question
// docs/transpose/decisions.md#optional-grade-model, and this stage does not
// settle it. That is why the carrier below is a purpose-built `Fallible`, not
// std::optional: registering std::optional as a graded carrier would answer
// the open question by fiat, and would contradict the ∅-default assertions
// tests/beman/transpose/grade.test.cpp pins on it.
//
// Its job here is to be a LEAK DETECTOR, per
// docs/transpose/decisions.md#grade-generality: an abstraction with one model
// is renamed, not generic. Every place this fails to slide in cleanly is a
// framework/model leak and gets logged as a divergence -- never patched
// around, and never smoothed by special-casing the framework to accommodate
// it.
//
// It is also the smallest possible non-trivial grade algebra: two elements,
// no parameters, no type ordering, no canonicalization. Anything the
// framework demands of it is something the framework demands of every
// algebra, which is exactly what makes it diagnostic.
// =========================================================================

namespace beman_transpose_boolean_grade {

/** ⊥: this computation cannot fail. */
struct never_fails {};

/** ⊤: this computation may fail. The whole algebra is {⊥, ⊤} with ⊥ ⊑ ⊤. */
struct may_fail {};

/** The explicit Boolean carrier: a value at a Boolean grade.
 *
 * Deliberately a distinct type rather than std::optional -- see the section
 * comment. `Fallible<T, never_fails>` is the explicit uniform form; framework
 * re-indexing at the model bottom still strips back to bare `T`.
 */
template <class VALUE, class GRADE>
class Fallible {
  public:
    using value_type = VALUE;
    using grade_type = GRADE;

    constexpr Fallible()
        requires std::is_same_v<GRADE, may_fail>
    = default;

    // NOLINTNEXTLINE(*-explicit-constructor) -- promotion is a subsumption
    // coercion and must be implicit, exactly as expected's from-T is.
    constexpr Fallible(VALUE value) : d_value(std::move(value)) {}

    template <class OTHER_GRADE>
        requires bt::grade_subsumes_v<OTHER_GRADE, GRADE>
    constexpr Fallible(const Fallible<VALUE, OTHER_GRADE> &other)
        : d_value(other.d_value) {}

    constexpr auto has_value() const noexcept -> bool {
        return d_value.has_value();
    }

    constexpr auto operator*() const -> const VALUE & { return *d_value; }

    friend auto operator==(const Fallible &, const Fallible &)
        -> bool = default;

  private:
    template <class, class>
    friend class Fallible;

    std::optional<VALUE> d_value;
};

} // namespace beman_transpose_boolean_grade

namespace boolean_grade = beman_transpose_boolean_grade;

using boolean_grade::Fallible;
using boolean_grade::may_fail;
using boolean_grade::never_fails;

namespace beman::transpose {

// -- The algebra: four joins, two bottoms, four order facts. This part is
// -- exactly what a grade algebra should cost, and it slides in cleanly.

template <>
struct grade_join<never_fails, never_fails> {
    using type = never_fails;
};
template <>
struct grade_join<never_fails, may_fail> {
    using type = may_fail;
};
template <>
struct grade_join<may_fail, never_fails> {
    using type = may_fail;
};
template <>
struct grade_join<may_fail, may_fail> {
    using type = may_fail;
};

template <>
struct grade_bottom<never_fails> {
    using type = never_fails;
};
template <>
struct grade_bottom<may_fail> {
    using type = never_fails;
};

template <>
struct grade_subsumes<never_fails, never_fails> : std::true_type {};
template <>
struct grade_subsumes<never_fails, may_fail> : std::true_type {};
template <>
struct grade_subsumes<may_fail, never_fails> : std::false_type {};
template <>
struct grade_subsumes<may_fail, may_fail> : std::true_type {};

struct boolean_grade_model {};

template <>
struct grade_model<never_fails> {
    using type = boolean_grade_model;
};

template <>
struct grade_model<may_fail> {
    using type = boolean_grade_model;
};

template <class CONTEXT>
inline constexpr bool is_fallible_v = false;

template <class VALUE, class GRADE>
inline constexpr bool is_fallible_v<Fallible<VALUE, GRADE>> = true;

template <class VALUE, class GRADE>
struct grade_of<Fallible<VALUE, GRADE>> {
    using type = GRADE;
};

template <class CARRIER>
struct fallible_value_type {
    using type = CARRIER;
};

template <class VALUE, class GRADE>
struct fallible_value_type<Fallible<VALUE, GRADE>> {
    using type = VALUE;
};

template <class CARRIER>
using fallible_value_type_t =
    typename fallible_value_type<std::remove_cvref_t<CARRIER>>::type;

/** Promotion: a bare value re-indexed at ⊤ acquires the carrier. */
template <class VALUE>
struct rebind_grade<VALUE, may_fail> {
    using type = Fallible<VALUE, may_fail>;
};

/** Re-indexing a carrier at the grade it already has must not nest. */
template <class VALUE, class GRADE>
struct rebind_grade<Fallible<VALUE, GRADE>, may_fail> {
    using type = Fallible<VALUE, may_fail>;
};

/** Re-indexing at the model's own bottom yields the bare value. */
template <class VALUE>
struct rebind_grade<VALUE, never_fails> {
    using type = VALUE;
};

template <class VALUE, class GRADE>
struct rebind_grade<Fallible<VALUE, GRADE>, never_fails> {
    using type = VALUE;
};

template <class CARRIER>
constexpr auto fallible_failed(const CARRIER &carrier) -> bool {
    if constexpr (is_fallible_v<std::remove_cvref_t<CARRIER>>) {
        return !carrier.has_value();
    } else {
        return false;
    }
}

template <class CARRIER>
constexpr auto fallible_value(const CARRIER &carrier) -> decltype(auto) {
    if constexpr (is_fallible_v<std::remove_cvref_t<CARRIER>>) {
        return *carrier;
    } else {
        return (carrier);
    }
}

template <class VALUE_TYPE, class GRADE>
struct FallibleApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> Fallible<std::remove_cvref_t<VALUE>, GRADE> {
        return Fallible<std::remove_cvref_t<VALUE>, GRADE>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (is_fallible_v<std::remove_cvref_t<CARRIERS>> || ...) &&
                (detail::mixes_with_model<GRADE, CARRIERS> && ...)
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::mixed_result_t<
            GRADE,
            Fallible<
                std::remove_cvref_t<std::invoke_result_t<
                    FUNCTION &, const fallible_value_type_t<CARRIERS> &...>>,
                GRADE>,
            CARRIERS...> {
        using Result = std::remove_cvref_t<std::invoke_result_t<
            FUNCTION &, const fallible_value_type_t<CARRIERS> &...>>;
        using Returned =
            detail::mixed_result_t<GRADE, Fallible<Result, GRADE>, CARRIERS...>;

        if constexpr (std::is_same_v<Returned, Result>) {
            return std::invoke(function, fallible_value(operands)...);
        } else {
            if ((fallible_failed(operands) || ...)) {
                return Returned{};
            }
            return Returned{std::invoke(function, fallible_value(operands)...)};
        }
    }
};

template <class VALUE_TYPE, class GRADE>
struct FallibleApplicativeMap
    : derive_applicative<FallibleApplicativeImpl<VALUE_TYPE, GRADE>> {
    using FallibleApplicativeImpl<VALUE_TYPE, GRADE>::invoke;
    using FallibleApplicativeImpl<VALUE_TYPE, GRADE>::pure;
};

template <class VALUE_TYPE, class GRADE>
inline constexpr auto applicative<Fallible<VALUE_TYPE, GRADE>> =
    FallibleApplicativeMap<VALUE_TYPE, GRADE>{};

} // namespace beman::transpose

/** The Boolean semilattice, as a harness subject. Two elements is the whole
 * algebra, so the sample is exhaustive rather than representative. */
struct boolean_model {
    using grades = laws::grade_sample<never_fails, may_fail>;
    using values =
        laws::value_sample<int, std::optional<int>, std::unique_ptr<int>>;
};

static_assert(laws::check_graded_laws<boolean_model>());

// =========================================================================
// MODEL-DISPATCHED MIXING.
//
// The harness above checks the ALGEBRA and the CARRIER TRAITS, and both
// models pass. This block reaches the place the algebra exists for: a real
// applicative mixing point. The deduced grade must be the framework
// `grade_join_t` of the operands' semantic `grade_of` readings, and a second
// model must drive an actual deduction rather than only passing laws.
// =========================================================================

namespace mixing_point {

constexpr auto add(const int &lhs, const int &rhs) -> int { return lhs + rhs; }

using bare_c = std::expected<int, std::errc>;
using bare_i = std::expected<int, std::io_errc>;
using graded_c = std::expected<int, e_c>;
using graded_i = std::expected<int, e_i>;
using boolean_may = Fallible<int, may_fail>;
using boolean_never = Fallible<int, never_fails>;

using graded_mix = decltype(bt::applicative<graded_c>.invoke(
    add, std::declval<const graded_c &>(), std::declval<const graded_i &>()));

static_assert(std::is_same_v<bt::grade_of_t<graded_mix>,
                             bt::grade_join_t<bt::grade_of_t<graded_c>,
                                              bt::grade_of_t<graded_i>>>,
              "The expected mixing point computes the result grade through "
              "grade_join_t.");

static_assert(std::is_same_v<bt::grade_of_t<bare_c>, e_c>);
static_assert(std::is_same_v<bt::grade_of_t<bare_i>, e_i>);

using bare_mix = decltype(bt::applicative<bare_c>.invoke(
    add, std::declval<const bare_c &>(), std::declval<const bare_i &>()));

static_assert(std::is_same_v<bare_mix, std::expected<int, e_ci>>);
static_assert(bt::graded_context<bare_mix>);

using framework_prediction = bt::rebind_grade_t<
    int, bt::grade_join_t<bt::grade_of_t<bare_c>, bt::grade_of_t<bare_i>>>;

static_assert(
    std::is_same_v<bare_mix, framework_prediction>,
    "Plain-error expected carriers are semantically graded at singleton "
    "model grades, so grade_of plus grade_join_t predicts the mixed result.");

using boolean_mix = decltype(bt::applicative<boolean_may>.invoke(
    add, std::declval<const boolean_may &>(),
    std::declval<const boolean_never &>()));

static_assert(std::is_same_v<boolean_mix, Fallible<int, may_fail>>);
static_assert(
    std::is_same_v<bt::grade_of_t<boolean_mix>,
                   bt::grade_join_t<bt::grade_of_t<boolean_may>,
                                    bt::grade_of_t<boolean_never>>>,
    "The Boolean model drives a real mixed deduction through grade_join_t.");

template <class LEFT, class RIGHT>
concept accepts_boolean_invoke = requires(const LEFT &lhs, const RIGHT &rhs) {
    bt::applicative<boolean_may>.invoke(add, lhs, rhs);
};

static_assert(accepts_boolean_invoke<boolean_may, boolean_never>);
static_assert(!accepts_boolean_invoke<boolean_may, graded_c>);

constexpr auto
accepts_expected_value_and_foreign_carrier(const int &expected_value,
                                           const boolean_may &) -> int {
    return expected_value;
}

template <class LEFT, class RIGHT>
concept accepts_expected_invoke = requires(const LEFT &lhs, const RIGHT &rhs) {
    bt::applicative<bare_c>.invoke(accepts_expected_value_and_foreign_carrier,
                                   lhs, rhs);
};

template <class LEFT, class RIGHT>
concept accepts_accumulating_expected_invoke =
    requires(const LEFT &lhs, const RIGHT &rhs) {
        bt::accumulating_applicative<bare_c>.invoke(
            accepts_expected_value_and_foreign_carrier, lhs, rhs);
    };

static_assert(accepts_expected_invoke<bare_c, int>);
static_assert(!accepts_expected_invoke<bare_c, boolean_may>);
static_assert(accepts_accumulating_expected_invoke<bare_c, int>);
static_assert(!accepts_accumulating_expected_invoke<bare_c, boolean_may>);

} // namespace mixing_point

// =========================================================================
// FUNCTOR-MONAD GROUNDING: the layered instance, its laws, and the
// agreement between a hand-written Functor and the fmap Monad grows.
//
// std::optional carries both a hand-written functor and a Monad
// instance; the latter grows fmap structurally, from bind + pure, and the
// full Functor instance is spelled derive_functor<OptionalMonadMap<int>>{}
// rather than registered anywhere. This section checks that layered instance
// compiles, satisfies the functor laws, and agrees with the hand-written
// one -- see docs/transpose/decisions.md#functor-monad-grounding.
// =========================================================================

namespace functor_monad_grounding {

constexpr bt::derive_functor<bt::OptionalMonadMap<int>> layered_functor{};

constexpr auto increment(int x) -> int { return x + 1; }
constexpr auto double_it(int x) -> int { return x * 2; }
constexpr auto identity(int x) -> int { return x; }

} // namespace functor_monad_grounding

TEST_CASE("laws: functor identity holds for derive_functor<OptionalMonadMap>") {
    using namespace functor_monad_grounding;
    REQUIRE(layered_functor.fmap(identity, std::optional<int>{5}) ==
            std::optional<int>{5});
    REQUIRE(layered_functor.fmap(identity, std::optional<int>{}) ==
            std::optional<int>{});
}

TEST_CASE(
    "laws: functor composition holds for derive_functor<OptionalMonadMap>") {
    using namespace functor_monad_grounding;
    auto composed = [](int x) { return double_it(increment(x)); };

    REQUIRE(
        layered_functor.fmap(composed, std::optional<int>{5}) ==
        layered_functor.fmap(
            double_it, layered_functor.fmap(increment, std::optional<int>{5})));
    REQUIRE(
        layered_functor.fmap(composed, std::optional<int>{}) ==
        layered_functor.fmap(
            double_it, layered_functor.fmap(increment, std::optional<int>{})));
}

TEST_CASE("laws: hand-written Functor and Monad-grown fmap agree on optional") {
    using functor_monad_grounding::increment;

    const auto &functor = bt::functor<std::optional<int>>;
    const auto &monad = bt::monad<std::optional<int>>;
    auto via_bind = [](std::optional<int> ma) {
        return bt::mbind(
            ma, [](int a) { return std::optional<int>{increment(a)}; });
    };

    for (auto ma : {std::optional<int>{5}, std::optional<int>{}}) {
        REQUIRE(functor.fmap(increment, ma) == monad.fmap(increment, ma));
        REQUIRE(functor.fmap(increment, ma) == via_bind(ma));
    }
}

TEST_CASE("laws: replace reaches fmap through the layered instance") {
    using namespace functor_monad_grounding;
    REQUIRE(layered_functor.replace(std::optional<int>{1}, 9) ==
            std::optional<int>{9});
}

TEST_CASE("laws: the shipped model satisfies the graded laws") {
    // Compile-time only; reaching here means the static_assert above held.
    SUCCEED("error_set is a bounded join-semilattice with coherent carriers");
}

TEST_CASE("laws: a second, unrelated model satisfies the same laws") {
    SUCCEED("the Boolean semilattice passes the harness written against the "
            "concept");
}

// =========================================================================
// INSTANCE LAWS: every registered Applicative and Monad object, against the
// Functor, Applicative and Monad laws in laws.hpp.
//
// This generalizes the one-off agreement test above: where that checked
// that the hand-written Functor and the Monad-grown fmap agree on
// std::optional<int>, the harness checks the laws themselves, over every
// instance, and the agreement as one law among them. See
// docs/transpose/decisions.md#functor-monad-grounding.
//
// COMPILE TIME WHERE THE INSTANCE ALLOWS IT. The optional instance and the
// test Identity instance are constant-evaluable, so their laws are
// static_asserts. The expected, array and zip_list instances are not: their
// basis operations (`pure`, `invoke`, `bind`) are not declared constexpr, so
// the same per-law checkers run under REQUIRE instead. The checkers are the
// same functions either way; only the evaluation time differs.
// =========================================================================

namespace instance_laws {

using laws::trace;

/** std::optional<trace>: two engaged values and the empty one. */
struct optional_sample {
    using context = std::optional<trace>;

    static constexpr auto contexts() {
        return std::array{context{trace{"a"}}, context{trace{"b"}}, context{}};
    }

    /** The arrow that fails, which no `pure . f` can spell. */
    static constexpr auto arrows() {
        return std::tuple{[](const trace &) { return context{}; }};
    }
};

/** A named error type for the expected sample: two alternatives, so that
 * which failing operand is observed -- leftmost, by
 * docs/transpose/decisions.md#applicative-objects -- is visible in the
 * result rather than collapsing to one value. */
enum class fault { first, second };

/** std::expected<trace, fault>: two values and both failures. */
struct expected_sample {
    using context = std::expected<trace, fault>;

    static constexpr auto contexts() {
        return std::array{context{trace{"a"}}, context{trace{"b"}},
                          context{std::unexpect, fault::first},
                          context{std::unexpect, fault::second}};
    }

    static constexpr auto arrows() {
        return std::tuple{[](const trace &) {
            return context{std::unexpect, fault::second};
        }};
    }
};

/** std::array<trace, 2>: positional application over two lanes. */
struct array_sample {
    using context = std::array<trace, 2>;

    static constexpr auto contexts() {
        return std::array{context{trace{"a"}, trace{"b"}},
                          context{trace{"c"}, trace{"d"}}};
    }
};

/** zip_list<trace>: two finite lists of different lengths, so truncation is
 * exercised, and the repeating list that `pure` produces. */
struct zip_list_sample {
    using context = bt::zip_list<trace>;

    static auto contexts() {
        return std::array{context{{trace{"a"}, trace{"b"}}},
                          context{{trace{"c"}}}, context::repeat(trace{"r"})};
    }
};

/** The test Identity: one value, nothing else to it. */
struct identity_sample {
    using context = bt::test::Identity<trace>;

    static constexpr auto contexts() { return std::array{context{trace{"a"}}}; }
};

// The samples really are samples.
static_assert(laws::instance_sample<optional_sample>);
static_assert(laws::instance_sample<expected_sample>);
static_assert(laws::instance_sample<array_sample>);
static_assert(laws::instance_sample<zip_list_sample>);
static_assert(laws::instance_sample<identity_sample>);

// -- std::optional: everything at compile time ----------------------------

using optional_context = optional_sample::context;

static_assert(
    laws::check_functor_laws<bt::functor<optional_context>, optional_sample>());
static_assert(
    laws::check_functor_laws<bt::monad<optional_context>, optional_sample>());
static_assert(laws::check_applicative_laws<bt::applicative<optional_context>,
                                           optional_sample>());
static_assert(laws::check_applicative_laws<bt::monad<optional_context>,
                                           optional_sample>());
static_assert(
    laws::check_monad_laws<bt::monad<optional_context>, optional_sample>());

// The layered instance -- the full Functor spelled over the Monad object --
// satisfies the functor laws too, at compile time.
constexpr bt::derive_functor<bt::OptionalMonadMap<trace>>
    layered_trace_functor{};

static_assert(
    laws::check_functor_laws<layered_trace_functor, optional_sample>());

// The agreement the grounding decision records, as laws: hand-written
// Functor, Monad-grown fmap, and the layered instance each agree with the
// Applicative's unary invoke, and the Monad's synthesized invoke agrees with
// the Applicative's native one.
static_assert(laws::fmap_agrees_with_invoke<bt::functor<optional_context>,
                                            bt::applicative<optional_context>,
                                            optional_sample>(),
              "optional: functor.fmap must equal applicative.invoke");
static_assert(laws::fmap_agrees_with_invoke<bt::monad<optional_context>,
                                            bt::applicative<optional_context>,
                                            optional_sample>(),
              "optional: monad.fmap must equal applicative.invoke");
static_assert(laws::fmap_agrees_with_invoke<layered_trace_functor,
                                            bt::applicative<optional_context>,
                                            optional_sample>(),
              "optional: the layered functor's fmap must equal "
              "applicative.invoke");
static_assert(
    laws::invoke_agrees<bt::monad<optional_context>,
                        bt::applicative<optional_context>, optional_sample>(),
    "optional: monad.invoke must equal applicative.invoke");

// -- The test Identity: at compile time -----------------------------------

using identity_context = identity_sample::context;

static_assert(laws::check_applicative_laws<bt::applicative<identity_context>,
                                           identity_sample>());

// -- Runtime: the same checkers, for instances that are not constexpr -----

/** Every applicative law, one REQUIRE each, so a failure names the law. */
template <const auto &OBJECT, laws::instance_sample SAMPLE>
void require_applicative_laws() {
    REQUIRE(laws::applicative_identity<OBJECT, SAMPLE>());
    REQUIRE(laws::applicative_homomorphism<OBJECT, SAMPLE>());
    REQUIRE(laws::applicative_interchange<OBJECT, SAMPLE>());
    REQUIRE(laws::applicative_interchange_nary<OBJECT, SAMPLE>());
    REQUIRE(laws::applicative_composition<OBJECT, SAMPLE>());
    REQUIRE(laws::applicative_composition_nary<OBJECT, SAMPLE>());
    REQUIRE(laws::ap_agrees_with_invoke<OBJECT, SAMPLE>());
    REQUIRE(laws::derived_applicative_agreement<OBJECT, SAMPLE>());
}

/** Every monad law, one REQUIRE each. */
template <const auto &OBJECT, laws::instance_sample SAMPLE>
void require_monad_laws() {
    REQUIRE(laws::functor_identity<OBJECT, SAMPLE>());
    REQUIRE(laws::functor_composition<OBJECT, SAMPLE>());
    REQUIRE(laws::monad_left_identity<OBJECT, SAMPLE>());
    REQUIRE(laws::monad_right_identity<OBJECT, SAMPLE>());
    REQUIRE(laws::monad_associativity<OBJECT, SAMPLE>());
    REQUIRE(laws::monad_fmap_grounding<OBJECT, SAMPLE>());
    REQUIRE(laws::monad_join_coherence<OBJECT, SAMPLE>());
    REQUIRE(laws::monad_invoke_coherence<OBJECT, SAMPLE>());
}

using expected_context = expected_sample::context;
using array_context = array_sample::context;
using zip_list_context = zip_list_sample::context;

} // namespace instance_laws

TEST_CASE("laws: std::expected's short-circuit object satisfies the "
          "applicative and monad laws") {
    using namespace instance_laws;
    require_applicative_laws<bt::applicative<expected_context>,
                             expected_sample>();
    require_applicative_laws<bt::monad<expected_context>, expected_sample>();
    require_monad_laws<bt::monad<expected_context>, expected_sample>();
    REQUIRE(laws::fmap_agrees_with_invoke<bt::monad<expected_context>,
                                          bt::applicative<expected_context>,
                                          expected_sample>());
    REQUIRE(laws::invoke_agrees<bt::monad<expected_context>,
                                bt::applicative<expected_context>,
                                expected_sample>());
}

TEST_CASE("laws: std::expected's accumulating object satisfies the "
          "applicative laws") {
    // No monad laws: the accumulating object has no bind, by
    // docs/transpose/decisions.md#applicative-objects.
    using namespace instance_laws;
    require_applicative_laws<bt::accumulating_applicative<expected_context>,
                             expected_sample>();
}

TEST_CASE("laws: std::array satisfies the applicative laws") {
    using namespace instance_laws;
    require_applicative_laws<bt::applicative<array_context>, array_sample>();
}

TEST_CASE("laws: zip_list satisfies the applicative laws") {
    using namespace instance_laws;
    require_applicative_laws<bt::applicative<zip_list_context>,
                             zip_list_sample>();
}

TEST_CASE("laws: the constexpr instances satisfy the instance laws") {
    // Compile-time only; reaching here means the static_asserts above held.
    SUCCEED("optional (functor, applicative, monad, layered functor) and the "
            "test Identity pass the instance laws in a constant expression");
}

// =========================================================================
// THE LAWLESS INSTANCE: the acceptance test for the harness itself.
//
// An instance whose `pure` leaves a mark fails the homomorphism law --
// invoke(f, pure(x)) computes x + "pure" + "f", pure(f(x)) computes
// x + "f" + "pure" -- and the diagnostic must NAME that law. Note that the
// identity law still holds for it, and that over integer addition the two
// sides would be equal: the order-sensitive algebra is what catches it.
//
// There is no compile-fail test mechanism in this repository, so the
// demonstration is behind a macro. Enabling it, with
//
//     -DBEMAN_TRANSPOSE_LAWS_DEMONSTRATE_LAWLESS
//
// fails the build of this translation unit with exactly these three errors
// (GCC 15.2; paths and line numbers trimmed):
//
//   laws.hpp: In instantiation of 'consteval bool
//   beman::transpose::test::laws::check_applicative_laws() [with const
//   auto& OBJECT = beman::transpose::applicative<lawless_demo::Lawless<
//   trace> >; SAMPLE = lawless_demo::lawless_sample]':
//   laws.test.cpp: required from here
//   laws.hpp: error: static assertion failed: applicative homomorphism:
//   invoke(f, pure(x)) must equal pure(f(x)), and
//   invoke(h, pure(x), pure(y)) must equal pure(h(x, y)).
//   laws.hpp: error: static assertion failed: applicative interchange:
//   invoke(evaluate, u, pure(y)) must equal invoke(g -> g(y), u) for a
//   context u of callables.
//   laws.hpp: error: static assertion failed: applicative interchange
//   (n-ary): a pure operand floats out, invoke(h, u, pure(y)) must equal
//   invoke(a -> h(a, y), u), and symmetrically on the left.
//
// Each law `pure` breaks is named by its own message; the laws it does not
// break (identity, both compositions, the coherence checks) are silent; and
// nothing collapses to a bare `static_assert(false)`.
// =========================================================================

#if defined(BEMAN_TRANSPOSE_LAWS_DEMONSTRATE_LAWLESS)

namespace lawless_demo {

/** A single-element context, like the test Identity. */
template <class VALUE_TYPE>
struct Lawless {
    using value_type = VALUE_TYPE;

    VALUE_TYPE value;

    friend constexpr auto operator==(const Lawless &, const Lawless &)
        -> bool = default;
};

struct lawless_sample {
    using context = Lawless<laws::trace>;

    static constexpr auto contexts() {
        return std::array{context{laws::trace{"a"}}};
    }
};

} // namespace lawless_demo

namespace beman::transpose {

template <class VALUE_TYPE>
struct LawlessApplicativeImpl {
    /** The violation: `pure` is not a plain embedding. */
    constexpr auto pure(this auto &&, const test::laws::trace &value)
        -> lawless_demo::Lawless<test::laws::trace> {
        return {value + test::laws::trace{"pure"}};
    }

    template <class FUNCTION, class FIRST, class... REST>
    constexpr auto invoke(this auto &&, FUNCTION &&function,
                          const lawless_demo::Lawless<FIRST> &first,
                          const lawless_demo::Lawless<REST> &...rest)
        -> lawless_demo::Lawless<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        return {std::invoke(function, first.value, rest.value...)};
    }
};

template <class VALUE_TYPE>
struct LawlessApplicativeMap
    : derive_applicative<LawlessApplicativeImpl<VALUE_TYPE>> {
    using LawlessApplicativeImpl<VALUE_TYPE>::invoke;
    using LawlessApplicativeImpl<VALUE_TYPE>::pure;
};

template <class VALUE_TYPE>
inline constexpr auto applicative<lawless_demo::Lawless<VALUE_TYPE>> =
    LawlessApplicativeMap<VALUE_TYPE>{};

} // namespace beman::transpose

static_assert(laws::check_applicative_laws<
              bt::applicative<lawless_demo::lawless_sample::context>,
              lawless_demo::lawless_sample>());

#endif // BEMAN_TRANSPOSE_LAWS_DEMONSTRATE_LAWLESS

// =========================================================================
// CONTEXT CROSSINGS. docs/transpose/decisions.md#licensed-context-crossings.
//
// cross-model-mixing forbids combining graded operands of different models at
// a mixing point, and stands. A CROSSING is a different operation: a
// deliberate, named move of one value from one carrier into another. The rule
// is about information, not lattices: a crossing is licensed when it loses
// nothing, which is the case exactly when the source is at its model's
// bottom, or on the failure path through a handler that supplies what the
// target needs or consumes what the source has. Success values pass through
// every licensed crossing unchanged.
//
// TEST-ONLY DEMONSTRATION. No operation ships; the functions below are what a
// future `into<C>` would have to satisfy, written against the Boolean model's
// Fallible rather than std::optional, so that optional-grade-model stays its
// own question (the stage's tripwire).
// =========================================================================

namespace context_crossing {

/** A handler that is callable with nothing: a crossing that asks for one has
 * no lossless form. */
struct uncallable {};

// -- Boolean model -> error-set model ----------------------------------------

/** ⊤ -> error set: the empty state carries no error, so the crossing is
 * licensed only through a handler that supplies one. `E` is named: the
 * target's grade is the crossing's to state, never inferred from the
 * source, which has nothing to infer it from. */
template <class E, class T, class SUPPLY>
    requires std::invocable<SUPPLY &> &&
             std::convertible_to<std::invoke_result_t<SUPPLY &>, E>
constexpr auto into_expected(const Fallible<T, may_fail> &source,
                             SUPPLY &&supply)
    -> std::expected<T, bt::error_set<E>> {
    if (source.has_value()) {
        return std::expected<T, bt::error_set<E>>{std::in_place, *source};
    }
    return std::expected<T, bt::error_set<E>>{
        std::unexpect, bt::error_set<E>{static_cast<E>(std::invoke(supply))}};
}

/** ⊥ -> error set: the source cannot be empty, so nothing is lost and no
 * handler is asked for. The crossing is the success path alone. */
template <class E, class T>
constexpr auto into_expected(const Fallible<T, never_fails> &source)
    -> std::expected<T, bt::error_set<E>> {
    return std::expected<T, bt::error_set<E>>{std::in_place, *source};
}

// -- error-set model -> Boolean model ----------------------------------------

/** error set -> ⊤: the error would be dropped, so the crossing is licensed
 * only through a handler that consumes it. The handler returns nothing: the
 * target's failure state has nothing to be supplied. */
template <class T, class... ERRORS, class CONSUME>
    requires std::invocable<CONSUME &, const bt::error_set_of<ERRORS...> &>
constexpr auto
into_fallible(const std::expected<T, bt::error_set_of<ERRORS...>> &source,
              CONSUME &&consume) -> Fallible<T, may_fail> {
    if (source.has_value()) {
        return Fallible<T, may_fail>{*source};
    }
    std::invoke(consume, source.error());
    return Fallible<T, may_fail>{};
}

/** The uniform spelling of the empty grade -> ⊥: both are the identity
 * carrier of their model, so the crossing is lossless in both directions
 * and asks for nothing. */
template <class T>
constexpr auto into_fallible(const std::expected<T, bt::error_set_of<>> &source)
    -> Fallible<T, boolean_grade::never_fails> {
    return Fallible<T, boolean_grade::never_fails>{*source};
}

/** Whether a crossing from SOURCE into the other model exists with no
 * handler: true exactly when nothing can be lost. */
template <class SOURCE>
concept crosses_without_handler = requires(const SOURCE &source) {
    into_fallible(source);
} || requires(const SOURCE &source) { into_expected<int>(source); };

using exp_c = std::expected<int, e_c>;
using exp_empty = std::expected<int, bt::error_set<>>;

// Lossless crossings need no handler ...
static_assert(
    crosses_without_handler<Fallible<int, boolean_grade::never_fails>>);
static_assert(crosses_without_handler<exp_empty>);
// ... and lossy ones are refused without one, in either direction.
static_assert(!crosses_without_handler<Fallible<int, may_fail>>);
static_assert(!crosses_without_handler<exp_c>);

// The types a licensed crossing produces, pinned.
static_assert(
    std::is_same_v<decltype(into_expected<std::errc>(
                       std::declval<const Fallible<int, may_fail> &>(),
                       std::declval<std::errc (&)()>())),
                   exp_c>);
static_assert(
    std::is_same_v<
        decltype(into_expected<std::errc>(
            std::declval<const Fallible<int, boolean_grade::never_fails> &>())),
        exp_c>);
static_assert(std::is_same_v<
              decltype(into_fallible(std::declval<const exp_c &>(),
                                     std::declval<void (&)(const e_c &)>())),
              Fallible<int, may_fail>>);

} // namespace context_crossing

TEST_CASE("crossings: an empty Boolean value enters the error-set model only "
          "through a handler that supplies the error") {
    using context_crossing::into_expected;
    Fallible<int, may_fail> present{7};
    Fallible<int, may_fail> empty{};
    int supplied = 0;
    auto supply = [&supplied] {
        ++supplied;
        return std::errc::invalid_argument;
    };

    auto crossed = into_expected<std::errc>(present, supply);
    REQUIRE(crossed == context_crossing::exp_c{7});
    REQUIRE(supplied == 0); // the success path asks the handler for nothing

    auto failed = into_expected<std::errc>(empty, supply);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(failed.error().holds<std::errc>());
    REQUIRE(supplied == 1);
}

TEST_CASE("crossings: a value that cannot fail enters any fallible context "
          "with no handler, and the identity carriers cross both ways") {
    using context_crossing::into_expected;
    using context_crossing::into_fallible;
    Fallible<int, boolean_grade::never_fails> certain{3};
    auto crossed = into_expected<std::errc>(certain);
    REQUIRE(crossed == context_crossing::exp_c{3});

    auto back = into_fallible(bt::fallible(9));
    STATIC_REQUIRE(std::is_same_v<decltype(back),
                                  Fallible<int, boolean_grade::never_fails>>);
    REQUIRE(*back == 9);
}

TEST_CASE("crossings: an error-set value enters the Boolean model only "
          "through a handler that consumes the error") {
    using context_crossing::into_fallible;
    int consumed = 0;
    auto consume = [&consumed](const e_c &) { ++consumed; };

    auto crossed = into_fallible(context_crossing::exp_c{5}, consume);
    REQUIRE(crossed == Fallible<int, may_fail>{5});
    REQUIRE(consumed == 0);

    auto failed = into_fallible(
        context_crossing::exp_c{std::unexpect, std::errc::invalid_argument},
        consume);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(consumed == 1);
}
