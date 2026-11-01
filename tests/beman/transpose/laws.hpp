// tests/beman/transpose/laws.hpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_TEST_LAWS_HPP
#define BEMAN_TRANSPOSE_TEST_LAWS_HPP

// The graded-law harness: semantic requirements, checked on demand.
//
// A TEST DELIVERABLE, NOT A REGISTRATION GATE. Nothing in the shipped headers
// includes this one, and no concept or `requires` clause anywhere calls
// `check_graded_laws`. That is deliberate and is the standard's own posture:
// semantic requirements are stated in prose, and checking them is opt-in. A
// bool-returning law checker that shipped alongside the concept would migrate
// into `requires` clauses -- it is one token to add and it looks like rigour
// -- and every constraint that merely MENTIONS a grade would then drag a full
// law check behind it, at every overload-resolution site, forever. The
// economics of a law check are pay-once-per-model; the economics of a
// constraint are pay-per-use. See `grade_semilattice` in
// include/beman/transpose/grade.hpp, which says the same thing from the other
// side, and the deliberate-omissions item in
// docs/transpose/transpose-grading-plan.md#paper-revision.
//
// FRAMEWORK VOCABULARY ONLY. This header includes grade.hpp and nothing else
// from the library. It never names `error_set`, `recover`, or `expected`. If
// it ever has to, the claim of
// docs/transpose/decisions.md#grade-generality -- that the framework layer
// speaks only grade vocabulary and `error_set` is merely one model -- is false,
// and the right response is to log the leak, not to add the include.
//
// A MODEL UNDER TEST supplies two type lists and nothing else:
//   - `grades`: a `grade_sample<...>` of elements to quantify over. For a
//     free semilattice on n generators the full 2^n subsets exercise every
//     law shape; a smaller sample is a weaker test, not a different one.
//   - `values`: a `value_sample<...>` of ordinary value types to instantiate
//     carriers at, via `rebind_grade`. A move-only probe is worth including:
//     the value-level laws run inside `consteval`, so a stray copy on the
//     subsumption path is a compile error rather than a performance note.
//
// Laws are checked with `static_assert` INSIDE the checkers rather than only
// by the returned bool, so a violation names the law and the offending grades
// instead of collapsing to `static_assert(false)` at the call site. The bool
// is returned anyway, because the plan's surface is
// `static_assert(check_graded_laws<Model>())` and a checker that cannot be
// used that way is not the deliverable.

#include <beman/transpose/grade.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose::test::laws {

// -- What a model under test supplies -------------------------------------

/** The grade elements to quantify the algebra laws over. */
template <class... GRADES>
struct grade_sample {};

/** The value types to instantiate carriers at, for the carrier laws. */
template <class... VALUES>
struct value_sample {};

/** A model under test: a grade sample and a value sample. Deliberately not a
 * description of the algebra itself -- that is what `grade_semilattice`
 * already is, and the harness is written against the concept. This says only
 * WHICH elements to try. */
template <class MODEL>
concept graded_model = requires {
    typename MODEL::grades;
    typename MODEL::values;
};

/** A distinguishable value of a probe type. The default builds one from an
 * int; specialize for a probe that cannot be built that way. */
template <class VALUE>
struct probe_value {
    static constexpr auto make() -> VALUE;
};

template <class VALUE>
constexpr auto probe_value<VALUE>::make() -> VALUE {
    return VALUE(42);
}

template <class VALUE>
struct probe_value<std::unique_ptr<VALUE>> {
    static constexpr auto make() -> std::unique_ptr<VALUE>;
};

template <class VALUE>
constexpr auto probe_value<std::unique_ptr<VALUE>>::make()
    -> std::unique_ptr<VALUE> {
    return std::make_unique<VALUE>(42);
}

// -- Declarations ---------------------------------------------------------

template <grade_semilattice GRADE>
consteval auto check_unary_grade_laws() -> bool;

template <grade_semilattice LEFT, grade_semilattice RIGHT>
consteval auto check_binary_grade_laws() -> bool;

template <grade_semilattice LEFT, grade_semilattice MIDDLE,
          grade_semilattice RIGHT>
consteval auto check_ternary_grade_laws() -> bool;

template <class VALUE, grade_semilattice GRADE>
consteval auto check_carrier_at() -> bool;

template <class VALUE, grade_semilattice FROM, grade_semilattice TO>
consteval auto check_subsumption_coherence() -> bool;

template <class... GRADES>
consteval auto check_grade_algebra(grade_sample<GRADES...>) -> bool;

template <class LEFT, class... GRADES>
consteval auto check_grade_algebra_from(grade_sample<GRADES...>) -> bool;

template <class LEFT, class MIDDLE, class... GRADES>
consteval auto check_grade_algebra_through(grade_sample<GRADES...>) -> bool;

template <class... GRADES, class... VALUES>
consteval auto check_carriers(grade_sample<GRADES...>, value_sample<VALUES...>)
    -> bool;

template <class VALUE, class... GRADES>
consteval auto check_carriers_for(grade_sample<GRADES...>) -> bool;

template <class VALUE, class FROM, class... GRADES>
consteval auto check_coherence_from(grade_sample<GRADES...>) -> bool;

template <graded_model MODEL>
consteval auto check_graded_laws() -> bool;

// -- The algebra laws, one grade at a time --------------------------------
//
// Idempotence and the unit are the two properties
// docs/transpose/decisions.md#grade-generality calls load-bearing on their own:
// idempotent makes fold grades shape-independent, so recursive data types
// stay ungraded while effects are graded; the unit is what lets an ungraded
// computation sit in a graded pipeline unchanged.

