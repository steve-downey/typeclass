# beman.typeclass — design and infrastructure review, 2026-10-13

Repository at `71129ec` (the day after reunification). Read-only review; nothing in the tree was changed.
Five scoped reports back this summary and carry the file:line evidence; every claim repeated here was
spot-checked against the source by the lead before inclusion, and the two bugs were confirmed by reading the
lines. "Verified" below means a compile/run probe or a direct read; "inferred" is marked.

| Report | Scope |
|---|---|
| `01-transpose-mechanism.md` | the typeclass mechanism on its own terms; repo vs paper surface |
| `02-fingertree-consumer.md` | fingertree as consumer — the design resistance it never offered |
| `03-tree-algorithms.md` | tree_algorithms as consumer; the two tiers; fold_map; pattern matching |
| `04-infrastructure.md` | build, CI, install, modules, paper and API-doc tooling |
| `05-docs-corpus.md` | every doc classified; orphan decisions; the slug plan; the post-EOM map |
| `06-research-organization.md` | the typeclass case: audiences, literature tiers, the comparison paper, pattern matching |

---

## 0. The hypothesis, tested

> "Because they were separated, there was no design resistance from fingertree, so the costs or tradeoffs may
> not have been apparent. Or I guessed wrong."

**Half right, and the half that is wrong is instructive.**

- **Monoid: fingertree *did* resist, through the decision log, and won.** `monoid-selection`
  (`docs/transpose/decisions.md:2964-3172`) is argued from six fingertree costs; `af05316` is fingertree
  accepting it. Instance-keyed nodes, `tuple_monoid` for the priority queue, `dual_monoid` for `reversed()`,
  stateful instances, non-consuming traversal for persistent structures (`consumes_rvalue_structure`) — all
  fit and are tested. The guess was right there, and it was right *because* you looked at fingertree.
- **Functor / Foldable / Traversable: nobody resisted, and that is where the mechanism fits poorly.** The
  fingertree instances were mechanical ports (`flatten()` + `from_sequence()`), kept unchanged at the merge
  ("touches no paper text", `machinery-deduplication.md:56`). The consequences are real and unrecorded (§2).
- **tree_algorithms hits the same wall from the other side**, and additionally carries a second `fold_map`
  whose shape contradicts the monoid vocabulary the coherence skill freezes (§3).

One root cause explains most of the consumer-side friction: **transpose's conformance vocabulary and derived
operations are keyed on conventions from its own domains** — a nested `value_type` (`applicative_value_t`,
`detail/typeclass_base.hpp:79-100`) or an `Impl::element_type`, and a "same constructor, different element"
relation that `optional`/`vector`/`expected` get for free and neither a measured sequence nor a base-functor
layer has. Fix that once and both consumers get simpler.

---

## 1. Blocking findings (decide before any paper text is written)

