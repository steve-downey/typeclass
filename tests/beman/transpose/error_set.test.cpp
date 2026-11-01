// tests/beman/transpose/error_set.test.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/error_set.hpp>

#include <beman/transpose/overloaded.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

// Named types with external linkage and a stable spelling -- the documented
// restrictions of the interim P2830 fallback. NOT in an anonymous namespace,
// deliberately: these probes must satisfy the restriction they are testing.
//
// The err_ prefix is load-bearing, not decoration. These names are hoisted into
// the global namespace by the using-declarations below, and a bare `gamma`
// collides there with the legacy XSI `::gamma(double)` that glibc's <math.h>
// declares. A class name and a function name may coexist in one scope, with the
// class hidden ([basic.scope.scope]), so `gamma` in a template argument list
// names the function and fails as "must be a type" -- but only under a standard
// library whose headers happen to drag in <math.h>, which libc++ does and
// libstdc++ does not. Keep the prefix; do not reintroduce bare Greek letters.
namespace beman_transpose_error_probes {

struct err_alpha {
    int value{};

    friend auto operator==(const err_alpha &, const err_alpha &)
        -> bool = default;
};
struct err_beta {
    int value{};

    friend auto operator==(const err_beta &, const err_beta &)
        -> bool = default;
};
struct err_gamma {
    int value{};

    friend auto operator==(const err_gamma &, const err_gamma &)
        -> bool = default;
};

} // namespace beman_transpose_error_probes

namespace probes = beman_transpose_error_probes;

using probes::err_alpha;
using probes::err_beta;
using probes::err_gamma;

using empty_set = bt::error_set<>;
using set_a = bt::error_set<err_alpha>;
using set_b = bt::error_set<err_beta>;
using set_ab = bt::error_set<err_alpha, err_beta>;
using set_abc = bt::error_set<err_alpha, err_beta, err_gamma>;

// =========================================================================
// Normalization. The load-bearing property: order of spelling is not part
// of the identity, so grade arithmetic is well-defined and inclusions are
// unique.
// =========================================================================

static_assert(std::is_same_v<bt::error_set<err_alpha, err_beta>,
                             bt::error_set<err_beta, err_alpha>>);
static_assert(std::is_same_v<bt::error_set<err_alpha, err_beta, err_gamma>,
                             bt::error_set<err_gamma, err_alpha, err_beta>>);
static_assert(std::is_same_v<bt::error_set<err_gamma, err_beta, err_alpha>,
                             bt::error_set<err_alpha, err_gamma, err_beta>>);

// Duplicates collapse, in any position and any multiplicity.
static_assert(std::is_same_v<bt::error_set<err_alpha, err_alpha>, set_a>);
static_assert(
    std::is_same_v<bt::error_set<err_alpha, err_beta, err_alpha>, set_ab>);
static_assert(std::is_same_v<
              bt::error_set<err_beta, err_alpha, err_beta, err_alpha>, set_ab>);

// Distinct sets stay distinct.
static_assert(!std::is_same_v<set_a, set_b>);
static_assert(!std::is_same_v<set_a, set_ab>);
static_assert(!std::is_same_v<empty_set, set_a>);

// =========================================================================
// Semilattice laws, on hand-picked sets over three generators. Eight
// elements is the free semilattice on three, so every law shape is
// exercised by concrete instances.
// =========================================================================

// Unit: bottom is the identity of join.
static_assert(std::is_same_v<bt::error_set_join_t<set_a, empty_set>, set_a>);
static_assert(std::is_same_v<bt::error_set_join_t<empty_set, set_a>, set_a>);
static_assert(
    std::is_same_v<bt::error_set_join_t<empty_set, empty_set>, empty_set>);
static_assert(std::is_same_v<bt::error_set_bottom, empty_set>);

// Idempotence -- the property that keeps fold grades shape-independent.
static_assert(std::is_same_v<bt::error_set_join_t<set_a, set_a>, set_a>);
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, set_ab>, set_ab>);

// Commutativity -- the property that makes grade arithmetic order-free.
static_assert(std::is_same_v<bt::error_set_join_t<set_a, set_b>,
                             bt::error_set_join_t<set_b, set_a>>);
static_assert(std::is_same_v<bt::error_set_join_t<set_a, set_b>, set_ab>);