template <grade_semilattice GRADE>
consteval auto check_unary_grade_laws() -> bool {
    static_assert(std::is_same_v<grade_join_t<GRADE, GRADE>, GRADE>,
                  "Idempotence: join(g, g) == g. Without it, fold grades "
                  "depend on the runtime shape of the structure being "
                  "folded, which is the case grade-generality excludes.");
    static_assert(
        std::is_same_v<grade_join_t<grade_bottom_t<GRADE>, GRADE>, GRADE>,
        "Left unit: join(bottom, g) == g.");
    static_assert(
        std::is_same_v<grade_join_t<GRADE, grade_bottom_t<GRADE>>, GRADE>,
        "Right unit: join(g, bottom) == g.");
    static_assert(std::is_same_v<grade_bottom_t<grade_bottom_t<GRADE>>,
                                 grade_bottom_t<GRADE>>,
                  "Bottom is its own bottom: the model has one ∅, and "
                  "asking ∅ for its ∅ does not move.");
    static_assert(std::is_same_v<grade_model_t<GRADE>,
                                 grade_model_t<grade_bottom_t<GRADE>>>,
                  "A grade and its bottom belong to the same model.");
    static_assert(grade_subsumes_v<GRADE, GRADE>,
                  "Reflexivity: every grade subsumes into itself. Follows "
                  "from idempotence by order-from-join; checked separately "
                  "so a hand-written order that disagrees is caught.");
    static_assert(grade_subsumes_v<grade_bottom_t<GRADE>, GRADE>,
                  "Bottom is least: a computation that raises nothing is "
                  "usable wherever one that may raise something is.");

    return true;
}

// -- The algebra laws, two grades at a time -------------------------------

template <grade_semilattice LEFT, grade_semilattice RIGHT>
consteval auto check_binary_grade_laws() -> bool {
    static_assert(
        std::is_same_v<grade_join_t<LEFT, RIGHT>, grade_join_t<RIGHT, LEFT>>,
        "Commutativity: join(a, b) == join(b, a). This is what "
        "makes grade arithmetic order-free, so the order operands "
        "are written in cannot change a deduced type.");
    static_assert(grade_subsumes_v<LEFT, RIGHT> ==
                      std::is_same_v<grade_join_t<LEFT, RIGHT>, RIGHT>,
                  "Order from join: a ⊑ b exactly when join(a, b) == b. The "
                  "order must be READ OFF the join rather than declared "
                  "beside it -- that is what makes the inclusion between two "
                  "grades unique, hence the coercion canonical, hence "
                  "subsumption coherently implicit.");
    static_assert(grade_subsumes_v<LEFT, grade_join_t<LEFT, RIGHT>>,
                  "The join is an upper bound of its left operand.");
    static_assert(grade_subsumes_v<RIGHT, grade_join_t<LEFT, RIGHT>>,
                  "The join is an upper bound of its right operand.");
    static_assert(
        !(grade_subsumes_v<LEFT, RIGHT> && grade_subsumes_v<RIGHT, LEFT>) ||
            std::is_same_v<LEFT, RIGHT>,
        "Antisymmetry: two grades that subsume into each other are "
        "the same TYPE. A model whose canonical forms are not "
        "decidable fails here, which is the requirement "
        "grade-generality places on any future algebra.");
    static_assert(
        std::is_same_v<grade_bottom_t<LEFT>, grade_bottom_t<RIGHT>>,
        "One model, one bottom: every grade in a sample must agree on ∅.");
    static_assert(std::is_same_v<grade_model_t<LEFT>, grade_model_t<RIGHT>>,
                  "One sample, one model: law checks quantify within a "
                  "single grade model.");
    return true;
}

// -- The algebra laws, three grades at a time -----------------------------

template <grade_semilattice LEFT, grade_semilattice MIDDLE,
          grade_semilattice RIGHT>
consteval auto check_ternary_grade_laws() -> bool {
    static_assert(
        std::is_same_v<grade_join_t<grade_join_t<LEFT, MIDDLE>, RIGHT>,
                       grade_join_t<LEFT, grade_join_t<MIDDLE, RIGHT>>>,
        "Associativity: join(join(a, b), c) == join(a, join(b, c)). With "
        "commutativity this makes a fold of joins independent of how the "
        "fold was bracketed.");
    static_assert(
        !(grade_subsumes_v<LEFT, MIDDLE> && grade_subsumes_v<MIDDLE, RIGHT>) ||
            grade_subsumes_v<LEFT, RIGHT>,
        "Transitivity of the order.");
    return true;
}

// -- Carrier laws ---------------------------------------------------------
//
// These are where the algebra meets a type that holds values. They run inside
// `consteval`: every carrier below is actually constructed during constant
// evaluation, so a subsumption path that allocates, copies a move-only value,
// or reaches a non-constexpr function fails to compile.