**B1. fingertree's `traverse` throws the measure, the monoid instance and the allocator away.**
`finger_tree_traversable.hpp:58-62` rebuilds `FingerTree<U>::from_sequence(values)` — default count measure,
`MONOID{}`, `std::allocator` — while the header claims "measure semantics are preserved" (`:17`) and registers
"for all monoid and measure combinations" (`:77-80`). A `pmr::FingerTree<int, max_monoid<…>>` traversed through
`beman::transpose::traverse` comes back a non-PMR count tree, silently. The same for all four wrappers.
There is no `functor<FingerTree<…>>` at all. *Verified.*
Root cause is structural, not a typo: Traversable/Functor index a type constructor `F<_>`, and
`FingerTree<_, MONOID, MEASURE_POLICY, ALLOC>` is one only when the measure is fixed per element type —
exactly Haskell's `Data.Sequence` (Traversable, count-only) vs `Data.FingerTree.traverse'` (measure-changing,
not Traversable) split. fingertree's registration is the union of the two and resolves the mismatch by
discarding the measure.
**Recommendation:** register Traversable (and a Functor) only for the count slice, reconstructing with the
*same* allocator and instance; offer a separate, explicit `traverse_measured<…>` for the general tree;
strike the "Preservation" bullet; record a transpose slug for "Traversable for a structure whose shape depends
on the element type" (tree_algorithms' `FringeTree` is the second consumer to hit it).

**B2. Two `fold_map`s with incompatible shapes in one `beman::` surface.**
transpose: `fold_map(f, structure, monoid_instance)` — operation object, monoid as an object with
`identity()`/`combine()` (`fold.hpp:209-250, 683-711`). tree_algorithms: `fold_map<Result>(map_fn, combine,
identity, …, tree)` — function template, monoid as two positional callables, structure last
(`fold_map.hpp:55-60, 79-85`). Same semantics; different argument order, different monoid passing. The skill
says `identity()`/`combine()` is "used by fingertree and anything new" — tree_algorithms' primary spelling
is a second monoid vocabulary, and Paper D's text says "no Monoid trait anywhere in the primary API". *Verified.*
**Recommendation:** the explicit tier takes a duck-typed monoid object (`m.identity()`, `m.combine(a,b)`) —
zero transpose dependency, so `explicit-tier-isolation` holds — and the two spellings then differ only in
argument order, which gets one ruling. Then answer the open `layer-fold-and-foldable` question by
registering `foldable<Fix<F>>` / `foldable<BinaryTree<T>>` in the lookup tier so transpose's ten derived
queries come free and the example-local Foldables disappear. (`Result` is in fact deducible from `identity`;
the "must be given explicitly" comments are wrong — keep the habit, fix the reason.)

**B3. The research framing names the wrong mechanism.**
Instances are resolved by **partial-specialization matching and partial ordering of a variable template**,
with constraint subsumption as the tie-breaker among equally specialized candidates; overload resolution
appears only one level down (inside an Impl's own overload set, inside `overloaded` algebras, inside
operation-object `operator()` sets). *Verified by probe:* concrete beats constrained-generic; two constrained
generics give "ambiguous partial specializations" surfaced as a substitution failure. No decision slug records
the registration mechanism itself — the only rationale is an `.org` explainer. And **nothing in the codebase
exercises the overlap case**: every instance is a specialization on a concrete class template; the section
named `[transpose.range]` is vector-only. Frame the comparison paper around partial ordering, record the
mechanism under a slug, and build the overlap experiments (§6).

**B4. 31 of your rulings exist only in files the end-of-month rule deletes.** The `ap`-basis/`invoke`-interface
ruling has no transpose slug (only SYNC entries 3/5, a worklist, `provenance.md`); the 2026-07-20
typeclass-instance contract lives only in `essay/CRITIQUE.md` §14–§16; the reintegration decision, layout and
tag-prefixing only in `SYNC.md`; `fold_left`-is-Foldable's only in `STATUS.md`. Full table with sources and
proposed slugs: `05-docs-corpus.md` §3. Migrate before deleting anything.

---

## 2. The mechanism (transpose) — significant findings

Ranked. Each has a recommendation in `01-transpose-mechanism.md`.

- **M1. Superclass story contradicted by code.** `derive_applicative` provides `map`/`lift`; `derive_functor`
  provides `fmap`; `std::array` and `std::expected` have *no* `fmap` (`functor<array>` is unregistered); a
  monad object fails both `functor_object` and `applicative_object`. Yet `functor.hpp:140-144`, D3200R0:396-400
  and the operation-objects post say Applicative "provides the Functor names" and "`map` and `lift` are spelled
  `fmap` and `pure`; an alias doesn't need a second name". *Verified.* Pick: rename `map`→`fmap` and make
  `derive_applicative` satisfy `functor_object`, or give `functor<T>` a constrained fallback specialization
  that derives from a registered applicative. Fix the sentence either way.
- **M2. Registered stateful monoids are permitted on paper and ignored in code.** `registered-instance-state`
  says `monoid<T>` is "one object … shared by every fold that defaults to it" and defends instrumentation
  counters; every defaulted path constructs `Monoid<R>{}` fresh (`fold.hpp:267, 279, 588, 599`;
  `induced_monoid.hpp:108`). A probe with a counter in a registered instance saw zero combines. *Verified by
  execution.* Either the defaulted paths read `monoid<R>` or the allowance is withdrawn for *registered*
  instances (keep it for passed ones). Also: `Monoid<T>` is a class-template registry needing default
  construction, while every other typeclass is a variable with a `false_type` sentinel — the odd one out.
- **M3. One element-type oracle, misnamed, two conventions.** `applicative_value_t` = `T::value_type` drives
  `functor_object`, `foldable_object`, the derived `to_vector`/`find_first`; Traversable/Foldable Impls need
  `Impl::element_type` instead. Base-functor layers have neither and should not; fingertree wrappers lack
  `value_type`, so eight of their ten registered instances cannot even be asked the conformance concepts.
  *Verified.* One hook, named `element_t`, customizable, with a fallback chain.
- **M4. Keying on the complete type, no rebind.** `functor<optional<int>>` where Haskell keys `Maybe`; the
  "same constructor, different element" relation — Functor's whole content — is represented nowhere, so
  there is no rebinding hook for fingertree (B1) and tree_algorithms carries two keys (`Fix<F>` on the
  unapplied template, `functor<F<A>>` on the applied type) plus binder aliases and a deduction trait.
  This is the HKT cost and it belongs in Paper D's Costs section and the comparison paper.
- **M5. Coherence is ODR-grade; every instance is an orphan.** Registration reopens `beman::transpose`;
  use-before-specialize is a hard error in one TU and silent across TUs (*verified / inferred*). transpose
  registers the `std::vector` instances itself, so no downstream can ever have a different one — and the
  library's own escape (`accumulating_applicative`, a second registry) does not generalize. D3200R0 should
  state who may specialize `applicative<T>` for which `T` ([namespace.std]-style) and record the cross-TU
  hazard. Modules make it sharper: a consumer specializing `functor<MyT>` inside its own named module is
  specializing a template attached to another module — unanswered anywhere.
- **M6. Allocator blindness.** `vector<T, pmr>` has no instance (*verified*); fold/traverse result containers
  are `std::allocator` only. Fingertree owns the PMR story and the typeclass path breaks it.
- **M7. Grade machinery inside the proposed Applicative core.** `apply.hpp` includes `grade.hpp`;
  `derive_applicative::subsume`, a `graded_context` conjunct in `applicative_object`, `accumulating_applicative`
  in the core header. Every container/tree Applicative carries error-grading surface and wording. Move
  `subsume` to the graded carriers; drop the conjunct.
- **M8. Shipped-but-silent:** `functor.hpp`, `monoid.hpp`, `simd.hpp`, and public-namespace names in
  `detail/typeclass_base.hpp` (`has_instance_v`, `applicative_value_t` — consumed by tree_algorithms).
  `fold.hpp` and `monad.hpp` carry the evidence banner; these do not, and D3200R0's Non-goals name neither
  Functor nor Monoid. Full P/E/S table in report 01 §4.
- **M9. Stated rules the code does not follow:** `proposal-name-case` ("case says whether a name is
  proposed") is false for `fold.hpp` and the whole `*_monoid` family; `lift`/`fold` exist as alias members
  despite "aliases get no second name"; `_with` members sit in proposed wording while mode 2 makes them
  vestigial; `mbind` is ADL-findable cruft; the `derive_*` `static_assert` messages are unreachable from
  lookup failure (the diagnostic is "no member named 'fmap' in integral_constant<bool,false>").
- **M10. Value category is handled on the Applicative side only.** `VectorFunctorImpl::fmap` takes
  `const vector&` only — `fmap(f, std::move(v))` copies every element; no Functor/Foldable analogue of the
  consuming declaration.
- Positive, for balance: compile cost of the core headers is small (~0.1 s over a std baseline); constexpr
  claims hold except the documented `std::function` fold paths; `consumes_rvalue_structure`, `collect`, and
  `invoke`-as-interface impose nothing on containers; `has_instance_v` and partial specialization over free
  `A`/`T` worked unchanged for the layers.

---

## 3. The consumers

### fingertree (report 02)
- **Bug (latent, one token):** `append`'s allocator-mismatch rebuild calls `from_sequence(right.flatten(),
  d_alloc)` with the monoid defaulting to `MONOID{}` (`finger_tree.hpp:1185`); the three sibling rebuild
  sites (`:916, :931, :947`) pass `d_monoid`. A stateful `modular_sum{5}` PMR tree across two resources
  recomputes node measures under modulus 0. *Verified by reading.* Not fixed here (not asked); needs a test.
- `MONOID` is never constrained by transpose's own `monoid_object` concept; `MEASURE_POLICY` must be stateless
  (`meas_fn()` default-constructs, `:320`); the two parameters are one Haskell typeclass (`Measured`) — a
  `measured` object with `identity`/`combine`/`measure` is the natural shape and is fingertree's own call.
- The measure-as-homomorphism law is unstated and untested: `fold_map(Meas{}, t, t.get_monoid()) ==
  t.measure()` is a one-line test and the equivalence law the skill demands for every new instance.
- Pushbacks fingertree would make to transpose: an optional declared `is_commutative` on monoid instances
  (pattern of `consumes_rvalue_structure`) so `reversed()` of a count tree keeps O(1) `size()`; move
  `optional_min_monoid`/`optional_max_monoid` into `monoid.hpp` beside `first_monoid` ("one definition of the
  machinery" is honoured for the mechanism and quietly not for the instance vocabulary); native `fold_left`
  and `length` to avoid `std::function` allocation and O(n) length on a tree advertising zero allocation.
- Tests: every *measure* monoid tested is commutative, so `dual_monoid`'s flipped combine is never observable
  (the test named `ReversedNonCommutativeElementOrder` uses the count monoid); zero tests for the eight
  wrapper instances; no `static_assert` of `foldable_object`/`traversable_object` conformance anywhere.
- Paper C readiness: no draft; `docs/fingertree/README.md` is the Copier template (TODO, `std::todo`,
  `PnnnnRr`, C++17 claimed vs C++26 floor). Eight gating questions, ordered, in report 02 §4 — container vs
  concept (`measured_sequence`) first, the Traversable slice + allocator propagation second.

### tree_algorithms (report 03)
- **The two-tier split is clean in code and now checked** (`-MM` closure of the five explicit-tier headers
  has zero transpose includes; the CMake `#error` TUs enforce it). *Verified.*