// Associativity.
static_assert(
    std::is_same_v<
        bt::error_set_join_t<bt::error_set_join_t<set_a, set_b>,
                             bt::error_set<err_gamma>>,
        bt::error_set_join_t<
            set_a, bt::error_set_join_t<set_b, bt::error_set<err_gamma>>>>);
static_assert(std::is_same_v<
              bt::error_set_join_t<set_ab, bt::error_set<err_gamma>>, set_abc>);

// Overlapping joins do not double-count.
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, set_a>, set_ab>);
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, set_abc>, set_abc>);

// =========================================================================
// Order from join: e ⊆ f  ⟺  e ∪ f = f. Subsumption is not an independent
// relation, it is read off the join -- which is what makes the coercions
// canonical and therefore coherently implicit.
// =========================================================================

template <class LEFT, class RIGHT>
inline constexpr bool order_agrees_with_join =
    bt::error_set_subsumes_v<LEFT, RIGHT> ==
    std::is_same_v<bt::error_set_join_t<LEFT, RIGHT>, RIGHT>;

static_assert(order_agrees_with_join<empty_set, set_a>);
static_assert(order_agrees_with_join<set_a, set_ab>);
static_assert(order_agrees_with_join<set_ab, set_a>);
static_assert(order_agrees_with_join<set_a, set_b>);
static_assert(order_agrees_with_join<set_ab, set_abc>);
static_assert(order_agrees_with_join<set_abc, set_ab>);
static_assert(order_agrees_with_join<set_a, set_a>);

// Spelled out, so a broken join and a broken order cannot agree vacuously.
static_assert(bt::error_set_subsumes_v<set_a, set_ab>);
static_assert(bt::error_set_subsumes_v<empty_set, set_abc>);
static_assert(bt::error_set_subsumes_v<set_a, set_a>);
static_assert(!bt::error_set_subsumes_v<set_ab, set_a>);
static_assert(!bt::error_set_subsumes_v<set_a, set_b>);

// =========================================================================
// Conversions: exactly the ⊆ widenings, and nothing else. A non-subset
// conversion compiling is a stage tripwire, so both directions are pinned.
// =========================================================================

template <class FROM, class TO>
concept converts_to = std::is_convertible_v<FROM, TO>;

// Widening along ⊆ is available.
static_assert(converts_to<set_a, set_ab>);
static_assert(converts_to<set_b, set_ab>);
static_assert(converts_to<set_a, set_abc>);
static_assert(converts_to<set_ab, set_abc>);

// Narrowing is not.
static_assert(!converts_to<set_ab, set_a>);
static_assert(!converts_to<set_abc, set_ab>);

// Incomparable sets do not convert in either direction.
static_assert(!converts_to<set_a, set_b>);
static_assert(!converts_to<set_b, set_a>);

// Injection: an error value becomes a set containing it.
static_assert(converts_to<err_alpha, set_a>);
static_assert(converts_to<err_alpha, set_ab>);
static_assert(converts_to<err_beta, set_ab>);

// A non-member does not inject.
static_assert(!converts_to<err_gamma, set_ab>);

// The empty set is uninhabited: nothing converts into it, and it cannot be
// default-constructed.
static_assert(!converts_to<err_alpha, empty_set>);
static_assert(!std::is_default_constructible_v<empty_set>);

// Membership is type-level and answers for non-members too.
static_assert(set_ab::contains<err_alpha>());
static_assert(set_ab::contains<err_beta>());
static_assert(!set_ab::contains<err_gamma>());
static_assert(!empty_set::contains<err_alpha>());

// =========================================================================
// Runtime surface: injection, membership of the held alternative, and
// visitation. This is the whole data-facing API on purpose.
// =========================================================================

TEST_CASE("error_set: injection and the held alternative") {
    set_ab held{err_alpha{7}};

    REQUIRE(held.holds<err_alpha>());
    REQUIRE_FALSE(held.holds<err_beta>());
}

TEST_CASE("error_set: visitation reaches the held alternative") {
    set_ab from_alpha{err_alpha{3}};
    set_ab from_beta{err_beta{4}};

    auto tag = [](const auto &error) {
        using held = std::remove_cvref_t<decltype(error)>;
        if constexpr (std::is_same_v<held, err_alpha>) {
            return error.value * 10;
        } else {
            return error.value * 100;
        }
    };

    REQUIRE(from_alpha.visit(tag) == 30);
    REQUIRE(from_beta.visit(tag) == 400);
}