template <class VALUE, grade_semilattice GRADE>
consteval auto check_carrier_at() -> bool {
    static_assert(std::is_same_v<grade_of_t<VALUE>, unit_grade>,
                  "A value probe must be an ORDINARY type -- ∅-graded, "
                  "unknown to any model. A probe the model already grades "
                  "would test the model against itself.");

    using carrier = rebind_grade_t<VALUE, GRADE>;

    // grade_of ∘ rebind_grade is the identity, with ONE licensed exception:
    // at the model's bottom the carrier is the bare value again
    // (docs/transpose/decisions.md#empty-grade-spelling), and a bare value
    // reports the framework's ungraded sentinel rather than a model grade.
    static_assert(
        std::is_same_v<grade_of_t<carrier>, GRADE> ||
            (std::is_same_v<GRADE, grade_bottom_t<GRADE>> &&
             std::is_same_v<grade_of_t<carrier>, unit_grade>),
        "Round trip: re-indexing a value at a grade and then asking for its "
        "grade returns what was asked for -- except at the model's bottom, "
        "where the carrier is the bare value and reports the framework's ∅.");

    // Re-indexing is REPLACEMENT, never nesting: asking a carrier for the
    // grade it already has changes nothing, and narrowing it to the model's ∅
    // hands back the bare value rather than a degenerate carrier. The second
    // of these is the mechanized form of
    // docs/transpose/decisions.md#empty-grade-spelling's sentinel, generalized
    // off the shipped model: NO model's framework path may materialize an
    // ∅-graded carrier the user did not write.
    static_assert(std::is_same_v<rebind_grade_t<carrier, GRADE>, carrier>,
                  "Re-indexing a carrier at its own grade is the identity. A "
                  "model that nests here builds carrier<carrier<T>> the first "
                  "time a grade is recomputed.");
    static_assert(
        std::is_same_v<rebind_grade_t<carrier, grade_bottom_t<GRADE>>, VALUE>,
        "Narrowing a carrier to the model's ∅ yields the BARE value.");

    static_assert(
        std::is_same_v<decltype(grade_subsume<GRADE>(
                           probe_value<VALUE>::make())),
                       carrier>,
        "Promotion: subsuming a bare value into a grade produces that "
        "grade's carrier. The bare value IS the ∅ fiber, so promotion is "
        "just subsumption from ∅ -- there is no separate lifting operation "
        "for the framework to get wrong.");

    // Actually run it, in a constant expression.
    auto promoted = grade_subsume<GRADE>(probe_value<VALUE>::make());
    static_assert(std::is_same_v<decltype(promoted), carrier>);
    return true;
}

template <class VALUE, grade_semilattice FROM, grade_semilattice TO>
consteval auto check_subsumption_coherence() -> bool {
    if constexpr (grade_subsumes_v<FROM, TO>) {
        using narrow = rebind_grade_t<VALUE, FROM>;
        using wide = rebind_grade_t<VALUE, TO>;

        static_assert(
            std::is_same_v<decltype(grade_subsume<TO>(grade_subsume<FROM>(
                               probe_value<VALUE>::make()))),
                           wide>,
            "Composed subsumption lands in the same carrier as the direct "
            "one. A licensed widening must stay licensed after another "
            "widening has already happened.");

        // COHERENCE, the payoff law: going ∅ → FROM → TO and going ∅ → TO
        // directly produce the same value. Two different coercion paths
        // between the same pair of grades must agree, or "subsumption is
        // implicit" is a promise the type system cannot keep. Skipped for a
        // move-only probe: comparing two independently-built values there
        // compares identities, not contents.
        if constexpr (std::copy_constructible<VALUE> &&
                      std::equality_comparable<wide>) {
            const wide direct = grade_subsume<TO>(probe_value<VALUE>::make());
            const wide composed = grade_subsume<TO>(
                grade_subsume<FROM>(probe_value<VALUE>::make()));
            if (!(direct == composed)) {
                return false;
            }
        } else {
            // Still exercise the path, for the move-only probe: it runs in a
            // constant expression, so a stray copy is a compile error.
            auto composed = grade_subsume<TO>(
                grade_subsume<FROM>(probe_value<VALUE>::make()));
            static_assert(std::is_same_v<decltype(composed), wide>);
        }
        static_assert(std::is_same_v<narrow, rebind_grade_t<VALUE, FROM>>);
    }
    return true;
}

// -- Quantification -------------------------------------------------------

template <class... GRADES>
consteval auto check_grade_algebra(grade_sample<GRADES...> sample) -> bool {
    return (check_unary_grade_laws<GRADES>() && ...) &&
           (check_grade_algebra_from<GRADES>(sample) && ...);
}

template <class LEFT, class... GRADES>
consteval auto check_grade_algebra_from(grade_sample<GRADES...> sample)
    -> bool {
    return (check_binary_grade_laws<LEFT, GRADES>() && ...) &&
           (check_grade_algebra_through<LEFT, GRADES>(sample) && ...);
}

template <class LEFT, class MIDDLE, class... GRADES>
consteval auto check_grade_algebra_through(grade_sample<GRADES...>) -> bool {
    return (check_ternary_grade_laws<LEFT, MIDDLE, GRADES>() && ...);
}

template <class VALUE, class FROM, class... GRADES>
consteval auto check_coherence_from(grade_sample<GRADES...>) -> bool {
    return (check_subsumption_coherence<VALUE, FROM, GRADES>() && ...);
}

template <class VALUE, class... GRADES>
consteval auto check_carriers_for(grade_sample<GRADES...> sample) -> bool {
    return (check_carrier_at<VALUE, GRADES>() && ...) &&
           (check_coherence_from<VALUE, GRADES>(sample) && ...);
}

template <class... GRADES, class... VALUES>
consteval auto check_carriers(grade_sample<GRADES...> grades,
                              value_sample<VALUES...>) -> bool {
    return (check_carriers_for<VALUES>(grades) && ...);
}