- Less clean in the paper: the primary-tier "After" example uses `overloaded` — now P3200's — unattributed;
  the explicit-tier *tests* include `transpose/overloaded.hpp`; the paper still says "consumer-stub lookup
  objects" while the lookup tier *defines* two lookup variables of its own (`layer_fold`, `project`) that
  LEWG will read as new customization points. Own that.
- Synopsis: no signature drift (13 declarations checked one by one). But it omits the two lookup variables
  its ledger claims to mirror; ledger paths are `include/…`; prose predates the 2026-07-18 `child_slot`
  amendment (paper still says every recursive position is `Box<A>`; Costs still says "performance is a
  non-goal" with MEASURE placeholders while DECISIONS.md records 4.656 ms vs 4.642 ms hand-written, zero
  allocations, and says the paper should state it).
- Merge friction: namespace hopping in five headers plus bench and tests (a qualified specialization from
  global scope compiles on clang-18; GCC untested here); stale `static_assert` messages naming
  `layer_fold_typeclass`/`project_typeclass`; the Map's `using Impl::fmap` hides `derive_functor::fmap`, so
  the CRTP base contributes only `replace`, used once in a test — the cost of a base whose derived surface the
  consumer does not want. Per-type lookup registration is ~33 lines for `fmap` + ~20 `layer_fold` + ~15
  `project` against a 12-line lambda; at five representations the trade is a wash and the paper should say so
  (transpose's own `explicit-parameter-tier-sufficiency` data point agrees).
- Code comments cite "Decision 4", "DEV-01/03/04" in 49 places inside `src/` and `examples/` — public text
  by your own rule, since anchored regions transclude into posts.

---

## 4. Decisions needed from you

Each is a slug waiting to be written. Recommendation in parentheses.

1. **traversable-for-shape-dependent-structures** — what Traversable/Functor mean for a type whose shape
   depends on the element type (register the count slice only; measure-changing traversal is a separate verb).
2. **fold-map-call-shape** — one `fold_map` shape across the repo (monoid as object with `identity()`/
   `combine()`, duck-typed in the explicit tier; settle argument order once).
3. **layer-fold-and-foldable** — already OPEN: is `layer_fold<Layer>` the per-layer Foldable? (Yes; register
   `foldable<Fix<F>>` etc. in the lookup tier.)
4. **instance-registration-mechanism** — record what the registry is (variable template), how it resolves
   (partial ordering; constraints as tie-breaker), who may specialize for which types, the cross-TU hazard, the
   module-attachment question.
5. **functor-names-across-the-family** — `map` vs `fmap`; does an Applicative-registered type have a Functor
   (M1).
6. **registered-instance-state** — amend: the defaulted paths either read `monoid<R>` or the state allowance
   is for passed instances only (M2). Related: unify the `Monoid<T>` class-template registry with the
   variable-template shape.
7. **element-type-hook** — one customizable `element_t` replacing `applicative_value_t`/`element_type` (M3).
8. **monoid-instance-properties** — optional declared `is_commutative` (and `is_idempotent`) on instances,
   re-exported by combinators.
9. **adjoined-identity-min-max** — `optional_min_monoid`/`optional_max_monoid` move to transpose.
10. **grade-surface-in-applicative-core** — `subsume` and `graded_context` out of the core (M7).
11. **unproposed-surface-labelling** — banners on `functor.hpp`, `monoid.hpp`, `simd.hpp`; Non-goals name
    Functor and Monoid; `detail/` names consumers depend on get a public home.
12. **toolchain-floor** — one entry absorbing `execution-toolchain-floor` and [S-5]: Clang 18 (tree_algorithms
    docs) vs 19 (root README) vs "under review" (skill), given tree_algorithms now links transpose which
    measured Clang 18.1.3 crashing on `error_set.hpp`.
13. **paper-numbers** — Paper C is simultaneously D4325R0, "not yet drafted", "unnumbered", `PnnnnRr`, and
    `P4325`; Paper D is `D4322`/`D4322R0`/`P4322`; the essay adds P4307. One provenance entry; everything cites it.
14. **decision-log-layout** — report 05 §4b option C: per-library `decisions.md` ×3 (+ new fingertree) plus
    `docs/coordination/decisions.md` for cross-library rulings, one repo-wide slug namespace checked by
    pre-commit. (Recommended.)
15. **essay-form** — the six-exhibit SPEC (r2) or the prose "Why four papers" post; one must yield. And whether
    P4307 (backtick) is in the map: the skill, README and coordination docs say three papers.
16. **measured-object** (fingertree's own) — one `measured` instance (`identity`/`combine`/`measure`) replacing
    `MONOID` + `MEASURE_POLICY`.
17. **paper-c-shape** — container, concept, or both; `sized_range` honesty (`size()`/`begin()`/`end()`/`==`
    are O(N) for non-count measures while `static_assert(sized_range<…>)` holds).

---

## 5. Infrastructure (report 04)

Method: every file in scope read; configured with CMake 4.3.2 under gcc 13 and clang 18 (both below the floor,
so configure/enumerate/install were verifiable, library compilation was not — clang 18 segfaults on transpose,
consistent with the README). Lead spot-checked the four claims marked † against the files.

**The good news: it is one coherent CMake project.** `beman.X` / `beman::X` / `beman.X-package` naming is
uniform; tree_algorithms and fingertree genuinely link `beman::transpose`; installing produces
`beman.transpose` + `beman.tree_algorithms` packages with `find_dependency(beman.transpose)` (verified); the
explicit-tier isolation TUs compile clean even on clang 18; the three `config.hpp` triplicates agree; 75 UUID
anchor pairs, none duplicated; commit-authorship check works.

**Significant:**
- **I1. The install test is broken by its own relocation.†** `Makefile:335` runs `cmake -S installtest` where
  no `CMakeLists.txt` exists; `installtest/transpose/CMakeLists.txt:11-14` hardcodes `../.install` with
  `NO_DEFAULT_PATH`, now resolving to `installtest/.install`, which nothing populates. Only transpose is covered;
  nothing in CI runs an install test at all.
- **I2. `doxygen-gh-pages.yml` fails on every push to main**† — it names `docs/Doxyfile`, which does not exist
  (known since July, tree_algorithms punch list). **API documentation is effectively nonexistent as a product:**
  `docs/transpose/Doxyfile` is the 2943-line exemplar (PROJECT_BRIEF "An Example of Everything", missing logo,
  `INPUT = src docs` so it would ingest all three libraries *and* every transactional doc, nonexistent
  EXCLUDE/EXAMPLE_PATH); `docs/fingertree/Doxyfile` points at `include/beman`; no tree_algorithms Doxyfile;
  `mrdocs.yml` is the untouched `beman::optional` exemplar; `Makefile` docs/mrdocs targets reference files that
  do not exist. Zero `@defgroup`/`@page`/`@anchor` anywhere — so no collisions, and no structure. Doc dialects
  diverge (transpose `///` + `//!` specgen; the others `/** */`), and Doxygen *will* ingest the `//!` wording
  blocks — `wording-pipeline.md:84`'s claim that `//!` is not Doxygen is wrong.
  **Target state:** one `docs/api/Doxyfile` over `src/beman` with per-library groups plus a cross-cutting
  `typeclass` group (the mechanism, documented once), library-slug-prefixed page ids, an `INPUT_FILTER` or
  `ALIASES` for `//!` blocks, one deploy path shared by Makefile and workflow; delete `mrdocs.yml`.
- **I3. Modules are mostly dead.** Only `tree_algorithms.cppm` is ever built, in four C++23 Debug CI rows;
  `fingertree.cppm` is unreachable; transpose has no module unit and its `USE_MODULES` macro is never set;
  examples/benchmarks gate `CXX_MODULE_STD` on a different variable than the library. Tie to slug 4's
  module-attachment question and [S-5]'s "ship/waive".
- **I4. The fingertree C++26 gate hard-errors instead of skipping**† on any compiler CMake lacks a C++26 dialect
  for, because the probe sets `CMAKE_CXX_STANDARD_REQUIRED` (`src/beman/CMakeLists.txt:12`); its "only
  libstdc++ 16 ships `inplace_vector`" premise is doubtful (GCC 15 likely ships it — *inferred, check*).
- **I5. `cmake/` is a stale fork of `infra/cmake`** (older `use-fetch-content.cmake` the root CMakeLists works
  around; dead `Config.cmake.in`; clang-flags pins C++20 vs gcc-flags C++26; a gcovr filter matching a path
  that never existed). `infra/.github` and `papers/wg21/.github` workflows are inert, not accidentally live.
- **I6. CI gaps:** nothing runs `make wording-check`, the paper or blog builds, an install test, or the vendored
  `papers/wg21` test suite; `preset-test` re-runs every preset at C++26 on `:latest` images including
  appleclang/msvc, against the matrix's own comment that those stay at C++23 (*inferred*); codeql scans actions
  only; codespell excludes `papers/`.
- **I7. Paper tooling is transpose-shaped.** The wording stamp is wired only to D3200R0 (29 include divs;
  algorithms-for-trees has none and keeps the hand-maintained synopsis); `gen-wording.sh` hardcodes the eight
  transpose headers and `transpose.manifest`; `make papers` depends on `wording`, which requires specgen,
  contradicting `wording-pipeline.md:22`; `defaults.yaml:9` ("appends") contradicts `wording-pipeline.md:182`
  ("replaces"). **Proposed layout:** `papers/wording/<lib>/` with a `<lib>.headers` spec, per-library
  render/manifest/stamp, per-paper Makefile rules — so Paper C and D wording can live beside transpose's
  without touching its pipeline.
- **I8. Blog export is owner-local.†** Five posts and the essay transclude from
  `orgit-file:~/src/typeclass/typeclass::<tag>::include/beman/…` — an absolute home-directory path *and* the
  old `include/` layout baked into content; `pins.md` is referenced four times and does not exist; the 16 blog
  tags exist on the remote but not in a shallow clone; the two 5.7k-line modus CSS files are referenced by
  nothing; `.md` exports are half-committed. Three bibliographies with overlapping keys and one divergent entry.

**Minor leftovers:** per-library READMEs carry retired-repo badges and stale build text (fingertree's is
exemplar TODO text); `CONTRIBUTING.md` documents a nonexistent `BEMAN_TYPECLASS_INSTALL_CONFIG_FILE_PACKAGE`,
`find_package(beman.typeclass)` and GoogleTest; four divergent `.clang-format` files; `.gitignore` misses
`docs/adoc`, `coverage.json`, `.emacs.d/custom.el`, `docs/coordination/essay/*.html`; Makefile submodule
remnants with no `.gitmodules`; `port/README` names `bemanproject/typeclass` while the remote is
`steve-downey/typeclass` (ties to the open `repository-owner` question).

**Infrastructure decisions needed** (add to §4): 18. **api-docs-shape** — one Doxygen run with groups, as
above; 19. **wording-layout** — `papers/wording/<lib>/`; 20. **module-build-status** — ship, advisory, or waive,
per library; 21. **blog-transclusion-root** — repo-relative, not `~/src`; 22. **ci-coverage** — which of
wording-check / papers / install test / wg21 tests become required jobs.

---

## 6. The typeclass case: teaching, background, the comparison paper, pattern matching

Full outline in `06-research-organization.md`. The load-bearing points:

- **What is being sold, in one sentence:** an instance is a *value* — a stateless object holding a type's
  implementations of a named family of operations, found by a per-concept variable template specialized on the
  type, and passed, pinned, or defaulted like any other value. Everything that works (four lookup modes,
  `_with`, instance objects for monoids, inheriting an instance to bring names into scope) follows from "is a
  value". What a language would have given you, and the library cannot, is *who decides which value*. That is
  the research.
- **Three audiences, three documents**, not one: LEWG (why bundled, why library-level now, what P2279-style
  language support would subsume later — modes 1 and 4 — and what it would remove — modes 2 and 3);
  implementers (the derivation bases, basis coherence, wording generation; `typeclass-object-pattern.md` is the
  seed and is stale); working programmers (the blog series, which lacks the "instances are values" post).
- **Literature in four tiers** (06 §2): typeclasses-as-dictionaries (Wadler–Blott; Hall et al. 1996; Peyton
  Jones–Jones–Meijer 1997; **Oliveira–Moors–Odersky 2010 "Type Classes as Objects and Implicits"** — the
  closest published relative and the anchor citation); resolution across languages (GHC overlap pragmas;
  Morris–Jones instance chains; **Selsam–Ullrich–de Moura 2020 "Tabled Typeclass Resolution"**; Rust RFC 1023;
  Odersky "Simplicitly"; Coq canonical structures; Agda instance arguments); C++'s own history (Gregor et al.
  2006 concept maps; **Bernardy et al. 2008 "A Comparison of C++ Concepts and Haskell Type Classes"** — the
  direct ancestor of the planned paper; Garcia et al. 2007 criteria grid; N4381 / P1895 / **P2279** / P2547);
  the algebras (already well covered). One `papers/references.bib`; the two copies today duplicate four entries.
  Items from memory are marked (verify) in 06.
- **The comparison paper** (06 §3): a ten-row grid — registration unit, selection rule, multiple instances per
  type, generic-vs-specific overlap, coherence guarantee, higher-kinded keys, output/associated types, where
  lookup is written, explicit instance passing, diagnostics — across C++ variable-template / GHC / Lean 4 (Rust
  as a fourth column if space). Five experiments the repo does not have: constrained generic + vector instance
  partial ordering across GCC/Clang/MSVC and the undecidable two-constrained case; cross-TU incoherence and
  whether a module exporting the specialization closes it; "many monoids on int" written three ways; `fmap`
  over `Fix<F>` three ways (the HKT cost); lookup vs CPO/tag_invoke compile cost on the existing fanout
  benchmark. The Lean side has a sibling repo already (`lean-model-sync`, steve-downey/lean-graded).
- **Pattern matching** (06 §4, 03 §6): the thesis sentence is that typeclass lookup is *type-level* pattern
  matching (partial specialization) and an algebra is *value-level* matching on one layer; orthogonal and
  composable. Variant layers (`ExprF`, `NatF`, `FringeTreeF`) transliterate to `match` directly; struct layers
  (`BinaryTreeF`, `RoseF`) gain little, so show one of each. `overloaded` (best-match) and `match` (first-match,
  exhaustive) compete for the same job — the ledger already records that `overloaded`'s exhaustiveness
  diagnostic is lost for every value-returning visitor, i.e. for every algebra in Paper D — and the deferred
  `fallback-overload-set` question dissolves under `match`. `Box`/`child_slot` must satisfy whatever
  optional-like pattern protocol lands, or boxed and inline slots stop "reading the same". Every P2688 claim
  is re-verified at publication; the punch-list item stands.

---

## 7. Sequencing to end of month

1. **Write the 17 slugs** (§4) — most are a paragraph each; the rulings already exist in your head or in the
   files about to be deleted. Do B4's migration (05 §3, 31 rows) in the same pass. Create
   `docs/coordination/decisions.md` and `docs/fingertree/decisions.md`; convert `DECISIONS.md` to slugs.
2. **Mechanical sweep** of the KEEP set: `include/`→`src/`, `~/src/` paths, `pure`+`apply`→`pure`+`invoke`|`ap`
   in five reference docs, `docs/decisions.md#`→`docs/transpose/decisions.md#` in headers, the three
   per-library READMEs rewritten from the root README's facts, the 49 numbered identifiers in code comments
   replaced by the rule in words. Add the link check and the slug-uniqueness check to pre-commit.
3. **Code changes that follow from the rulings**, smallest first: the `append` monoid fix + test; the
   `monoid_object` constraint on `MONOID`; the homomorphism law test; `foldable_object`/`traversable_object`
   static_asserts; then B1 (register the count slice), B2 (fold_map shape), M1/M2/M3.
4. **Delete the transactional set** (05 §5 list) only after each file's rows have landed with `Sources:`.
5. **Then** the essay decision (§4 item 15) and the "instances are values" post, which the three papers cite
   instead of each re-explaining the mechanism; the comparison paper's experiments can start as soon as
   slug 4 exists, since E1/E2 are a dozen lines of probe each.