TEST_CASE("error_set: widening preserves the held alternative") {
    set_a narrow{err_alpha{5}};
    set_abc wide{narrow};

    REQUIRE(wide.holds<err_alpha>());
    REQUIRE_FALSE(wide.holds<err_beta>());
    REQUIRE(wide.visit([](const auto &error) { return error.value; }) == 5);
}

TEST_CASE("error_set: widening is available through a join") {
    using joined = bt::error_set_join_t<set_a, set_b>;
    static_assert(std::is_same_v<joined, set_ab>);

    joined from_left{err_alpha{1}};
    joined from_right{err_beta{2}};

    REQUIRE(from_left.holds<err_alpha>());
    REQUIRE(from_right.holds<err_beta>());
}

TEST_CASE("error_set: laws hold in a consteval context") {
    // The static_asserts above already ran at compile time; this case makes
    // the coverage visible to ctest rather than silently passing by being a
    // translation unit that merely compiled.
    SUCCEED("normalization, semilattice laws, and conversion fences compiled");
}

// =========================================================================
// Value-level amendment (docs/transpose/decisions.md#accumulation-evidence): a
// value is a non-empty WITNESSED SUBSET, not a one-of union. The tests above
// already cover the singleton case -- every one of them holds exactly one
// witness, which is the short-circuit special case of the general
// invariant. These tests exercise the general case: more than one witness
// present at once, and the left-biased combine that produces it.
// =========================================================================

TEST_CASE("error_set: combining distinct types unions their witnesses") {
    set_ab from_alpha{err_alpha{1}};
    set_ab from_beta{err_beta{2}};

    auto combined = bt::error_set_combine(from_alpha, from_beta);

    REQUIRE(combined.holds<err_alpha>());
    REQUIRE(combined.holds<err_beta>());
    REQUIRE(combined.witness<err_alpha>() == std::optional{err_alpha{1}});
    REQUIRE(combined.witness<err_beta>() == std::optional{err_beta{2}});
}

TEST_CASE("error_set: combining the same type keeps the LEFT witness") {
    set_a left{err_alpha{10}};
    set_a right{err_alpha{20}};

    auto combined = bt::error_set_combine(left, right);

    REQUIRE(combined.holds<err_alpha>());
    REQUIRE(combined.witness<err_alpha>() == std::optional{err_alpha{10}});

    // Combine is not commutative on the WITNESS (only the type-level join
    // is): swapping the operands changes which witness survives.
    auto swapped = bt::error_set_combine(right, left);
    REQUIRE(swapped.witness<err_alpha>() == std::optional{err_alpha{20}});
}

TEST_CASE("error_set: combining is left-biased per type, not wholesale") {
    // Left holds err_alpha only, right holds both err_alpha and err_beta. The
    // combined result keeps left's err_alpha (left-biased) but still picks up
    // right's err_beta, since left never witnessed err_beta at all.
    set_ab left{err_alpha{100}};
    set_ab right{err_beta{200}};
    // Give `right` a err_beta witness and, via a second combine, an err_alpha
    // witness too, to show a per-type -- not per-value -- decision.
    auto right_both = bt::error_set_combine(right, set_ab{err_alpha{999}});
    REQUIRE(right_both.holds<err_alpha>());
    REQUIRE(right_both.holds<err_beta>());

    auto combined = bt::error_set_combine(left, right_both);
    REQUIRE(combined.witness<err_alpha>() ==
            std::optional{err_alpha{100}}); // left's
    REQUIRE(combined.witness<err_beta>() ==
            std::optional{err_beta{200}}); // right's
}

TEST_CASE("error_set: equality is same present set, equal witnesses") {
    set_ab both_a_b{err_alpha{1}};
    auto with_beta_too = bt::error_set_combine(both_a_b, set_ab{err_beta{2}});

    // Different present set: unequal even though err_alpha's witness agrees.
    REQUIRE_FALSE(both_a_b == with_beta_too);

    // Same present set, same witness values: equal.
    REQUIRE(with_beta_too ==
            bt::error_set_combine(set_ab{err_alpha{1}}, set_ab{err_beta{2}}));

    // Same present set, DIFFERENT witness value: unequal. This is the
    // "equal witnesses" half of the amendment -- presence alone is not
    // enough.
    auto different_witness =
        bt::error_set_combine(set_ab{err_alpha{1}}, set_ab{err_beta{99}});
    REQUIRE_FALSE(with_beta_too == different_witness);
}