/** Checks every graded law for MODEL, over the grade and value samples it
 * names. Usable as `static_assert(check_graded_laws<Model>())`. */
template <graded_model MODEL>
consteval auto check_graded_laws() -> bool {
    return check_grade_algebra(typename MODEL::grades{}) &&
           check_carriers(typename MODEL::grades{}, typename MODEL::values{});
}

// =========================================================================
// INSTANCE LAWS: Functor, Applicative, Monad.
//
// The checkers above are about the grade algebra. These are about the
// typeclass OBJECTS registered over a carrier: `applicative_object` and
// `monad_object` are syntactic conformance, and this is the semantic half.
// Same posture as above -- a test deliverable, never a registration gate --
// and the same discipline: each law is its own `static_assert` with its
// own message, so a violation names the law rather than collapsing to
// `static_assert(false)` at the call site.
//
// STILL FRAMEWORK-FREE. Nothing here names a typeclass header. A checker is
// quantified over an OBJECT -- any object with the member it exercises
// (`fmap`, `pure`, `invoke`, `bind`, ...) -- and a SAMPLE, so the harness
// includes no instance and no base; the test that runs it does.
//
// THE LAWS ARE STATED IN invoke/pure FORM. `invoke` is the user-facing
// application verb of this library (apply.hpp's pattern invariants), and
// McBride & Paterson's canonical form `pure f <*> u1 <*> ... <*> un` is
// `invoke(f, u1, ..., un)`. The classic laws are recovered by taking the
// function to be `evaluate` and the operands to be contexts of callables;
// each checker says which form it is. Where an object also has `ap`, the
// basis-coherence check pins `ap(u, v)` to `invoke(evaluate, u, v)`.
//
// A SAMPLE UNDER TEST supplies:
//   - `context`: the carrier type, instantiated at `trace`.
//   - `contexts()`: a `static constexpr` function returning the contexts to
//     quantify over, as something range-for iterates. Include the empty or
//     failing shapes: the laws are most informative where a function is
//     never called.
//   - `arrows()` (optional, monad laws only): a `static constexpr` function
//     returning a `std::tuple` of extra Kleisli arrows `trace -> context`.
//     The harness always adds arrows of the form `pure . f`; a sample adds
//     the ones only it can spell, such as the one that fails.
//
// VALUE CATEGORY TRIPWIRE. The checkers hand every operand to the object as
// a `const` lvalue, or as the prvalue the law itself constructs (`pure(x)`,
// an inner `invoke`). No law asks an object to copy from an rvalue it did
// not offer, and none asks for `noexcept`. If a checker here ever needs
// either to compile, that is the probe-category defect
// docs/transpose/decisions.md#operand-value-category already fixed once:
// stop and log it, do not weaken the check.
// =========================================================================

// -- The probe algebra ----------------------------------------------------

/** The value algebra the instance laws compute in: a short string under
 * concatenation.
 *
 * Every law below is an equation between two ways of building a context,
 * and the function the two sides apply is the only thing that can tell
 * them apart. Under integer addition an instance that applies its function
 * twice (`f(f(x))` for `f = +c` is `x + 2c`, and `x + c + c` is the same
 * number) or applies its operands in the wrong order (`a + b == b + a`)
 * can pass a law. Concatenation is neither idempotent nor commutative, so
 * each application and each operand leaves a distinct mark in the result:
 * a law that holds over it holds for the SHAPE of the computation, not for
 * an accident of the numbers.
 *
 * A fixed buffer rather than `std::string`, so that a value is a literal
 * type a `constexpr` variable may hold and a `static constexpr` sample
 * function may return. */
struct trace {
    static constexpr std::size_t capacity = 32;

    std::array<char, capacity> text{};
    std::size_t length{0};

    constexpr trace() = default;
    constexpr explicit trace(const char *literal);

    friend constexpr auto operator==(const trace &, const trace &)
        -> bool = default;
};

/** Thrown when a concatenation would overflow `trace::capacity`. Inside a
 * constant expression that is a compile error, pointing at the harness
 * rather than at a law; at runtime it is a test failure. */
struct trace_overflow {};

constexpr auto operator+(const trace &left, const trace &right) -> trace;

/** Appends a fixed tag: the unary function the laws map with. */
struct append_tag {
    trace tag;

    constexpr auto operator()(const trace &value) const -> trace;
};

/** Concatenates: the binary function the n-ary laws invoke with. */
struct concatenate {
    constexpr auto operator()(const trace &left, const trace &right) const
        -> trace;
};

/** A callable held IN a context: `prepend{a}(y) == a + y`. The classic
 * forms of interchange and composition quantify over contexts of callables;
 * the harness builds those from a sample's value contexts by invoking
 * `make_prepend` over them, so a sample never has to spell one. */
struct prepend {
    trace left;

    constexpr auto operator()(const trace &right) const -> trace;
};

struct make_prepend {
    constexpr auto operator()(const trace &value) const -> prepend;
};

/** Applies a callable to one argument: the evaluator that recovers one-step
 * application (`ap`) from the n-ary core. */
struct evaluate {
    template <class FUNCTION, class ARGUMENT>
    constexpr auto operator()(const FUNCTION &function,
                              const ARGUMENT &argument) const
        -> std::invoke_result_t<const FUNCTION &, const ARGUMENT &>;
};

struct identity_function {
    template <class VALUE>
    constexpr auto operator()(VALUE &&value) const
        -> std::remove_cvref_t<VALUE>;
};

/** The Kleisli arrow `pure . (+ tag)`: what the monad laws bind with when a
 * sample supplies no arrows of its own. */
