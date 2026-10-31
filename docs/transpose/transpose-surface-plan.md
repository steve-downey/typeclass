# The edges of the context surface in beman.transpose — closure plan

Status: DRAFT for review by Steve before any execution
Audience: a Claude Code agent executing concrete work in `beman.transpose`
(P3200). This document is the contract between the plan and the agent: **the
Why sections are load-bearing.**

Companion: [decisions.md](decisions.md), the decision log, and the evidence
this plan closes out, [review/libfn-comparison.md](review/libfn-comparison.md).
Every question below is a slug in the log, named for the question; every
reference to one is a link. Stages carry outline numbers *and* slugs;
cross-reference by slug, since numbers shift when a stage splits.

The divergence protocol of
[transpose-grading-plan.md §0](transpose-grading-plan.md#divergence-protocol)
applies verbatim: a false Why stops the work; a false What with a true Why
gets a proposed adaptation and waits; every divergence is a dated Log entry
under the implicated slug; tripwires are sensors, never obstacles.

## 1. Context {#context}

The grading plan built the graded `expected` and proved the grade algebra
generic over a second model. A comparison against libfn, which arrived at the
same graded-monad shape from a closed family of carriers, showed where the
*edges* of transpose's context surface are undecided rather than decided:
what a context with no value is, what generic code may do at the empty grade,
how a value moves between contexts of different models, how a canonical sum
is eliminated, and how failure is entered generically. None of these is a
change to the grading design. Each is a question the grading design left
standing because nothing in the coordinated set had asked it yet.

The questions, each OPEN in the log as of 2026-10-30:

- [unit-payload-spelling](decisions.md#unit-payload-spelling)
- [generic-closure-at-empty-grade](decisions.md#generic-closure-at-empty-grade),
  paired with the older [uniform-form-surface](decisions.md#uniform-form-surface)
- [licensed-context-crossings](decisions.md#licensed-context-crossings)
- [heterogeneous-arm-elimination](decisions.md#heterogeneous-arm-elimination),
  with a challenge logged under
  [multi-witness-elimination](decisions.md#multi-witness-elimination)
- [value-side-grading](decisions.md#value-side-grading)
- [failure-injection-spelling](decisions.md#failure-injection-spelling)
- [alternative-disjunction-semantics](decisions.md#alternative-disjunction-semantics)
- [canonical-set-home](decisions.md#canonical-set-home)
- [operation-noexcept](decisions.md#operation-noexcept)
- [type-order-abi](decisions.md#type-order-abi)

## 2. Standing decisions this plan must not weaken {#standing-decisions}

- [empty-grade-spelling](decisions.md#empty-grade-spelling): ∅ is bare `T`.
  Nothing here reopens that. Stage [unit-and-closure](#unit-and-closure) adds
  a reachable uniform spelling; it never makes the framework materialize one.
- [grading-footprint](decisions.md#grading-footprint): grading claims only
  previously-ill-formed territory. The goldens in
  `baseline_deduction.test.cpp` stay green through every stage.
- [cross-model-mixing](decisions.md#cross-model-mixing): no product of
  models, no mixing of models at a mixing point. Crossings are a different
  operation and must be spelled so that this constraint is visibly intact.
- [grade-generality](decisions.md#grade-generality): the framework speaks
  grade vocabulary only. Moving the canonical set out of the model must not
  move error vocabulary with it.
- [applicative-objects](decisions.md#applicative-objects): bind belongs to the
  short-circuit object only; left-to-right order is normative.
- [recover-grade-inference](decisions.md#recover-grade-inference): grades
  narrowed by handling are annotated and checked. An exhaustive eliminator
  narrows to ∅ or to a value; it is not a second inference path.

## 3. Grouping into pull requests {#grouping}

Not one PR per slug. The groups below share a mechanism or a ruling, and
splitting them would leave a PR that cannot be tested on its own.

| PR | Stages | Why together |
|---|---|---|
| A | [unit-and-closure](#unit-and-closure) | the unit and the uniform spelling are the two degenerate corners of one carrier, and the vacuity rule for error-side operations is stated once for both |
| B | [set-home-and-abi](#set-home-and-abi) | moving the set and adding its ABI namespace touch the same declarations once |
| C | [set-elimination](#set-elimination) | the `error_set` eliminator and the heterogeneous-arm visitor are one join-of-arms mechanism with two clients |
| D | [instance-laws](#instance-laws) | test-only; independent of every other PR |
| E | [context-crossings](#context-crossings) | a ruling and a test-only demonstration; no shipped surface |
| F | [failure-side](#failure-side) | `raise` and `zero` are decided under one rule, and Alternative's disjunction depends on the value-side ruling |
| G | [noexcept-sweep](#noexcept-sweep) | touches every header and the wording generator; isolating it keeps the diff reviewable |
| H | [surface-paper-revision](#surface-paper-revision) | last, after the rulings |

Order of execution: A, then B, then C (C depends on B's set home). D any time.
E after A (the uniform spelling is one end of a crossing). F after C and E (it
needs the eliminator for its tests and the crossing rule for `optional`'s
`zero`). G after F (so the sweep covers the new operations). H last.

## 4. Work plan {#work-plan}

### Stage 1 — [unit-and-closure](#unit-and-closure) {#unit-and-closure}
Closes [unit-payload-spelling](decisions.md#unit-payload-spelling),
[generic-closure-at-empty-grade](decisions.md#generic-closure-at-empty-grade),
[uniform-form-surface](decisions.md#uniform-form-surface).
Deliverables:
- A ruling on the unit type (own type, or `std::monostate`) and on how a
  `void`-valued context reports its element type.
- `std::expected<void, E>` registered as an applicative and monad context, at
  the same grade reading as the valued form.
- `traverse` with a `void`-returning effect: either the structure of units or
  a `traverse_` shape yielding `C<unit>`, per the ruling; the ruling says
  which, or both.
- The uniform spelling `std::expected<T, error_set<>>` reachable from generic
  code by an explicit operation (the surface is the ruling for
  uniform-form-surface), and `recover` vacuous on it: handler neither invoked
  nor instantiated, result is the operand.
- Tests: `traverse` over `vector<int>` with `f : int -> expected<void, E>`
  deduces the ruled shape; a generic function calling `recover<E>` compiles
  when instantiated at the ∅ fiber through the uniform spelling; the goldens
  are untouched.
Why: these are the two corners a reviewer reaches first. A library whose
flagship verb cannot take the most common fallible function, and whose
recovery verb is ill-formed exactly where failure was proved impossible, is
read as incomplete at the edges rather than small by design.
Acceptance: goldens green; new goldens for the `void` shapes; a negative test
that the framework never materializes `expected<T, error_set<>>` without the
user writing it (the empty-grade-spelling sentinel, re-asserted).
Tripwire: if making `expected<void, E>` a context requires `holdable_value`
to admit `void` for every instance rather than for the unit, STOP. The unit
is a value; `void` is not, and a `void`-holding `optional` must stay
ill-formed.

### Stage 2 — [set-home-and-abi](#set-home-and-abi) {#set-home-and-abi}
Closes [canonical-set-home](decisions.md#canonical-set-home),
[type-order-abi](decisions.md#type-order-abi).
Deliverables:
- The canonical set machinery (`type_list`, sort, dedupe, `canonical_t`, the
  name order and its collision check) in its own header beside `grade.hpp`,
  with no error vocabulary in it; `error_set.hpp` includes it and loses the
  copies.
- An inline namespace on the order-dependent types, keyed on the order in use
  (today only the interim name order; the key exists so a P2830 order is a
  second value, not a layout change in place).
- Whether the set header is public or `detail` is the ruling; the plan
  proposes `detail` until stage [set-elimination](#set-elimination) gives it a
  second client, then public.
Why: [grade-generality](decisions.md#grade-generality) says the framework is
free of error vocabulary, and the set is the half with none. libfn's history
with two orders in one program is the ABI argument; adopting the namespace
before there is a second order makes that day a non-event.
Acceptance: `error_set.hpp` compiles with the set header included and nothing
else changed; the law harness still includes `grade.hpp` and nothing else; a
compile-fail test that two translation units cannot mix order keys.
Tripwire: an `error_set` spelling in any deduced type changing because of the
move → STOP; this stage is a relocation.

### Stage 3 — [set-elimination](#set-elimination) {#set-elimination}
Closes [heterogeneous-arm-elimination](decisions.md#heterogeneous-arm-elimination);
rules on the 2026-10-30 challenge under
[multi-witness-elimination](decisions.md#multi-witness-elimination).
Read that slug and [accumulation-evidence](decisions.md#accumulation-evidence)
before starting. The ruling has two possible outcomes, and the stage must be
written for whichever Steve chooses:
- *Fence holds.* No public eliminator for `error_set`; the heterogeneous-arm
  mechanism ships for sums only (`std::variant` and the variant-shaped layer
  in `tree_algorithms`), beside `overloaded`.
- *Fence moves.* Two `error_set` verbs, per the 2026-10-31 entry under
  that slug: a `fold` over present witnesses with an explicit accumulator
  and arms exhaustive against the grade, and a `collapse` to one error type
  with the left-biased multiplicity rule of `combined_with`. `visit` stays
  partial. Neither verb needs the join-of-arms type.
Deliverables (both outcomes): the join-of-arms result-type computation for
one-of sums, deduplicating and collapsing to the common type when the arms
agree, built on the canonical set from stage
[set-home-and-abi](#set-home-and-abi); an `apply_type`-style form that passes
the injection tag to the arm; tests with heterogeneous arms over a
`std::variant` and over a `tree_algorithms` layer.
Deliverables (fence moves): `fold` and `collapse`, with `recover`
re-expressed through `fold` or left as is per the ruling, a negative test that
a missing arm is a compile error naming the alternative, and a positive test
that two present witnesses reach the accumulator in canonical order.
Deliverables (fence holds): a test pinning the boundary-collapse idiom through
`recover` with a raising handler, and a paragraph in the paper's
implementation-experience section stating the per-witness read as the cost of
the witnessed-subset representation.
Why: `std::visit` requiring one result type is the pain this mechanism
removes, and it is the same pain in both clients. Writing it twice, once
beside `overloaded` and once inside `error_set`, is the drift
[functor-monad-grounding](decisions.md#functor-monad-grounding) warns about.
Acceptance: heterogeneous arms compile and deduce the documented type; the
catch-all ambiguity of `overloaded` still fires; goldens green.
Tripwire: deduplicating the arms' result types is expected and is not a
value-side grade. If any operation object, `grade_of`, or `rebind_grade`
has to *read* the join-of-arms result as a grade to make the tests pass, or
if `fmap` over it wants to distribute per alternative, STOP and reread
[value-side-grading](decisions.md#value-side-grading): that is the reading
sub-question, and the proposed answer is no.

### Stage 4 — [instance-laws](#instance-laws) {#instance-laws}
Test deliverable; closes no slug. Extends `tests/beman/transpose/laws.hpp`.
Deliverables:
- Applicative laws (identity, homomorphism, interchange, composition) and
  Monad laws (left identity, right identity, associativity) as `consteval`
  checkers quantified over an object and a value sample, in the style of
  `check_graded_laws`.
- Run against every registered instance that is constexpr-clean (`optional`,
  `expected` both objects, `array`, `zip_list`) and the test `Identity`.
- Functor composition added beside the existing identity check.
Why: `applicative_object` is syntactic conformance; this is the semantic half,
and it generalizes the one-off agreement test under
[functor-monad-grounding](decisions.md#functor-monad-grounding). libfn checks
functor identity and monad left identity on values; the harness here already
has the shape to do all of them.
Acceptance: harness green on every listed instance; a deliberately lawless
test instance fails the named law with the law's message, not
`static_assert(false)`.
Tripwire: if a law requires `noexcept` or a copy the operation's
specification does not ask for, STOP; that is the probe-category defect
[operand-value-category](decisions.md#operand-value-category) already fixed
once.

### Stage 5 — [context-crossings](#context-crossings) {#context-crossings}
Closes [licensed-context-crossings](decisions.md#licensed-context-crossings);
advances [optional-grade-model](decisions.md#optional-grade-model).
Deliverables:
- The rule, as a decision: a crossing is licensed when it loses no
  information, or when it is on the failure path through a handler that
  supplies what the target needs. Spelled against models, not carriers.
- A test-only demonstration: the two-point model's `Fallible<T>` lifted into
  the error-set model's `expected<T, error_set<E>>` through a handler
  supplying `E` for the empty state; the reverse direction shown to be
  refused on the success path and licensed on the failure path.
- No shipped operation. The slug records what a future `into<C>` or similar
  would have to satisfy.
Why: [cross-model-mixing](decisions.md#cross-model-mixing) is correct and is
about mixing points; without a stated crossing rule every future question
about `optional` and `expected` together re-derives it. libfn's identity
cluster is the evidence that the rule is statable in a page.
Acceptance: the demonstration compiles; a negative test that a lossy crossing
on the success path does not; cross-model-mixing's tests unchanged.
Tripwire: pressure to register `std::optional` as graded to make the
demonstration work → STOP; that is optional-grade-model's own question and
must be answered there, not here as a side effect.

### Stage 6 — [failure-side](#failure-side) {#failure-side}
Closes [failure-injection-spelling](decisions.md#failure-injection-spelling),
[alternative-disjunction-semantics](decisions.md#alternative-disjunction-semantics),
[value-side-grading](decisions.md#value-side-grading).
Deliverables:
- Ruling on value-side-grading first; the proposed answer is no, and the rest
  of this stage assumes it.
- `raise<C>(e)`: an operation object parameterized on the context, result at
  the join of `C`'s grade with the singleton of `e`'s type; the ungraded
  `expected<T, E>` case accepts an `E` and deduces `expected<T, E>`.
- `zero<C>()` decided under the same rule and spelled alongside, even if
  Alternative's `alt` is not yet shipped.
- The Alternative disjunction semantics as a decision and, if Steve wants the
  typeclass now, `derive_alternative<Impl>` with `zero` + `alt` as basis,
  `first_monoid` re-expressed through the `optional` instance, and the
  annihilation rule tested: an infallible operand makes the result total.
Why: `pure` without its dual leaves generic graded code unable to fail
without naming the carrier, which is the one thing generic code cannot do.
Deciding `raise` and `zero` together keeps the "builds a context, so names
it" rule in one place.
Acceptance: `raise` deduces the documented grades; the goldens are untouched;
if Alternative ships, its laws are added to the stage
[instance-laws](#instance-laws) harness in the same PR.
Tripwire: if `alt`'s value side wants a canonical set to type-check its own
tests, STOP; that contradicts the value-side-grading ruling this stage began
with.

### Stage 7 — [noexcept-sweep](#noexcept-sweep) {#noexcept-sweep}
Closes [operation-noexcept](decisions.md#operation-noexcept).
Deliverables:
- Conditional `noexcept` on every basis operation of every registered
  instance and on every derived operation of the CRTP bases, computed from
  the callable and the carrier's constructors; the deduced-return members
  compute it from the probe type their constraint already names.
- `[[nodiscard]]` on every operation object and derived member returning a
  context.
- Wording generator carries both; `make wording-check` green.
- Tests: a throwing callable's `noexcept(false)` is visible through
  `traverse`; a non-throwing one over `optional<int>` is `noexcept(true)`.
Why: LEWG asks on every proposal; libfn's test that the pipeline propagates
the verb's own answer rather than promising `noexcept` and terminating is the
shape to copy.
Acceptance: no header without `noexcept` on its public operations; wording
check green.
Tripwire: a `noexcept` that cannot be computed without instantiating a
deduced body → STOP; that is a basis that is not SFINAE-friendly, which
`apply.hpp`'s invariant forbids, and the fix is upstream of this stage.

### Stage 8 — [surface-paper-revision](#surface-paper-revision) {#surface-paper-revision}
Deliverables: P3200 rationale for the unit and the `void` effect; the
closure-at-∅ argument and the uniform spelling; the crossing rule as a
sentence in the compatibility section; the eliminator, if shipped, beside
`overloaded`; `raise` beside `pure`; deliberate omissions updated (value-side
grading, with the Tolstoy argument in one paragraph; Alternative if deferred).
Why: deliberate omissions with recorded reasons survive review from both
directions.

## 5. Open questions {#open-questions}

Tracked as OPEN entries in the decision log, same namespace as decisions:
every slug in §1, plus
[datum-entry-point](decisions.md#datum-entry-point), unchanged by this plan.