TEST_CASE("error_set: combine is associative, allocation-free evidence") {
    // Associativity is what makes combine well-defined as a fold over any
    // number of failures regardless of grouping, exactly like the type-level
    // join it sits beside.
    set_abc a{err_alpha{1}};
    set_abc b{err_beta{2}};
    set_abc c{err_gamma{3}};

    auto left_first = bt::error_set_combine(bt::error_set_combine(a, b), c);
    auto right_first = bt::error_set_combine(a, bt::error_set_combine(b, c));

    REQUIRE(left_first == right_first);
    REQUIRE(left_first.holds<err_alpha>());
    REQUIRE(left_first.holds<err_beta>());
    REQUIRE(left_first.holds<err_gamma>());
}

// =========================================================================
// visit's precondition is checked, not merely documented.
//
// Multi-witness values and visitation shipped one stage apart
// (accumulating-object and error-set-type respectively), and the seam
// between them is where a silent wrong answer would have lived: visit
// picking the leftmost witness and dropping the rest would surface as a
// lost error somewhere else entirely, much later.
// =========================================================================

/** visit still works in a CONSTANT EXPRESSION when the precondition holds.
 * This is the assertion that matters after the hardening: the check added to
 * to_variant() calls a non-constexpr function, and if that call were reachable
 * on the valid path it would have quietly cost every constexpr use of visit. */
consteval auto visit_a_singleton_at_compile_time() -> int {
    const set_ab held{err_alpha{7}};
    return held.visit([](const auto &error) { return error.value; });
}

static_assert(visit_a_singleton_at_compile_time() == 7);

// witness_count lets a caller check the precondition instead of tripping it.
consteval auto count_after_combining() -> std::size_t {
    const set_ab left{err_alpha{1}};
    const set_ab right{err_beta{2}};
    return left.combined_with(right).witness_count();
}

static_assert(count_after_combining() == 2);

consteval auto count_of_a_singleton() -> std::size_t {
    return set_ab{err_alpha{1}}.witness_count();
}

static_assert(count_of_a_singleton() == 1);

// Left-bias keeps the count at one when both sides witness the SAME type.
consteval auto count_after_combining_same_type() -> std::size_t {
    const set_ab left{err_alpha{1}};
    const set_ab right{err_alpha{2}};
    return left.combined_with(right).witness_count();
}

static_assert(count_after_combining_same_type() == 1);

// =========================================================================
// recover (docs/transpose/transpose-grading-plan.md#recover-narrowing): narrows
// the grade by set difference. HANDLED is ANNOTATED (explicit template
// arguments) and CHECKED (membership, and the handler's result shape) --
// docs/transpose/decisions.md#recover-grade-inference. Elimination is a static
// fold over HANDLED with runtime presence filtering, built from the existing
// public API only -- docs/transpose/decisions.md#multi-witness-elimination.
// =========================================================================

using exp_ab = std::expected<int, set_ab>;
using exp_a = std::expected<int, set_a>;
using exp_b = std::expected<int, set_b>;

auto recover_to_int = [](const err_alpha &a) { return a.value; };

// -- Narrowing verified in deduced types -----------------------------------

static_assert(
    std::is_same_v<decltype(bt::recover<err_alpha>(
                       std::declval<const exp_ab &>(), recover_to_int)),
                   exp_b>);

// Handling every member of the grade collapses to the bare value: ∅ is bare
// T, per docs/transpose/decisions.md#empty-grade-spelling, and rebind_grade
// already mechanizes that collapse.
static_assert(std::is_same_v<decltype(bt::recover<err_alpha, err_beta>(
                                 std::declval<const exp_ab &>(),
                                 [](const auto &e) { return e.value; })),
                             int>);

// A handler that can itself raise widens the "raised" side of the formula:
// (e \ HANDLED) ∪ raised.
using set_g = bt::error_set<err_gamma>;
auto recover_may_raise_gamma =
    [](const err_alpha &a) -> std::expected<int, set_g> {
    if (a.value < 0) {
        return std::expected<int, set_g>{std::unexpect, err_gamma{a.value}};
    }
    return std::expected<int, set_g>{a.value};
};
static_assert(std::is_same_v<
              decltype(bt::recover<err_alpha>(std::declval<const exp_ab &>(),
                                              recover_may_raise_gamma)),
              std::expected<int, bt::error_set<err_beta, err_gamma>>>);