template <const auto &OBJECT>
struct pure_after {
    trace tag;

    constexpr auto operator()(const trace &value) const;
};

// -- What a sample under test supplies ------------------------------------

template <class SAMPLE>
concept instance_sample = requires {
    typename SAMPLE::context;
    requires std::ranges::input_range<decltype(SAMPLE::contexts())>;
    requires std::same_as<
        std::ranges::range_value_t<decltype(SAMPLE::contexts())>,
        typename SAMPLE::context>;
};

template <class SAMPLE>
concept supplies_arrows = requires { SAMPLE::arrows(); };

// -- Declarations ---------------------------------------------------------

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto functor_identity() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto functor_composition() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_identity() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_homomorphism() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_interchange() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_interchange_nary() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_composition() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_composition_nary() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto ap_agrees_with_invoke() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto derived_applicative_agreement() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_left_identity() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_right_identity() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_associativity() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_fmap_grounding() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_join_coherence() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_invoke_coherence() -> bool;

template <const auto &FUNCTOR_LIKE, const auto &APPLICATIVE,
          instance_sample SAMPLE>
constexpr auto fmap_agrees_with_invoke() -> bool;

template <const auto &LEFT, const auto &RIGHT, instance_sample SAMPLE>
constexpr auto invoke_agrees() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
consteval auto check_functor_laws() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
consteval auto check_applicative_laws() -> bool;

template <const auto &OBJECT, instance_sample SAMPLE>
consteval auto check_monad_laws() -> bool;

// -- The probe algebra, defined -------------------------------------------

constexpr trace::trace(const char *literal) {
    for (; *literal != '\0'; ++literal) {
        if (length == capacity) {
            throw trace_overflow{};
        }
        text[length++] = *literal;
    }
}

constexpr auto operator+(const trace &left, const trace &right) -> trace {
    if (left.length + right.length > trace::capacity) {
        throw trace_overflow{};
    }
    trace result = left;
    for (std::size_t index = 0; index < right.length; ++index) {
        result.text[result.length++] = right.text[index];
    }
    return result;
}

constexpr auto append_tag::operator()(const trace &value) const -> trace {
    return value + tag;
}

constexpr auto concatenate::operator()(const trace &left,
                                       const trace &right) const -> trace {
    return left + right;
}

constexpr auto prepend::operator()(const trace &right) const -> trace {
    return left + right;
}

constexpr auto make_prepend::operator()(const trace &value) const -> prepend {
    return prepend{value};
}

template <class FUNCTION, class ARGUMENT>
constexpr auto evaluate::operator()(const FUNCTION &function,
                                    const ARGUMENT &argument) const
    -> std::invoke_result_t<const FUNCTION &, const ARGUMENT &> {
    return std::invoke(function, argument);
}

template <class VALUE>
constexpr auto identity_function::operator()(VALUE &&value) const
    -> std::remove_cvref_t<VALUE> {
    return std::forward<VALUE>(value);
}

template <const auto &OBJECT>
constexpr auto pure_after<OBJECT>::operator()(const trace &value) const {
    return OBJECT.pure(value + tag);
}

// The non-idempotent, order-sensitive probe values and functions every law
// uses. One character each so that the deepest nesting stays well inside
// `trace::capacity`.
inline constexpr trace probe_x{"x"};
inline constexpr trace probe_y{"y"};
inline constexpr append_tag probe_f{trace{"f"}};
inline constexpr append_tag probe_g{trace{"g"}};
inline constexpr concatenate probe_h{};

// A `trace` and its tags really do form a non-idempotent, order-sensitive
// algebra; otherwise the laws below could be satisfied by accident.
static_assert(probe_x + probe_x != probe_x, "concatenation is not idempotent");
static_assert(probe_x + probe_y != probe_y + probe_x,
              "concatenation is not commutative");
static_assert(probe_f(probe_f(probe_x)) != probe_f(probe_x),
              "applying the probe function twice is observable");

namespace detail {

/** The Kleisli arrows the monad laws quantify over: the harness's own
 * `pure . f` arrows, followed by whatever the sample adds. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto arrows_for() {
    auto own = std::tuple{pure_after<OBJECT>{trace{"k"}},
                          pure_after<OBJECT>{trace{"h"}}};
    if constexpr (supplies_arrows<SAMPLE>) {
        return std::tuple_cat(own, SAMPLE::arrows());
    } else {
        return own;
    }
}

/** `predicate` holds for every element of `tuple`. */
template <class TUPLE, class PREDICATE>
constexpr auto all_of_tuple(const TUPLE &tuple, PREDICATE &&predicate) -> bool {
    return std::apply(
        [&](const auto &...element) { return (predicate(element) && ...); },
        tuple);
}

} // namespace detail

// -- Functor laws ---------------------------------------------------------

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto functor_identity() -> bool {
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.fmap(identity_function{}, context) == context)) {
            return false;
        }
    }
    return true;
}

template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto functor_composition() -> bool {
    auto composed = [](const trace &value) { return probe_f(probe_g(value)); };
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.fmap(composed, context) ==
              OBJECT.fmap(probe_f, OBJECT.fmap(probe_g, context)))) {
            return false;
        }
    }
    return true;
}

// -- Applicative laws -----------------------------------------------------

