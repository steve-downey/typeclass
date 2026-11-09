# Primitive constraints and object conformance

Stage 2 of the [coherence plan](coherence-plan.md), implementing the element
and applicability contracts in [coherence-contracts.md](coherence-contracts.md).
This stage follows correctness PR #44 and contracts PR #45.

## Changes

The five structural Functors (`BinaryTreeF`, `FringeTreeF`, `RoseF`, `ExprF`,
and `NatF`) now compute invocation results from `Fn&`, matching their repeated
invocation of the named callable. They declare callable, holdable-result,
result-construction, and result-move requirements. The three layers carrying a
payload also require that payload to be copyable. Invalid calls are rejected
through constraints at both the instance and free operation object.

Consumer-side `applicative_value` specializations identify the recursive
position as the layer element. Concrete fingertree wrappers expose nested
`value_type` metadata for their payloads. The base tree already supplied it.
The explicit tree algorithm tier remains independent of transpose.

All five fingertree Foldable primitives require `monoid_object` over the
mapped result, independently of their measure monoid. Incorrect identity or
combine result types, invalid callables, and unholdable mapped results reject
softly through direct and free calls. Stale lookup fold diagnostics now use
the registered names `layer_fold` and `project`.

Rope retains Foldable over string chunks and removes its generic Traversable
registration. The replacement traverses `chunks()` and explicitly rebuilds
string results inside the context. Tests cover arbitrary length and unit
results, optional string rebuilding, native collection and pairwise
composition, changing carriers, empty sources, and deferred reconstruction
after the source is destroyed. The [public support guide](../fingertree/typeclass-support.md)
provides migration examples.

## Validation

- GCC 16 C++26 Debug: full build, 570/570 CTest cases, and all 53 standalone
  header compilation steps passed without compiler diagnostics. This includes
  examples, the finite-instantiation compile probe, and explicit-tier isolation.
- GCC 16 C++23: structural conformance suite passed (5 cases, 105 assertions),
  including all five constexpr layer checks.
- Clang 21 with GCC 16's standard library: C++23 structural conformance
  (5 cases, 105 assertions), C++26 fingertree conformance (19 cases, 208
  assertions), and C++26 traversal regressions (16 cases, 279 assertions) passed.
- The public Rope migration example compiles in GCC 16 C++26.
- Changed C++ regions were formatted with clang-format-21; staged diff checks
  passed. No transpose source or generated wording changed.

The worktree-local build is `build/conformance`, configured with
`cmake/gcc-16-toolchain.cmake`, C++26, Debug, benchmarks disabled, and the
existing Catch2 source supplied through `FETCHCONTENT_SOURCE_DIR_CATCH2`.
Builds used at most two compiler jobs:

```sh
cmake --build build/conformance --target all all_verify_interface_header_sets -j 2
ctest --test-dir build/conformance --output-on-failure -j 2
```

Direct compiler checks used `-O0 -g0`, the worktree and generated include
paths, and its built Catch2 libraries. Clang used
`--gcc-toolchain=/home/sdowney/install/gcc-16` and `-no-pie` when linking those
libraries. Benchmarks, opt-in P2300 evidence, and module configurations were
not exercised in this stage.

The conformance suites check all five structural Functors, all five
fingertree Foldables, and all four remaining generic fingertree Traversables.
Full-object checks use copyable integer and string representatives; strings
also exercise canonical Monoid-derived `combine_all` and `fold`. Layer tests
retain constexpr coverage and compare identity/composition and ordered child
mapping. A move-only base-tree case verifies reference-reading folds and
soft rejection of copying collection/search operations. No change to
transpose's deep concepts was needed.

## Remaining scope

Stage 3 supplies configured reconstruction, fresh-result movement, payload
Functor instances for fingertree, and concatenation monoids. Existing generic
fingertree traversal still rebuilds default-configured result trees; this
stage's conformance checks do not establish configuration preservation.
Stages 4 and 5 supply the payload tree instances and optional structural
fingertree integration. Stage 6 closes packaging and the integrated matrix.
The [plan](coherence-plan.md) retains their gates.