TEST_CASE("recover: a value that never held the handled type passes through "
          "unchanged, just narrower") {
    exp_ab held_beta{std::unexpect, set_ab{err_beta{9}}};

    auto result = bt::recover<err_alpha>(held_beta, recover_to_int);

    static_assert(std::is_same_v<decltype(result), exp_b>);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().holds<err_beta>());
    REQUIRE(result.error().witness<err_beta>() == std::optional{err_beta{9}});
}

TEST_CASE("recover: the handled type present and alone recovers to a value") {
    exp_ab held_alpha{std::unexpect, set_ab{err_alpha{4}}};

    auto result = bt::recover<err_alpha>(held_alpha, recover_to_int);

    static_assert(std::is_same_v<decltype(result), exp_b>);
    REQUIRE(result.has_value());
    REQUIRE(*result == 4);
}

TEST_CASE("recover: an already-successful computation passes through, just "
          "re-typed at the narrower grade") {
    exp_ab already_ok{7};

    auto result = bt::recover<err_alpha>(already_ok, recover_to_int);

    static_assert(std::is_same_v<decltype(result), exp_b>);
    REQUIRE(result.has_value());
    REQUIRE(*result == 7);
}

TEST_CASE("recover: handling every member of the grade collapses to the "
          "bare value type") {
    exp_ab held_alpha{std::unexpect, set_ab{err_alpha{4}}};

    auto result = bt::recover<err_alpha, err_beta>(
        held_alpha, [](const auto &e) { return e.value; });

    static_assert(std::is_same_v<decltype(result), int>);
    REQUIRE(result == 4);
}

// -- The multi-witness case: "set-difference at the value level" ----------
// docs/transpose/decisions.md#accumulation-evidence, in the section's own
// words: "recovering type A from a value that raised {a,b} ... the handler
// consumes the A witness, {b} remains an error, empty means success."

TEST_CASE("recover: a multi-witness value keeps the unhandled witness as a "
          "still-failing, narrower error") {
    set_ab both =
        bt::error_set_combine(set_ab{err_alpha{1}}, set_ab{err_beta{2}});
    exp_ab computation{std::unexpect, both};
    REQUIRE(computation.error().witness_count() == 2);

    auto result = bt::recover<err_alpha>(computation, recover_to_int);

    static_assert(std::is_same_v<decltype(result), exp_b>);
    REQUIRE_FALSE(result.has_value()); // {b} remains -- not empty, not success
    REQUIRE(result.error().holds<err_beta>());
    REQUIRE(result.error().witness<err_beta>() == std::optional{err_beta{2}});
}

TEST_CASE("recover: handling every witnessed type in a multi-witness value "
          "is success -- empty means success") {
    set_ab both =
        bt::error_set_combine(set_ab{err_alpha{1}}, set_ab{err_beta{2}});
    exp_ab computation{std::unexpect, both};

    auto result = bt::recover<err_alpha, err_beta>(
        computation, [](const auto &e) { return e.value * 10; });

    static_assert(std::is_same_v<decltype(result), int>);
    // Leftmost by canonical order wins when more than one handled witness is
    // simultaneously present and every one independently recovers --
    // err_alpha precedes err_beta.
    REQUIRE(result == 10);
}

// -- A handler that can itself raise ---------------------------------------

TEST_CASE("recover: a handler that raises widens the result with the fresh "
          "error, alongside any surviving unhandled witness") {
    exp_ab held_alpha{std::unexpect, set_ab{err_alpha{-1}}};

    auto result = bt::recover<err_alpha>(held_alpha, recover_may_raise_gamma);

    static_assert(
        std::is_same_v<decltype(result),
                       std::expected<int, bt::error_set<err_beta, err_gamma>>>);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().holds<err_gamma>());
    REQUIRE_FALSE(result.error().holds<err_beta>());
    REQUIRE(result.error().witness<err_gamma>() ==
            std::optional{err_gamma{-1}});
}

TEST_CASE("recover: a handler that succeeds still narrows, and drops the "
          "would-be-raised alternative entirely from the value") {
    exp_ab held_alpha{std::unexpect, set_ab{err_alpha{5}}};

    auto result = bt::recover<err_alpha>(held_alpha, recover_may_raise_gamma);

    static_assert(
        std::is_same_v<decltype(result),
                       std::expected<int, bt::error_set<err_beta, err_gamma>>>);
    REQUIRE(result.has_value());
    REQUIRE(*result == 5);
}

