# Comparison: beman.transpose against libfn

Surveyed 2026-10-30 against `libfn/functional` main (`de37db3`, the 0.2 development
cycle) and `beman.typeclass` main (`5d248b9`). libfn is Bronek Kozicki's
header-only C++20 library: four carriers (`expected`, `optional`, `just`,
`choice`), a canonical sum `copack`, a flat product `pack`, pipeline verbs, and
a graded `expected` whose grade is a `copack` of error types. Its design
document is `TYPE_ALGEBRA.md` in that repository.

**Read this as a record.** The questions it raised are slugs in
[decisions.md](../decisions.md), and the work to close them is
[transpose-surface-plan.md](../transpose-surface-plan.md). This note is the
evidence those entries cite, so it is not revised when they are answered.

---

## 1. Shared core

Both libraries arrive at the same graded-monad shape for `expected`
independently; [empty-grade-spelling](../decisions.md#empty-grade-spelling)
already records libfn as a known divergence. The common ground:

- A canonical type set (libfn `copack_for`, transpose `error_set`) that sorts
  and deduplicates, with the same interim ordering (the type's spelling read
  out of the compiler's function signature) and the same collision
  `static_assert`.
- Join is set union, bottom is the empty set, widening is subset inclusion,
  narrowing is only by handling.
- Lazy promotion: an ungraded operand joins as the bottom and leaves no trace.
- Mixed-error composition of two expecteds is well-formed and deduces the
  union.
- SFINAE-friendly bases so a probe fails cleanly, and laws asserted at compile
  time in a test harness rather than in a `requires` clause.
- Refusal of catch-all arms in an overload set: libfn documents that an
  unconstrained `[](auto)` defeats exhaustiveness and recommends a
  concept-constrained arm; transpose's `overloaded` makes a generic arm
  ambiguous with its `consteval` catch-all.

## 2. The split

| | typeclass / transpose | functional / libfn |
|---|---|---|
| Unit of abstraction | typeclass objects over any context (open set) | four carriers (closed set: `some_monadic_type` is a fixed disjunction) |
| Extension direction | new carriers; verbs fixed per typeclass | new verbs (nested `apply`); carriers fixed |
| Grade lives on | error side only | error side and value side |
| Empty grade | bare `T` | `expected<T, copack<>>` and `just<T>` |
| Value of a grade | witnessed subset, several witnesses | one-of, exactly one alternative |
| Accumulation | second applicative object, no `bind` | none; `&` keeps the leftmost failure |
| Narrowing | `recover<H...>`, annotated and checked | exhaustive handler; result grade is the join of the arms |
| Elimination | partial `visit`, per-type `witness<E>()` | `apply`, `apply_type`, exhaustive by overload set |
| Disjunction | none | `\|` and `disjoin`; an infallible operand annihilates the error side |
| Structure traversal | `traverse`, `transpose`, Foldable | none; fixed-arity `&` and `conjoin` only |
| `void` payload | refused (`holdable_value`) | `void` on a plain side, `pack<>` as the unit factor in a sum or product |
| Cross-context moves | ill-formed by constraint ([cross-model-mixing](../decisions.md#cross-model-mixing)) | licensed crossings: identity cluster, failure-path bridging |
| `noexcept` | none | computed on every operation and propagated by the pipeline |
| Laws checked | grade algebra in full; functor identity for one object | functor identity and monad left identity on values |
| Type-order modes | one interim order | two orders, two inline ABI namespaces |

## 3. What transpose should take, ranked

1. **A unit, and `void`-returning effectful functions.** transpose refuses a
   `void` result outright, so `traverse(f, xs)` with `f : T -> expected<void,
   E>` has no meaning, and neither does a context whose value type is `void`.
   That is the validation-pass and IO shape. libfn's rules: `void` stays
   `void` on a plain side, and enters a sum or product as the nullary product
   `pack<>`, which is the unit. transpose needs the decision, not the type.
   Filed: [unit-payload-spelling](../decisions.md#unit-payload-spelling).

2. **Generic code stays closed under every grade, including the empty one.**
   libfn's `or_else` on an uninhabited error side is vacuously well-formed and
   asks nothing of its handler, so a recovery step does not become ill-formed
   because an upstream stage proved failure impossible. transpose's bare-`T`
   empty grade has the cost that `recover` on the empty fiber is ill-formed.
   Filed:
   [generic-closure-at-empty-grade](../decisions.md#generic-closure-at-empty-grade),
   paired with [uniform-form-surface](../decisions.md#uniform-form-surface).

3. **Licensed crossings instead of "cross-model mixing is ill-formed".** libfn
   states when a value may move between contexts as a rule about information:
   an infallible context may enter any fallible one on the success path, and
   two fallible contexts may bridge only on the failure path through a
   handler that names the target. That is the answer
   [optional-grade-model](../decisions.md#optional-grade-model) is circling.
   Filed:
   [licensed-context-crossings](../decisions.md#licensed-context-crossings).

4. **An exhaustive eliminator for `error_set`.**
   [multi-witness-elimination](../decisions.md#multi-witness-elimination)
   refused a public verb until a second client needs one. libfn's history says
   the client arrives at once: `apply` with an overload set is its most used
   operation, and exhaustive `transform_error` over a graded side is how users
   collapse a grade. The witnessed-subset representation makes this a fold,
   not a visit, which `recover` already does internally. `apply_type`, which
   hands the injection tag to the arm, is worth taking too. Filed as a
   challenge in that slug's log and as
   [heterogeneous-arm-elimination](../decisions.md#heterogeneous-arm-elimination).

5. **Name the operations that enter the failure state.** transpose has
   `pure<C>(v)` and `grade_subsume` to widen, and nothing generic that injects
   an error. libfn spells this with `fail` and `unexpected{copack{e}}`. The
   planned `zero<C>()` for Alternative has the same shape. Filed:
   [failure-injection-spelling](../decisions.md#failure-injection-spelling).

6. **Disjunction with annihilation, as the specification for Alternative.**
   libfn's `|`: values sum into the canonical set, errors multiply into a
   product, leftmost success wins, and an infallible operand annihilates the
   error channel so the disjunction becomes total. Borrow the algebra, not
   the operator. Filed:
   [alternative-disjunction-semantics](../decisions.md#alternative-disjunction-semantics).

7. **The canonical type set is a datum, not an error thing.** Its machinery
   sits in `error_set.hpp` as detail, under the model, although it is the
   reusable half. Filed: [canonical-set-home](../decisions.md#canonical-set-home).

8. **Laws per instance, on values.** `laws.hpp` checks the grade semilattice
   fully and functor identity for one object. libfn asserts functor identity
   and monad left identity as compile-time equalities on real values. The
   Applicative laws and the remaining Monad laws belong in the harness. Work
   item in the plan, stage
   [instance-laws](../transpose-surface-plan.md#instance-laws).

9. **Computed `noexcept` and `[[nodiscard]]`.** transpose headers carry no
   `noexcept`. libfn computes it on every verb and tests that the pipeline
   propagates the verb's own answer rather than promising `noexcept` and
   terminating. Filed: [operation-noexcept](../decisions.md#operation-noexcept).

10. **The type-order ABI hazard.** libfn's interim name ordering and
    `std::type_order` disagree, so a copack's layout differs by mode, and each
    mode lives in its own inline ABI namespace so mixing fails to link rather
    than violating ODR. `error_set` stores witnesses in canonical order, so the
    same hazard arrives with P2830. Filed:
    [type-order-abi](../decisions.md#type-order-abi).

One residue to record rather than borrow: libfn grades only on opt-in, and
rejects two plain expecteds with different error types unless one already
carries a `copack`. transpose joins them implicitly. Logged under
[grading-footprint](../decisions.md#grading-footprint).

## 4. Why not grade the value side

libfn carries the same canonical set on the success side (`copack` as a
value, `choice` as the identity carrier over it). The argument for it: when
the computation is a classifier rather than a function, success is one of
several kinds and each kind wants its own continuation. Parsers, decoders and
command dispatch have that shape. What the user gets: per-alternative
continuation with the branch results unified automatically; disjunction over
alternatives of different type; recovery into a value of a type the happy
path never produces; a lawful identity carrier over a sum.

Why it is weaker than the error-side argument:

1. Errors are handled alike and values are not. Errors from many sources
   arrive in accidental order and meet one handler at a boundary, so set
   semantics fit. Values meet continuations that differ per kind, so the sum
   is never the end of the story and multiplies at every branch.
2. Idempotence is right for errors and often wrong for values. Two sources
   raising the same error type raise the same kind of error. Two alternatives
   of the same value type can mean different things by position. libfn admits
   this and restricts value sums to distinct tag types, while keeping `pack`
   positional for exactly the reason products of values carry position.
3. Distribution produces types nobody asked for: conjoining a choice with
   anything expands to a sum of products.

The pressure in this tree is a fold algebra over a variant-shaped layer in
`tree_algorithms`, where the arms return different types and the user unifies
by hand. That is an eliminator problem, not a grade problem; the narrow
borrow is an overload-set elimination whose result type is the join of the
arms. Filed: [value-side-grading](../decisions.md#value-side-grading) and
[heterogeneous-arm-elimination](../decisions.md#heterogeneous-arm-elimination).

## 5. What libfn lacks

libfn has no user-supplied contexts, no traversal over a structure, no
accumulating composition, no multi-witness evidence, and a grade algebra
hardwired to one model. It ships its own `expected` and `optional` rather than
working on the standard ones. Each of those is what transpose exists to
provide.

Not worth borrowing: the identity carrier spelling (`just`, `expected<T,
copack<>>`), already a recorded divergence; the member-plus-pipeline dual
surface, a closed-library convenience; the `pfn` polyfill layer; reference
alternatives in sums, which `std::expected` forecloses.