/** identity: `invoke(id, v) == v`. The unary case of `invoke` is the
 * functor's map, and this is its identity law stated on the applicative
 * object; it holds for contexts that cannot hold a callable at all. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_identity() -> bool {
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.invoke(identity_function{}, context) == context)) {
            return false;
        }
    }
    return true;
}

/** homomorphism: `invoke(f, pure(x)) == pure(f(x))`, and at arity two
 * `invoke(h, pure(x), pure(y)) == pure(h(x, y))`. Pure operands never
 * leave the pure fibre, and `pure` leaves no mark of its own. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_homomorphism() -> bool {
    return OBJECT.invoke(probe_f, OBJECT.pure(probe_x)) ==
               OBJECT.pure(probe_f(probe_x)) &&
           OBJECT.invoke(probe_h, OBJECT.pure(probe_x), OBJECT.pure(probe_y)) ==
               OBJECT.pure(probe_h(probe_x, probe_y));
}

/** interchange, classic form: `u <*> pure y == pure ($ y) <*> u`, spelled
 * `invoke(evaluate, u, pure(y)) == invoke(g -> g(y), u)` for a context `u`
 * of callables, which the harness builds from each value context. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_interchange() -> bool {
    auto apply_to_y = [](const prepend &function) { return function(probe_y); };
    for (const auto &context : SAMPLE::contexts()) {
        const auto functions = OBJECT.invoke(make_prepend{}, context);
        if (!(OBJECT.invoke(evaluate{}, functions, OBJECT.pure(probe_y)) ==
              OBJECT.invoke(apply_to_y, functions))) {
            return false;
        }
    }
    return true;
}

/** interchange, n-ary form: a pure operand floats out of an application in
 * either position, `invoke(h, u, pure(y)) == invoke(a -> h(a, y), u)` and
 * `invoke(h, pure(x), v) == invoke(b -> h(x, b), v)`. With `h = evaluate`
 * the first is the classic law; stated for every `h` because `invoke` is
 * n-ary here and the pure operand may be anywhere in the list. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_interchange_nary() -> bool {
    auto with_y = [](const trace &a) { return probe_h(a, probe_y); };
    auto with_x = [](const trace &b) { return probe_h(probe_x, b); };
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.invoke(probe_h, context, OBJECT.pure(probe_y)) ==
              OBJECT.invoke(with_y, context))) {
            return false;
        }
        if (!(OBJECT.invoke(probe_h, OBJECT.pure(probe_x), context) ==
              OBJECT.invoke(with_x, context))) {
            return false;
        }
    }
    return true;
}

/** composition, classic form: `pure (.) <*> u <*> v <*> w == u <*> (v <*>
 * w)`, spelled `invoke((f, g, x) -> f(g(x)), u, v, w) == invoke(evaluate,
 * u, invoke(evaluate, v, w))` for contexts `u`, `v` of callables built from
 * the sample's value contexts and `w` a value context. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_composition() -> bool {
    auto compose = [](const prepend &f, const prepend &g, const trace &x) {
        return f(g(x));
    };
    const auto contexts = SAMPLE::contexts();
    for (const auto &first : contexts) {
        const auto u = OBJECT.invoke(make_prepend{}, first);
        for (const auto &second : contexts) {
            const auto v = OBJECT.invoke(make_prepend{}, second);
            for (const auto &w : contexts) {
                if (!(OBJECT.invoke(compose, u, v, w) ==
                      OBJECT.invoke(evaluate{}, u,
                                    OBJECT.invoke(evaluate{}, v, w)))) {
                    return false;
                }
            }
        }
    }
    return true;
}

/** composition, n-ary form: nesting `invoke` inside `invoke` is the same as
 * one flat `invoke` of the composed function, whichever operand the inner
 * one occupies. `invoke(h, u, invoke(h, v, w)) == invoke((a, b, c) -> h(a,
 * h(b, c)), u, v, w)` and the left-nested mirror. This is what makes the
 * n-ary core a presentation of the classic `<*>` chain rather than a
 * separate operation: a traversal that folds its accumulator through
 * `invoke` relies on exactly this. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto applicative_composition_nary() -> bool {
    auto right_nested = [](const trace &a, const trace &b, const trace &c) {
        return probe_h(a, probe_h(b, c));
    };
    auto left_nested = [](const trace &a, const trace &b, const trace &c) {
        return probe_h(probe_h(a, b), c);
    };
    const auto contexts = SAMPLE::contexts();
    for (const auto &u : contexts) {
        for (const auto &v : contexts) {
            for (const auto &w : contexts) {
                if (!(OBJECT.invoke(probe_h, u, OBJECT.invoke(probe_h, v, w)) ==
                      OBJECT.invoke(right_nested, u, v, w))) {
                    return false;
                }
                if (!(OBJECT.invoke(probe_h, OBJECT.invoke(probe_h, u, v), w) ==
                      OBJECT.invoke(left_nested, u, v, w))) {
                    return false;
                }
            }
        }
    }
    return true;
}

/** basis coherence: where the object has `ap`, `ap(u, v) == invoke(evaluate,
 * u, v)`. Whichever half of the dual basis the instance supplied, the other
 * half is derived, and the two must describe one applicative. Vacuously
 * true for an object without `ap`. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto ap_agrees_with_invoke() -> bool {
    const auto contexts = SAMPLE::contexts();
    for (const auto &first : contexts) {
        const auto functions = OBJECT.invoke(make_prepend{}, first);
        for (const auto &argument : contexts) {
            if constexpr (requires { OBJECT.ap(functions, argument); }) {
                if (!(OBJECT.ap(functions, argument) ==
                      OBJECT.invoke(evaluate{}, functions, argument))) {
                    return false;
                }
            }
        }
    }
    return true;
}

/** derived operations: `map`, `lift` and `zip_with` are each pinned to the
 * basis they are derived from. Each is checked only where the object has
 * it; a monad object grows no `map`. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto derived_applicative_agreement() -> bool {
    if constexpr (requires { OBJECT.lift(probe_x); }) {
        if (!(OBJECT.lift(probe_x) == OBJECT.pure(probe_x))) {
            return false;
        }
    }
    const auto contexts = SAMPLE::contexts();
    for (const auto &first : contexts) {
        if constexpr (requires { OBJECT.map(probe_f, first); }) {
            if (!(OBJECT.map(probe_f, first) ==
                  OBJECT.invoke(probe_f, first))) {
                return false;
            }
        }
        for (const auto &second : contexts) {
            if constexpr (requires {
                              OBJECT.zip_with(probe_h, first, second);
                          }) {
                if (!(OBJECT.zip_with(probe_h, first, second) ==
                      OBJECT.invoke(probe_h, first, second))) {
                    return false;
                }
            }
        }
    }
    return true;
}

// -- Monad laws -----------------------------------------------------------

/** left identity: `bind(pure(x), k) == k(x)`. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_left_identity() -> bool {
    return detail::all_of_tuple(
        detail::arrows_for<OBJECT, SAMPLE>(), [](const auto &arrow) {
            return OBJECT.bind(OBJECT.pure(probe_x), arrow) == arrow(probe_x) &&
                   OBJECT.bind(OBJECT.pure(probe_y), arrow) == arrow(probe_y);
        });
}

/** right identity: `bind(m, pure) == m`. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_right_identity() -> bool {
    auto lift = [](const trace &value) { return OBJECT.pure(value); };
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.bind(context, lift) == context)) {
            return false;
        }
    }
    return true;
}

/** associativity: `bind(bind(m, k), h) == bind(m, a -> bind(k(a), h))`. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_associativity() -> bool {
    const auto arrows = detail::arrows_for<OBJECT, SAMPLE>();
    for (const auto &context : SAMPLE::contexts()) {
        const bool holds = detail::all_of_tuple(arrows, [&](const auto &k) {
            return detail::all_of_tuple(arrows, [&](const auto &h) {
                auto then_h = [&](const trace &a) {
                    return OBJECT.bind(k(a), h);
                };
                return OBJECT.bind(OBJECT.bind(context, k), h) ==
                       OBJECT.bind(context, then_h);
            });
        });
        if (!holds) {
            return false;
        }
    }
    return true;
}

/** grounding: the `fmap` a monad object grows is `bind(m, pure . f)`. This
 * is the theorem docs/transpose/decisions.md#functor-monad-grounding rests
 * on, stated as a law over the object rather than checked once by hand. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_fmap_grounding() -> bool {
    auto pure_after_f = [](const trace &a) { return OBJECT.pure(probe_f(a)); };
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.fmap(probe_f, context) ==
              OBJECT.bind(context, pure_after_f))) {
            return false;
        }
    }
    return true;
}

/** join: `join(pure(m)) == m` and `join(fmap(k, m)) == bind(m, k)`, the two
 * halves of the join/bind pair being one operation. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_join_coherence() -> bool {
    const auto arrows = detail::arrows_for<OBJECT, SAMPLE>();
    for (const auto &context : SAMPLE::contexts()) {
        if (!(OBJECT.join(OBJECT.pure(context)) == context)) {
            return false;
        }
        const bool holds = detail::all_of_tuple(arrows, [&](const auto &k) {
            return OBJECT.join(OBJECT.fmap(k, context)) ==
                   OBJECT.bind(context, k);
        });
        if (!holds) {
            return false;
        }
    }
    return true;
}

/** invoke: the n-ary `invoke` a monad object synthesizes is the nest of
 * binds it is specified as, `invoke(h, u, v) == bind(u, a -> bind(v, b ->
 * pure(h(a, b))))`, with the unary case `invoke(f, u) == fmap(f, u)`. */