// -- Negative controls, per
// docs/transpose/decisions.md#recover-grade-inference's CHECKED half. Both
// static_asserts below were hand-verified to fire: inverted (recover<err_gamma>
// against set_ab; a handler returning `double` instead of
// `int`/`std::expected<int,...>`), the build failed with the static_assert's
// own message naming the reason, then restored. A permanently-failing
// translation unit cannot live in this suite, so the verification is documented
// rather than encoded, matching the precedent in accumulating_object.test.cpp
// for the accumulating object's refused bind.
//
//   bt::recover<err_gamma>(std::declval<const exp_ab &>(), recover_to_int);
//     -- fails: "recover's HANDLED types must be members of the
//        computation's grade"
//   bt::recover<err_alpha>(std::declval<const exp_ab &>(),
//                      [](const err_alpha &) -> double { return 0.0; });
//     -- fails: "recover's handler must return either the recovered VALUE
//        directly, or std::expected<VALUE, error_set<...>>"

TEST_CASE("recover: negative controls on the CHECKED half were hand-verified") {
    SUCCEED(
        "recover<err_gamma>(exp_ab, ...) and a wrong-return-type handler both "
        "fail to compile with the static_assert's own message; see the "
        "comment above for how this was verified");
}

TEST_CASE("error_set: witness_count reports how many alternatives did raise") {
    set_ab single{err_alpha{1}};
    REQUIRE(single.witness_count() == 1);

    auto combined = single.combined_with(set_ab{err_beta{2}});
    REQUIRE(combined.witness_count() == 2);
    REQUIRE(combined.holds<err_alpha>());
    REQUIRE(combined.holds<err_beta>());

    // A caller checks rather than trips: this is the guarded idiom the
    // precondition expects of anyone holding a possibly-accumulated value.
    if (combined.witness_count() == 1) {
        FAIL("combined value should witness two alternatives");
    }
    REQUIRE(combined.witness<err_alpha>().has_value());
    REQUIRE(combined.witness<err_beta>().has_value());
}

// =========================================================================
// THE UNIFORM SPELLING AND CLOSURE AT THE EMPTY GRADE. The empty grade is
// the bare value (docs/transpose/decisions.md#empty-grade-spelling); `fallible`
// is the explicit spelling that reaches expected<T, error_set<>>
// (docs/transpose/decisions.md#uniform-form-surface), and recover on it is
// vacuous (docs/transpose/decisions.md#generic-closure-at-empty-grade).
// =========================================================================

namespace {

/** A handler that is not callable with anything: if recover at the empty
 * grade so much as asked whether it could invoke this, the test would not
 * compile. */
struct uncallable {};

/** Generic code written once for every grade: recover from err_alpha and
 * hand back whatever remains. Instantiated below at a real grade and at the
 * empty grade through the uniform spelling. */
template <class COMPUTATION>
auto settle(COMPUTATION computation) {
    return bt::recover<err_alpha>(bt::fallible(std::move(computation)),
                                  [](const err_alpha &a) { return a.value; });
}

} // namespace

TEST_CASE("fallible: the explicit spelling of a value at the empty grade") {
    auto uniform = bt::fallible(7);
    STATIC_REQUIRE(
        std::is_same_v<decltype(uniform), std::expected<int, empty_set>>);
    REQUIRE(uniform.has_value());
    REQUIRE(*uniform == 7);

    // Already in the carrier: identity, at whatever grade it carries.
    auto graded = bt::fallible(std::expected<int, set_a>{3});
    STATIC_REQUIRE(std::is_same_v<decltype(graded), std::expected<int, set_a>>);
    auto plain = bt::fallible(std::expected<int, err_alpha>{3});
    STATIC_REQUIRE(
        std::is_same_v<decltype(plain), std::expected<int, err_alpha>>);

    // The spelling is the user's and only the user's: the framework's
    // re-indexing at the empty grade is still the bare value.
    STATIC_REQUIRE(std::is_same_v<bt::rebind_grade_t<int, empty_set>, int>);
    STATIC_REQUIRE(
        std::is_same_v<decltype(bt::grade_subsume<empty_set>(1)), int>);
}

TEST_CASE(
    "recover: vacuous at the empty grade -- the handler is neither "
    "invoked nor required to be callable, and the operand is the result") {
    auto uniform = bt::fallible(7);
    auto same = bt::recover<err_alpha>(uniform, uncallable{});
    STATIC_REQUIRE(
        std::is_same_v<decltype(same), std::expected<int, empty_set>>);
    REQUIRE(same == uniform);

    // The handled set is annotated as always; only its membership check is
    // waived, because there is no grade for it to be a member of.
    auto several = bt::recover<err_alpha, err_beta>(uniform, uncallable{});
    REQUIRE(*several == 7);
}

TEST_CASE("recover: generic code stays well-formed at the empty grade") {
    // At a real grade: the handler consumes the alternative and the grade
    // narrows to empty, which is spelled as the bare value.
    auto handled =
        settle(std::expected<int, set_a>{std::unexpect, err_alpha{5}});
    STATIC_REQUIRE(std::is_same_v<decltype(handled), int>);
    REQUIRE(handled == 5);

    // At the empty grade, through the uniform spelling: the same generic
    // function compiles, and does nothing.
    auto untouched = settle(9);
    STATIC_REQUIRE(
        std::is_same_v<decltype(untouched), std::expected<int, empty_set>>);
    REQUIRE(*untouched == 9);
}

// =========================================================================
// The type-order key. `error_set_of` lays its witness tuple out in canonical
// order, so it is declared in the inline namespace `ordered_by_type_name`,
// the key of the order in use (docs/transpose/decisions.md#type-order-abi).
// The stage asked for a compile-fail test that two translation units cannot
// mix order keys; with one key in existence that cannot be written, and
// this is the honest substitute: the key is part of the type's identity --
// visible in the compiler's rendering and reachable by the qualified
// spelling -- while every unqualified spelling above is unchanged.
// =========================================================================

TEST_CASE("error_set: the type-order key is part of error_set_of's identity") {
    STATIC_REQUIRE(
        std::is_same_v<set_a,
                       bt::ordered_by_type_name::error_set_of<err_alpha>>);
    STATIC_REQUIRE(
        std::is_same_v<empty_set, bt::ordered_by_type_name::error_set_of<>>);
    STATIC_REQUIRE(
        std::is_same_v<bt::error_set<err_alpha>, bt::error_set_of<err_alpha>>);

#if defined(__GNUC__) && !defined(__clang__)
    // GCC's __PRETTY_FUNCTION__ renders inline namespaces, so the key is
    // visible in the type's name. Clang suppresses them in that rendering,
    // which is why this half is GCC-only; the is_same checks above hold on
    // every compiler.
    STATIC_REQUIRE(bt::detail::type_name<set_ab>().find(
                       "ordered_by_type_name") != std::string_view::npos);
#endif
}

// =========================================================================
// PUBLIC ELIMINATORS FOR A MULTI-WITNESS VALUE. The fence of
// docs/transpose/decisions.md#multi-witness-elimination moved on 2026-11-01:
// fold_witnesses threads every present witness through an accumulator, and
// collapse maps every alternative to one error type at the boundary. Both are
// total over the witnessed subset; visit stays partial. Stage set-elimination
// of docs/transpose/transpose-surface-plan.md.
// =========================================================================

namespace {

auto describe(const err_alpha &a) -> std::string {
    return "alpha=" + std::to_string(a.value);
}
auto describe(const err_beta &b) -> std::string {
    return "beta=" + std::to_string(b.value);
}

using exp_ab = std::expected<int, set_ab>;

/** A value witnessing both alternatives, in the order they were raised. */
auto both_raised() -> exp_ab {
    return exp_ab{std::unexpect,
                  set_ab{err_beta{2}}.combined_with(set_ab{err_alpha{1}})};
}

} // namespace

TEST_CASE("fold_witnesses: every present witness reaches the accumulator, in "
          "canonical order, by overload set") {
    auto both = both_raised();
    REQUIRE(both.error().witness_count() == 2);

    auto messages = bt::fold_witnesses(
        both.error(), std::vector<std::string>{},
        bt::overloaded{
            [](std::vector<std::string> acc, const err_alpha &a) {
                acc.push_back(describe(a));
                return acc;
            },
            [](std::vector<std::string> acc, const err_beta &b) {
                acc.push_back(describe(b));
                return acc;
            },
        });
    // Canonical order, not raise order: alpha precedes beta by name.
    REQUIRE(messages == std::vector<std::string>{"alpha=1", "beta=2"});

    // A generic arm is enough when the alternatives share an interface, and
    // a single witness folds once.
    auto count = bt::fold_witnesses(both.error(), 0,
                                    [](int n, const auto &) { return n + 1; });
    REQUIRE(count == 2);
    REQUIRE(
        bt::fold_witnesses(set_ab{err_beta{7}}, 0, [](int n, const auto &e) {
            return n + e.value;
        }) == 7);
}