template <const auto &OBJECT, instance_sample SAMPLE>
constexpr auto monad_invoke_coherence() -> bool {
    const auto contexts = SAMPLE::contexts();
    for (const auto &u : contexts) {
        if (!(OBJECT.invoke(probe_f, u) == OBJECT.fmap(probe_f, u))) {
            return false;
        }
        for (const auto &v : contexts) {
            auto nested = [&](const trace &a) {
                return OBJECT.bind(v, [&](const trace &b) {
                    return OBJECT.pure(probe_h(a, b));
                });
            };
            if (!(OBJECT.invoke(probe_h, u, v) == OBJECT.bind(u, nested))) {
                return false;
            }
        }
    }
    return true;
}

// -- Agreement between objects over one carrier ---------------------------

/** `FUNCTOR_LIKE.fmap(f, v) == APPLICATIVE.invoke(f, v)`: a hand-written
 * Functor, or the `fmap` a Monad grows, agrees with the registered
 * Applicative's unary `invoke`. The one-off agreement test for
 * `std::optional` in laws.test.cpp, generalized. */
template <const auto &FUNCTOR_LIKE, const auto &APPLICATIVE,
          instance_sample SAMPLE>
constexpr auto fmap_agrees_with_invoke() -> bool {
    for (const auto &context : SAMPLE::contexts()) {
        if (!(FUNCTOR_LIKE.fmap(probe_f, context) ==
              APPLICATIVE.invoke(probe_f, context))) {
            return false;
        }
    }
    return true;
}