TEST_CASE("collapse: every alternative to one error type, deduced from the "
          "arms; the first witness in canonical order is kept by default") {
    auto first = bt::collapse(
        both_raised(), bt::overloaded{
                           [](const err_alpha &a) { return describe(a); },
                           [](const err_beta &b) { return describe(b); },
                       });
    // The grade is gone: the result is the ungraded expected.
    STATIC_REQUIRE(
        std::is_same_v<decltype(first), std::expected<int, std::string>>);
    REQUIRE_FALSE(first.has_value());
    REQUIRE(first.error() == "alpha=1");
}

TEST_CASE("collapse: a combine keeps the second witness") {
    auto joined = bt::collapse(
        both_raised(), [](const auto &e) { return describe(e) + ";"; },
        std::plus<>{});
    REQUIRE(joined.error() == "alpha=1;beta=2;");
}

TEST_CASE("collapse: a named target converts each arm's result, and a success "
          "passes through") {
    auto code = bt::collapse<long>(
        both_raised().error(),
        bt::overloaded{
            [](const err_alpha &a) { return a.value; },
            [](const err_beta &b) { return static_cast<short>(b.value); },
        });
    STATIC_REQUIRE(std::is_same_v<decltype(code), long>);
    REQUIRE(code == 1);

    auto ok =
        bt::collapse(exp_ab{7}, [](const auto &e) { return describe(e); });
    STATIC_REQUIRE(
        std::is_same_v<decltype(ok), std::expected<int, std::string>>);
    REQUIRE(ok == std::expected<int, std::string>{7});

    // Staying graded is a matter of naming the singleton set as the target.
    auto graded = bt::collapse<bt::error_set<std::string>>(
        both_raised(), [](const auto &e) { return describe(e); });
    STATIC_REQUIRE(
        std::is_same_v<decltype(graded),
                       std::expected<int, bt::error_set<std::string>>>);
    REQUIRE(graded.error().holds<std::string>());
}

TEST_CASE("collapse: a void computation collapses too") {
    using exp_void_ab = std::expected<void, set_ab>;
    auto failed = bt::collapse(exp_void_ab{std::unexpect, set_ab{err_alpha{3}}},
                               [](const auto &e) { return describe(e); });
    STATIC_REQUIRE(
        std::is_same_v<decltype(failed), std::expected<void, std::string>>);
    REQUIRE(failed.error() == "alpha=3");
    auto fine =
        bt::collapse(exp_void_ab{}, [](const auto &e) { return describe(e); });
    REQUIRE(fine.has_value());
}

TEST_CASE("fold_witnesses and collapse: constexpr, and collapse agrees with "
          "visit on a single witness") {
    constexpr auto n = [] {
        set_ab e{err_alpha{3}};
        return bt::fold_witnesses(
            e, 0, [](int acc, const auto &) { return acc + 1; });
    }();
    STATIC_REQUIRE(n == 1);

    constexpr set_ab single{err_beta{4}};
    constexpr auto collapsed =
        bt::collapse(single, [](const auto &e) { return e.value; });
    STATIC_REQUIRE(collapsed == 4);
    // The equivalence law: where visit is defined, collapse agrees with it.
    REQUIRE(collapsed == single.visit([](const auto &e) { return e.value; }));
}

// Negative controls, hand-verified with GCC 15 (no compile-fail mechanism in
// this repository):
//
//     bt::collapse(set_ab{err_alpha{3}}, bt::overloaded{[](const err_alpha&) {
//     return 1; }});
//
// fails with "collapse: no arm accepts this alternative -- add a case" in the
// instantiation of collapse_arm_check<..., err_beta>, naming the missing
// alternative, and
//
//     bt::collapse(set_ab{err_alpha{3}}, bt::overloaded{
//         [](const err_alpha&) { return 1; },
//         [](const err_beta&) { return std::string{}; }});
//
// fails with "collapse: the arms return different types. Name the target,
// collapse<TARGET>(...), and every arm's result converts to it."
TEST_CASE("eliminators: the negative controls were hand-verified") {
    SUCCEED("recorded in the comment above");
}