/** Two objects over one carrier compute the same `invoke`: the monad object
 * (synthesizing it from `bind`) and the applicative object (native). */
template <const auto &LEFT, const auto &RIGHT, instance_sample SAMPLE>
constexpr auto invoke_agrees() -> bool {
    const auto contexts = SAMPLE::contexts();
    for (const auto &u : contexts) {
        if (!(LEFT.invoke(probe_f, u) == RIGHT.invoke(probe_f, u))) {
            return false;
        }
        for (const auto &v : contexts) {
            if (!(LEFT.invoke(probe_h, u, v) == RIGHT.invoke(probe_h, u, v))) {
                return false;
            }
        }
    }
    return true;
}

// -- Quantification -------------------------------------------------------
//
// One `static_assert` per law, so the diagnostic names the law. The
// per-law checkers above are `constexpr`, not `consteval`, so that an
// instance that is not constant-evaluable can run the same checks at
// runtime under REQUIRE; these aggregates are the compile-time surface.

/** Checks the Functor laws for `OBJECT.fmap` over SAMPLE. Usable as
 * `static_assert(check_functor_laws<functor<C>, Sample>())`. */
template <const auto &OBJECT, instance_sample SAMPLE>
consteval auto check_functor_laws() -> bool {
    static_assert(functor_identity<OBJECT, SAMPLE>(),
                  "functor identity: fmap(identity, v) must equal v.");
    static_assert(functor_composition<OBJECT, SAMPLE>(),
                  "functor composition: fmap(f . g, v) must equal "
                  "fmap(f, fmap(g, v)).");
    return true;
}

/** Checks the Applicative laws for `OBJECT` over SAMPLE, in invoke/pure
 * form, plus the coherence of its derived operations with its basis. */
template <const auto &OBJECT, instance_sample SAMPLE>
consteval auto check_applicative_laws() -> bool {
    static_assert(applicative_identity<OBJECT, SAMPLE>(),
                  "applicative identity: invoke(identity, v) must equal v.");
    static_assert(applicative_homomorphism<OBJECT, SAMPLE>(),
                  "applicative homomorphism: invoke(f, pure(x)) must equal "
                  "pure(f(x)), and invoke(h, pure(x), pure(y)) must equal "
                  "pure(h(x, y)).");
    static_assert(applicative_interchange<OBJECT, SAMPLE>(),
                  "applicative interchange: invoke(evaluate, u, pure(y)) must "
                  "equal invoke(g -> g(y), u) for a context u of callables.");
    static_assert(applicative_interchange_nary<OBJECT, SAMPLE>(),
                  "applicative interchange (n-ary): a pure operand floats "
                  "out, invoke(h, u, pure(y)) must equal "
                  "invoke(a -> h(a, y), u), and symmetrically on the left.");
    static_assert(applicative_composition<OBJECT, SAMPLE>(),
                  "applicative composition: invoke((f, g, x) -> f(g(x)), u, "
                  "v, w) must equal invoke(evaluate, u, invoke(evaluate, v, "
                  "w)) for contexts u, v of callables.");
    static_assert(applicative_composition_nary<OBJECT, SAMPLE>(),
                  "applicative composition (n-ary): invoke(h, u, invoke(h, "
                  "v, w)) must equal invoke((a, b, c) -> h(a, h(b, c)), u, "
                  "v, w), and symmetrically left-nested.");
    static_assert(ap_agrees_with_invoke<OBJECT, SAMPLE>(),
                  "applicative basis coherence: ap(u, v) must equal "
                  "invoke(evaluate, u, v) wherever the object has ap.");
    static_assert(derived_applicative_agreement<OBJECT, SAMPLE>(),
                  "applicative derived operations: map(f, v) must equal "
                  "invoke(f, v), lift(x) must equal pure(x), and "
                  "zip_with(h, u, v) must equal invoke(h, u, v).");
    return true;
}

/** Checks the Monad laws for `OBJECT` over SAMPLE, plus the coherence of
 * the operations it grows -- `fmap`, `join`, `invoke` -- with `bind`. */
template <const auto &OBJECT, instance_sample SAMPLE>
consteval auto check_monad_laws() -> bool {
    static_assert(monad_left_identity<OBJECT, SAMPLE>(),
                  "monad left identity: bind(pure(x), k) must equal k(x).");
    static_assert(monad_right_identity<OBJECT, SAMPLE>(),
                  "monad right identity: bind(m, pure) must equal m.");
    static_assert(monad_associativity<OBJECT, SAMPLE>(),
                  "monad associativity: bind(bind(m, k), h) must equal "
                  "bind(m, a -> bind(k(a), h)).");
    static_assert(monad_fmap_grounding<OBJECT, SAMPLE>(),
                  "monad fmap grounding: fmap(f, m) must equal "
                  "bind(m, pure . f).");
    static_assert(monad_join_coherence<OBJECT, SAMPLE>(),
                  "monad join coherence: join(pure(m)) must equal m, and "
                  "join(fmap(k, m)) must equal bind(m, k).");
    static_assert(monad_invoke_coherence<OBJECT, SAMPLE>(),
                  "monad invoke coherence: invoke(h, u, v) must equal "
                  "bind(u, a -> bind(v, b -> pure(h(a, b)))), and "
                  "invoke(f, u) must equal fmap(f, u).");
    return true;
}

} // namespace beman::transpose::test::laws

#endif // BEMAN_TRANSPOSE_TEST_LAWS_HPP
