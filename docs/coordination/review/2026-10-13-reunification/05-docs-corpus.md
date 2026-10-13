# 05 — Documentation corpus audit and decision-doc integration plan

Scope: every `.md`/`.org` under `docs/` (62 files, 18,181 lines), `README.md`, `CONTRIBUTING.md`, `.claude/skills/tree-proposals-coherence/SKILL.md`, `papers/blog/index.{md,org}`, and the front matter + section lists of `papers/D3200R0.md` and `papers/algorithms-for-trees.md`. All files were read in full. Grep counts below were run on the checkout at `/home/user/typeclass` (every file's git date is 2026-10-11/12 because of the reintegration, so "last meaningful date" is taken from the latest date written inside the file).

## Executive summary

1. The corpus is three decision systems plus a large transactional shell. Only `docs/transpose/decisions.md` (3,470 lines, 48 slugged entries) follows the owner's slug rule. `docs/tree_algorithms/DECISIONS.md` is numbered (Decision 1–8 + four dated amendments), fingertree has no decision log at all (its three design docs *are* its log, with numbered `### 1.`–`### 8.` headings), and the cross-library rulings live in `SYNC.md` entries, `STATUS.md` `[S-n]` markers, `machinery-deduplication.md` "Questions", and the coherence skill.
2. At least 31 rulings by Steve are recorded *only* in files the end-of-month rule would delete (worklists, STATUS/SYNC, essay CRITIQUE, blog PLAN, punch list, representations note, grading-plan §4/§5). The biggest: the 2026-07-13/14 `ap`-basis/`invoke`-interface ruling has no entry in the transpose decision log (it lives in SYNC entries 3 and 5, the 2026-07-13 worklist, and `provenance.md`); the 2026-07-20 typeclass-instance contract (shape unspecified, call-position validity only, trivially copyable, taken by value, "not a resource") exists only in `docs/coordination/essay/CRITIQUE.md` §14–§15; the reintegration decision itself (why one repo, the layout, tag prefixing, `archive/cited-commits`) exists only in `SYNC.md`. Full list in §3.
3. Slug collisions already exist: `overloaded-home` is a `##` heading in `docs/transpose/decisions.md:3415` and a `###` heading in `docs/coordination/machinery-deduplication.md:168` with different text; the superseded `decisions-execution-addendum.md` duplicates 10 slugs of `decisions.md`; and `machinery-deduplication.md`'s seven stage slugs (`ruling-before-code`, `explicit-tier-isolation`, `transpose-constexpr-derivations`, …) are link *targets* that have no heading of their own, so the three inbound links from `DECISIONS.md` and `decisions.md` dangle on GitHub.
4. The merge left the docs describing a repo that no longer exists: 149 references to `include/…` paths (the include root is `src/`), 19 references to `~/src/...` sibling checkouts (three of them to `~/src/transpose/main`, now in-tree), 20 dead links from `decisions.md` into `../tmp/plan/step-*.md`, a dangling reference from `extraction-plan.md:436-438` to a coordination `DECISIONS.md`/`graded-fold-scope`/`transpose-grading-landed` that never existed in this checkout, and three per-library READMEs whose badges, options, floors, and test frameworks are those of the retired repos.
5. Factual collisions that need one ruling each: Paper C is "D4325R0, assigned 2026-07-19" (skill), "not yet drafted" (root README), "Paper C (unnumbered)" (coordination README, extraction-plan), and `P4325` (essay); tree_algorithms' Clang floor is 18 (its README, DECISIONS Decision 3), 19 (root README), and "under review" (skill) — while transpose, which tree_algorithms' lookup tier now links, has measured that Clang 18.1.3 crashes on `error_set.hpp` (`execution-toolchain-floor`); the Applicative primitive is still `pure` + `apply` in `CODING_RULES.md:98`, `typeclass-object-pattern.md:209,222,245`, and two `.org` explainers, against the settled `ap`/`invoke` ruling; and the typeclass-instance contract in CRITIQUE §14 ("no observable state") conflicts with `monoid-selection` points 4–5 (instances may carry state, 2026-10-07).
6. Recommendation: four decision files (one per library plus `docs/coordination/decisions.md` for cross-library rulings), one repo-wide slug namespace enforced by a pre-commit check, `DECISIONS.md` converted entry-by-entry to slugs with the amendments folded into Logs, fingertree's design docs mined into a new log, and the transactional set removed after a one-pass migration of the 31 orphans listed in §3. Grep counts for the identifier migration: `Decision [0-9]` 43 hits (10 in `src/`/`examples/` code comments), `DEV-0` 65 (39 in `src/`/`examples/`), `\[S-[0-9]` 17, fixpoint `D1–D11` 14, "SYNC entry" 17.

What is verified vs inferred: every file:line, count, and quotation below was read or grepped in this checkout. Classifications ("transactional" vs "decision") apply the owner's rule as stated in the preamble; where a file is a hybrid I say so and mark it UNSURE. The slug proposals in §4 are proposals, not records of anything Steve has chosen.

---

## 1. Findings (severity-ordered)

### F1 — Steve rulings recorded only in files scheduled for deletion — **blocking**
Evidence: see §3 table; the worst cases are the `ap`-basis ruling (`docs/coordination/SYNC.md:84-98,121-132`; `docs/transpose/coordination-worklist-2026-07-13.md:89-128`; `docs/transpose/provenance.md:77-87` — no `decisions.md` slug), the instance contract (`docs/coordination/essay/CRITIQUE.md:768-993`), the reintegration record (`docs/coordination/SYNC.md:46-82`), `fold_left`-is-Foldable's (`docs/coordination/STATUS.md:27-29`; `docs/tree_algorithms/notes/representations-and-sequence-interfaces.md:163-174`), publication venue `[S-4]` (`STATUS.md:40-42`, `SYNC.md:134-141`), fingertree grading scope (`docs/transpose/transpose-grading-plan.md:368-394`).
Why it matters: the owner's rule preserves decisions and removes transactional docs; without a migration pass these rulings vanish with their containers, and the skill (`SKILL.md:88-100`) already cites SYNC entries as the authority for several of them.
Recommendation: migrate each to a slugged entry before any deletion (checklist in §4e). Treat `SYNC.md` as a history/provenance document to keep, not as the decision record.

### F2 — Slug and anchor collisions across the files to be integrated — **significant**
Evidence: `overloaded-home` at `docs/transpose/decisions.md:3415` (`##`) and `docs/coordination/machinery-deduplication.md:168` (`###`), different bodies, each linking to the other; `machinery-deduplication.md:41-115` stage headings `### Stage N — [slug](#slug)` where no `### slug` heading exists, so `DECISIONS.md:181` (`#explicit-tier-isolation`), `DECISIONS.md:184` and `decisions.md:3451` (`#transpose-constexpr-derivations`) resolve to nothing on GitHub (verified: the only `###` targets in that file are the six Questions, `machinery-deduplication.md:158-240`); `decisions-execution-addendum.md` repeats `execution-dependency-shape`, `sender-instance-keying`, `sender-value-type-reading`, `runtime-arity-composition`, `erasure-boundary`, `all-of-failure-semantics`, `demo-sender-fate`, `sender-error-grade`, `sender-monad-instance`, `static-arity-array` (10 duplicates, all now in `decisions.md`); `transpose-grading-plan.md` and `transpose-execution-plan.md` both define explicit `{#context}`, `{#standing-decisions}`, `{#work-plan}`.
Why it matters: any single-file or cross-file integration will produce duplicate anchors; GitHub silently renders the second as `slug-1`.
Recommendation: one slug namespace repo-wide; keep `overloaded-home` in exactly one file (the cross-library file, since it is a C/D/A division-of-labor ruling) and leave a one-line redirect stub in the other; delete the addendum (its reconciliation is already logged under `p2300-front-door-shape` 2026-09-13).

### F3 — Three anchor styles in use; two of them break on GitHub — **significant**
Evidence: bare `## slug` headings (`decisions.md`, machinery Questions) — auto-anchor equals slug, works everywhere; pandoc explicit `{#slug}` (28 instances, only in the two transpose plans) — GitHub does not honour `{#id}`, so `transpose-execution-plan.md#execution-baseline` works in pandoc output but on GitHub the heading `### Stage 0 — [execution-baseline](#execution-baseline) {#execution-baseline}` gets a long auto-anchor; prose headings (`DECISIONS.md` "## Decision 4 — API shape: explicit `fmap` …", amendment headings) — the one inbound link, `extraction-plan.md:33` → `#amendment-2026-10-12--the-lookup-tier-consumes-bemantranspose`, depends on GitHub's exact slugification of a colon and a dot.
Recommendation: the entry heading *is* the slug (`## slug`), nothing else on the line; put the question in the body. Never use `{#…}`.

### F4 — Docs describe the retired repos' layout and toolchains — **significant**
Evidence: `include/` paths: 149 hits (`anchor-inventory.md` 35, `p3200-reference-implementation-review.md` 27, `decisions.md` 23, `transpose-execution-plan.md` 11, `CODING_RULES.md:21-23`, `typeclass-object-pattern.md:36-38`, `docs/transpose/README.md:59`, `SKILL.md:179`, `DECISIONS.md:137`, `STATUS.md:100`) vs `README.md:47` "`src/` is the single include root" and the actual tree `src/beman/<lib>/*.hpp` beside `*.test.cpp`. `tests/beman/` paths: 33 hits across 12 files; there is no `tests/` directory. `~/src/` paths: `SKILL.md:27-28`, `docs/coordination/README.md:10-11`, `CLAUDE.md:21`, `extraction-plan.md:4,19-23,402,409` (including `~/src/transpose/main`, now in-tree), `provenance.md:13`, essay `.org` link labels. Old repo URLs: `docs/transpose/README.md:7-9` (`bemanproject/transpose` badges), `docs/tree_algorithms/README.md:9-11`, `docs/fingertree/README.md:9-11`; `papers/algorithms-for-trees.md` and three `papers/blog/*.md` link `github.com/steve-downey/{tree_algorithms,transpose}` (retired per `README.md:30-31`).
Recommendation: a mechanical sweep for the live docs (READMEs, CODING_RULES, pattern doc, skill, anchor inventory); historical log entries may keep old paths if the log header says so once.

### F5 — Per-library READMEs and CONTRIBUTING contradict the root README — **significant**
Evidence: `docs/fingertree/README.md:15-17` "(... TODO: description)", "`std::todo` proposed in TODO (PnnnnRr)", `:40` "C++20 standard or greater", `:42` "(Test Only) GoogleTest", `:49-59` table "GCC 16-13 … C++26-C++17", `:44` `BEMAN_FINGERTREE_BUILD_TESTS` — vs `README.md:59-60` "needs `std::inplace_vector` at C++26, which today means GCC 16", `:62` Catch2, `:89` `BEMAN_TYPECLASS_BUILD_TESTS`. `docs/tree_algorithms/README.md:76` `BEMAN_TREE_ALGORITHMS_BUILD_TESTS`, `:81` "toolchain floor is GCC 14 and Clang 18", `:85-88` "Clang 22-18", `:124` `vcpkg install beman-tree-algorithms` — vs `README.md:58` "GCC 14, Clang 19". `docs/transpose/README.md:59` "Headers live under `include/beman/transpose/`", `:73-74` "three lookup modes: implicit (`*_typeclass<T>` …)" — vs `operation-objects` (four modes) and `lookup-variable-naming` (`functor<T>`). `CONTRIBUTING.md:35` "requires GoogleTest", `:131-132` `find_package(beman.typeclass)` (no such package; `README.md:98-105` says each library installs its own). `docs/coordination/STATUS.md:3` "Updated: 2026-07-17" with a 2026-10-12 Done entry at `:156`.
Recommendation: rewrite the three library READMEs from the root README's facts; delete the copier boilerplate (vcpkg registry, install tree) or make it true.

### F6 — Paper-number and language-floor statements disagree — **significant**
Evidence (paper C): `SKILL.md:177-178` "Paper C is D4325R0 … assigned 2026-07-19"; `README.md:22` "not yet drafted"; `docs/coordination/README.md:8` and `extraction-plan.md:20` "Paper C (unnumbered)"; `machinery-deduplication.md:56` "Paper C is not yet drafted"; `docs/fingertree/README.md:17` `PnnnnRr`; essay files use `P4325`/`P4322`/`P4307` (`trees-and-transposition-for-29.org`, `PUBLISHING-NOTES.md:45-46` "P3200, P4307, P4322, P4325 all return 404 on wg21.link as of 2026-09-22"). Paper D: `D4322` (README, STATUS, D3200R0), `D4322R0` (skill, DECISIONS, papers), `P4322` (essay). `DECISIONS.md:3` still titles Paper D "Recursive Tree Algorithms for the Standard Library" (superseded by "Algorithms for Trees", `DECISIONS.md:149`).
Evidence (floors): `DECISIONS.md:40,199` "gcc-14+ / clang-18+"; `docs/tree_algorithms/README.md:81` Clang 18; `README.md:58` Clang 19; `SKILL.md:110-111` "clang floor under review"; `extraction-plan.md:278` "clang-19(18?)+"; `punch-list.md:28` clang-18 CI jobs fail (constexpr `Box`); `decisions.md:2845-2873` (`execution-toolchain-floor`, OPEN) "Clang 18.1.3 crashes the frontend on `error_set.hpp`", GCC 13 cannot build. Since 2026-10-12 tree_algorithms' lookup tier links `beman::transpose` (`DECISIONS.md:180`), so its effective floor is transpose's.
Recommendation: one `toolchain-floor` entry in the cross-library log (absorbing `execution-toolchain-floor`'s facts and `[S-5]`'s clang-18 question), one `paper-numbers` provenance entry; READMEs cite them.

### F7 — Settled vocabulary contradicted in reference docs — **significant**
Evidence: `pure` + `apply` as the Applicative basis at `CODING_RULES.md:98`, `typeclass-object-pattern.md:209,222,245`, `typeclass-object-pattern-in-cpp.org:523`, `box-traits-and-typeclass-objects.org:569` — against the ruling "name is `ap`, never `apply`; never lead with it" (`SKILL.md:86-96`, `SYNC.md:84-98`). `CODING_RULES.md:21-23` prescribes the split `include/`/`tests/` layout that `codestyle.org:25-30,64-73` (merged `src/`, "merged-src pattern wins") and the actual tree contradict. `DECISIONS.md:56,133` and `article-reconciliation-memo.md:35,70` name `functor_typeclass`; `STATUS.md:57` too; `decisions.md` body uses `monoid_v`/`applicative_typeclass` in pre-2026-10-10 entries (acceptable as history, but `p2300-front-door-shape`'s live invariant "nothing execution-dependent enters `include/`" (`decisions.md:2015-2026`) is phrased with a path that no longer exists).
Recommendation: fix the five `apply` sites now (they are reference docs, not history); add a one-line "spelling history" note at the top of `decisions.md` so old names in Logs read as history.

### F8 — Dangling cross-references that will not survive the cleanup — **significant**
Evidence: `decisions.md` → `../tmp/plan/step-*.md` (20 links, 12 distinct files; `docs/tmp` does not exist); `extraction-plan.md:436-438` → "`graded-fold-scope` in the coordination repo's `DECISIONS.md`, pinned by the SYNC entry `transpose-grading-landed`" (neither exists anywhere in the checkout); `STATUS.md:82` → `tree-coordination/essay/SPEC.md`; `docs/fingertree/measure-based-splitting.md:300` and `CODING_RULES.md:88` → `docs/decisions.md#monoid-selection` (wrong relative path from `docs/fingertree/` and `docs/transpose/`); `SKILL.md:134,139,148,156` → `docs/DECISIONS.md`, `docs/provenance.md`, `docs/notes/extraction-plan.md`, `docs/notes/anchor-inventory.md` (old repo-relative); `coordination-worklist-2026-07-13.md:19` → `papers/wg21/D3200R0.md` (now `papers/D3200R0.md`); `operation-objects-and-the-lookup-modes.org:1036` → `file:../examples/cpo_example.cpp` (would resolve to `docs/examples/`); `docs/transpose/mrdocs.yml:6-19` → `../include`, `beman::optional::**` (a copier leftover from another project). `prior-art-when-all-range.md` is cited *instead of* restating facts by `decisions.md:2543-2547` and `:2469-2491` ("cited there rather than restated here"), so deleting it as a dated review would hollow out two DECIDED entries.
Recommendation: link-check as part of the migration (a 10-line script found all of the above); either keep `prior-art-when-all-range.md` as a reference or fold its §2/§3/§6 into the two slugs.

### F9 — Instance statefulness: two rulings disagree — **significant**
Evidence: `CRITIQUE.md:770-778` (Steve, 2026-07-20): "no contract to treat them as stateful … **A typeclass is not a resource**"; `:939-941` "trivially copyable, copies interchangeable, no observable state"; `:982-988` five-line contract. `decisions.md:3004-3046` (`monoid-selection` points 4–5, Steve 2026-10-07): "Instances may carry state. The library does not go out of its way to avoid copying them … A registered instance may be stateful"; `registered-instance-state` (`:3174-3206`) "No bound"; `docs/fingertree/allocator-design.md:55-58` stores the instance on the tree; `measure-based-splitting.md:174-193` `modular_sum{5}`. "Trivially copyable + taken by value + copies interchangeable" survives; "no observable state" does not. The essay (`trees-and-transposition-for-29.md:206`) already says "the standard typeclasses being proposed are all stateless. User written typeclasses might have state".
Recommendation: when CRITIQUE §14 is migrated to a slug, record it as DECIDED 2026-07-20 and AMENDED 2026-10-07 by `monoid-selection`, with the surviving clauses stated.

### F10 — Internal identifiers leak into shipped headers and examples — **minor**
Evidence: `DEV-0` in 39 `src/`/`examples/` sites (`recursion_schemes.test.cpp` 7, `binary_tree.test.cpp` 5, …, `examples/tree_algorithms/{sequence_algorithms,search_tree_on_fix,expression_algorithms}.cpp`); `Decision [0-9]` in `recursion_schemes.hpp`, `recursion_schemes_lookup.hpp`, `fold_map_lookup.hpp`, `binary_tree.hpp` and six tests/examples. The skill's own public-text rule (`SKILL.md:72-80`) and `MUSTFIX.md` MF-4 (`:115-151`) already treat header comments as public text because anchored regions transclude into posts. Papers and `papers/blog/*.md` are clean (grep returned nothing).
Recommendation: part of the identifier migration — replace with the rule stated in words plus the slug in a non-transcluded comment.

### F11 — `fixpoint` stage numbers collide with Steve markers — **minor**
Evidence: `STATUS.md:133` "Evidence repo complete (S00–S16, 214 tests)" and `MUSTFIX.md:118` grep `S0[0-9]|S1[0-6]` use `S-nn` for fixpoint stages; `STATUS.md:6-42` uses `[S-n]` for Steve decisions. Two meanings of one abbreviation in one directory.
Recommendation: moot once `[S-n]` is retired to slugs; do not reuse `S-`.

### F12 — Generation noise and stale forks kept beside sources — **minor**
Evidence: `codestyle.org:32,39,45,…` carries `citeturn9search0turn12search10…` artifacts and `:806-810` instructs an agent Co-Authored-By trailer; `docs/coordination/essay/fork-essay.org` (666 lines, 13 `#+begin_comment` revision-board blocks dated through 2026-09-05) is the pre-strip copy of `trees-and-transposition-for-29.org` (340 lines, 0 comment blocks) that `PUBLISHING-NOTES.md:4-7` says was stripped on 2026-09-22 and "is in git history"; `trees-and-transposition-for-29.html` is a stale export (`PUBLISHING-NOTES.md:55-56`); `docs/transpose/metrics/fanout-runs.jsonl` records one day's step timings.
Recommendation: remove the fork, the html, the jsonl, `mrdocs.yml`; strip the `citeturn` tokens and the agent-instruction section from `codestyle.org` or move the latter to `CLAUDE.md`.

---

## 2. Inventory

Classification key: **KEEP-D** = KEEP-AS-DECISION, **KEEP-E** = KEEP-AS-EXPLAINER, **KEEP-R** = KEEP-AS-REFERENCE, **REMOVE** = TRANSACTIONAL-REMOVE-EOM, **UNSURE**. "Orphan decisions" = rulings in this file that no decision log records; "—" means none, or everything here is already in a log. Date = latest date written inside the file (git dates are all 2026-10-11/12).

| Path | Library | Kind | Class | Orphan decisions not in any decision log (would be lost) | Date |
|---|---|---|---|---|---|
| `README.md` | repo | README | KEEP-R (rewrite) | Layout rule "`src/` is the single include root" (`:47`); fingertree build-probe policy (`:59-60`). Neither is in a log. | 2026-10-12 |
| `CONTRIBUTING.md` | repo | README | KEEP-R (rewrite) | — (copier boilerplate; GoogleTest and `beman.typeclass` package claims false) | — |
| `.claude/skills/tree-proposals-coherence/SKILL.md` | repo | invariants/coding-rules | KEEP-R (is the invariants doc) | Frozen design decisions, naming, test discipline, hazards — several only here: "Public text stands alone" ruling 2026-07-17 (`:72-80`); "paper files named by name not number" ruling 2026-07-17 (`:182-187`); "no normative dependence C↔D"; "traversal order is contractual"; "extraction is one-way" | 2026-10-12 |
| `docs/coordination/CLAUDE.md` | coord | agent orientation | REMOVE (or 5-line rewrite) | Process rule "every coordination decision gets a SYNC entry" — superseded by whatever §4 adopts | 2026-10-12 |
| `docs/coordination/README.md` | coord | README/map | KEEP-R (rewrite) | — ("Paper C (unnumbered)" stale; `~/src` paths) | 2026-10-12 |
| `docs/coordination/STATUS.md` | coord | worklist/status | REMOVE after migration | **[S-4]** sdowney.org publishes everything (`:40-42`); **[S-6] partial**: `fold_left` is Foldable's derived op, one definition from `fold_map`, never a view-based parallel (`:27-29`); **[S-5] open items**: repo owner swap, clang-18 floor, modules ship/waive (`:12-19`); transpose item 6 n-ary BinaryTree note (`:112-115`) | 2026-10-12 |
| `docs/coordination/SYNC.md` | coord | decision ledger + SHA pins | UNSURE → KEEP as *history* after extracting decisions | 2026-07-13 (1): "repo surface ≠ paper surface" principle adopted; Monad acknowledged out-of-band; 2026-07-13 (3): pure/apply forms removed; 2026-07-14 (4): Monad deferred-not-rejected framing; 2026-07-14 (5): `ap` basis / `invoke` interface, final form; 2026-10-12: reintegration decision, layout (`src/beman/<lib>`, papers flat "because MPark.WG21 requires it", namespaces unchanged), tag prefixing, `import/<repo>/<branch>` branches, `archive/cited-commits`, "pinned states are beman.typeclass SHAs" | 2026-10-12 |
| `docs/coordination/machinery-deduplication.md` | coord | plan + decision questions (hybrid) | UNSURE: Questions → KEEP-D (migrate); Stages/Outcome → history | `fingertree-public-names` DECIDED (no aliases; accepted type-identity cost) (`:158-166`); `consumer-lookup-names` DECIDED (`:192-200`); `layer-fold-and-foldable` OPEN (`:202-209`); `binary-tree-ports` OPEN (`:210-217`); `fallback-overload-set` DEFERRED with candidate-name list, `overload_set` reserved, exhaustive/partial vs first-match/best-match terminology (`:218-240`); gcc-16 catch-all measurements under `overloaded-home` (`:184-190`); "Left open" items (P3200 text for `overloaded`, module tests, `fold_left` via `std::function`, gcc-14 `std::format`) (`:137-143`); target dependency shape (`:23-36`) | 2026-10-12 |
| `docs/coordination/blog/PLAN.md` | coord/blog | plan | REMOVE after migration | Positioning (Steve 2026-07-21): monadic-algorithms collection sits beside the campaign essay and carries the long-form "why not Monad"; thesis "few algorithms require Monad in an interesting way"; Functor→Applicative→Monad as three grades of composition (Steve's framing); reflection-monad instance lives in fixpoint's orbit (`:6-15,42-55,89-93`) | 2026-07-21 |
| `docs/coordination/blog/MUSTFIX.md` | coord/blog | worklist | REMOVE | Decisions *needed* (MF-1..3), none recorded as made; MF-4 rule "headers describe, docs cite" (`:131-139`) | 2026-07-21 |
| `docs/coordination/blog/CRITIQUE-fixpoint.md` | coord/blog | critique | REMOVE | Steve's read on the "worth/earns" tic: fix density not vocabulary (`:52-58`) | 2026-07-21 |
| `docs/coordination/blog/CRITIQUE-transpose.md` | coord/blog | critique | REMOVE | Ruling 2026-07-21: "front door" is not Steve's idiom — replace throughout (`:92-97`); "(Paper C)/(Paper D)" letters are internal, not public (`:77-87`) | 2026-07-21 |
| `docs/coordination/blog/where-are-the-monads.org` | coord/blog | explainer (draft post) | KEEP-E (move to `papers/blog/`) | — (states the Monad position publicly) | — |
| `docs/coordination/essay/SPEC.md` | coord/essay | plan/spec | REMOVE after migration | Exhibit-essay form decisions: six transcluded exhibits, prose ≤1,200 words, truth mechanism "every exhibit transcluded from UUID-anchored CI-covered source", "each paper's companion section shrinks to two sentences + link" (`:3-8,29-37,111-121`) | — |
| `docs/coordination/essay/CRITIQUE.md` | coord/essay | critique + rulings (hybrid) | UNSURE: rulings → KEEP-D (migrate); rest REMOVE | **§13f-2** pre-concede debug-build cost, stratified (Steve 2026-07-20) (`:706-740`); **§14** typeclass-instance contract: shape unspecified and the non-specification is contract, call-position validity only, trivially copyable, taken by value, not a resource, profiling via non-owning handle (`:768-949`); **§14i** derived members are members so instances can override (Steve 2026-07-20) (`:891-904`); **§15** no recommended-practice notes; rationale to paper, guarantees to wording (`:953-993`); **§16** membership criterion "member iff some plausible instance has a materially better implementation", three-language table in paper (`:997-1064`); drop the P3294-is-C++29 timing argument (`:783-793`); Steve directions on lede/fingertree pitch (`:31-48`) | 2026-08-14 |
| `docs/coordination/essay/EDIT-LETTER.md` | coord/essay | edit letter | REMOVE | Decision 1 "instance contract resolved 2026-07-20" cross-ref (`:417-425`) — same as CRITIQUE §14 | 2026-07-20 |
| `docs/coordination/essay/DRAFT-REVIEW-2026-08-06.md` | coord/essay | review | REMOVE | — | 2026-08-14 |
| `docs/coordination/essay/PUBLISHING-NOTES.md` | coord/essay | status/memo | REMOVE after migration | Exhibit tags pinned in three repos (`:56-58`) — now `transpose/blog/...` prefixed tags; wctm transclusion config note | 2026-09-22 |
| `docs/coordination/essay/fork-essay.org` | coord/essay | stale fork with revision board | REMOVE | — (history has it) | 2026-09-05 |
| `docs/coordination/essay/trees-and-transposition-for-29.org` | coord/essay | explainer (published post) | KEEP-E (move to `papers/blog/`) | Public statement of the campaign map; "`invoke` is the key operation and `ap` is implementation detail" (`.md:43`) | — |
| `docs/coordination/essay/trees-and-transposition-for-29.md` | coord/essay | generated export | KEEP-E (regenerate) | — | — |
| `docs/coordination/essay/trees-and-transposition-for-29.html` | coord/essay | stale export | REMOVE | — | 2026-07-19 |
| `docs/fingertree/README.md` | fingertree | README | KEEP-R (rewrite; copier stub) | — | — |
| `docs/fingertree/allocator-design.md` | fingertree | design rationale (numbered 1–8) | KEEP-D (convert to slugs) | All eight: allocator as 4th template parameter + monoid instance stored on tree; which allocations use it (not spine shells); rebuild-on-mismatch at `append`/assign/extended ctor; `is_always_equal` short-circuit; `uses_allocator` ctor by value; propagation traits, non-`noexcept` move-assign; `append` fast path; `allocate_spine` placement-new; null-upstream probe test (`:45-198`) | — |
| `docs/fingertree/compile-time-design.md` | fingertree | design rationale | KEEP-D (convert) | Uniform spine type + runtime variant instead of nested type; `Elem` keyed on the monoid *instance* type; `inplace_vector<ElemPtr,4>` digit; DWARF-count probe as the guard (`:38-84`) | — |
| `docs/fingertree/measure-based-splitting.md` | fingertree | explainer + rationale | KEEP-E | Monoid is a template parameter instance, not looked up from the tag; no `Monoid<size_t>` fallback; `reversed()` keyed on `dual_monoid<MONOID>` (`:6-13,280-308`) — consistent with `monoid-selection`, but the fingertree-side consequence is recorded only here | — |
| `docs/transpose/README.md` | transpose | README | KEEP-R (rewrite) | — | — |
| `docs/transpose/CODING_RULES.md` | transpose | coding-rules | KEEP-R (fix) | Semantic defaults: tree Applicative monad-derived, zip is alternate, range Applicative = nondeterminism, ZipList = positional (`:10-15`); Foldable/Traversable rules (`:86-105`) — rules, not in a log | — |
| `docs/transpose/codestyle.org` | transpose | coding-rules (house style) | KEEP-R (clean) | Merged-`src/` precedence; prolog/SPDX line rules; double-include test pattern (`:20-30,195-214,495-559`) | 2026-04-28 |
| `docs/transpose/provenance.md` | transpose | provenance | KEEP-D | The 2026-07-14 pure/apply removal + evidence-label ruling (`:77-96`) — the only transpose-side record of it; one-way extraction policy | 2026-07-14 |
| `docs/transpose/decisions.md` | transpose | decision log (slugged) | KEEP-D | — (the model) | 2026-10-12 |
| `docs/transpose/decisions-execution-addendum.md` | transpose | superseded draft | REMOVE | — (reconciliation logged under `p2300-front-door-shape` 2026-09-13) | 2026-09-13 |
| `docs/transpose/transpose-grading-plan.md` | transpose | plan | REMOVE after migration | §0 divergence protocol (process); **§4 tree_algorithms guidance** `fold-grade-invariant`, `cata-entry-points`, `widen-once`, `recover-in-clauses`, `fuel-axis` (`:337-366`); **§5 "no structural grading work in fingertree"** with reasoning (`:368-394`); WG21 posture "graded-monad language in rationale, never wording" (`:66-68`) | 2026-08-28 |
| `docs/transpose/transpose-execution-plan.md` | transpose | plan | REMOVE | "`all_of` is not being proposed; a range `when_all` belongs to the execution papers" (`:939-942`) — stated also in the receipts review, in no slug | 2026-09-14 |
| `docs/transpose/coordination-worklist-2026-07-13.md` | transpose | worklist | REMOVE after migration | Item 5: the `apply` removal ruling with the simd argument and the "C++ spellings over Haskell artifacts" editorial principle (`:89-128`) | 2026-07-14 |
| `docs/transpose/coordination-worklist-2026-09-07.md` | transpose | worklist (all HELD) | REMOVE after migration | Item 3 Monad alternate complete bases contingency, lazy-sender acceptance tests, SQL-NULL/Promise motivating prose, "no derivation may flatten on the caller's behalf" (`:186-260`); Item 4 Identity/Const combinators, HKT via reflection far horizon (`:262-303`) — only "recorded contingency" appears in `typeclass-conformance-depth` Log | 2026-09-07 |
| `docs/transpose/baseline-vocabulary-audit.md` | transpose | audit deliverable | REMOVE (fix inbound link) | — (findings logged under `grade-operation-spelling`, `expected-instance-introduction`) | 2026-09-13 |
| `docs/transpose/typeclass-object-pattern.md` | transpose | explainer/reference | KEEP-E (fix `apply`, paths) | Algorithm-object inheritance pattern (`:135-201`) | — |
| `docs/transpose/wording-pipeline.md` | transpose | reference/how-to | KEEP-R | Markup rules (`//!` vs `///`, `\omit`, `\expos`) | — |
| `docs/transpose/typeclass-object-a-new-extension-point.org` | transpose | explainer | KEEP-E | — | — |
| `docs/transpose/typeclass-object-pattern-in-cpp.org` | transpose | explainer | KEEP-E (fix `apply` row `:523`) | — | — |
| `docs/transpose/box-traits-and-typeclass-objects.org` | transpose | explainer | KEEP-E (fix `:569`) | — | — |
| `docs/transpose/bringing-typeclass-operations-into-scope.org` | transpose | explainer | KEEP-E | — | — |
| `docs/transpose/operation-objects-and-the-lookup-modes.org` | transpose | explainer | KEEP-E (fix link `:1036`) | — (decisions in `operation-objects`, `empty-predicate-name`) | 2026-10-11 |
| `docs/transpose/mrdocs.yml` | transpose | stale tool config | REMOVE | — | — |
| `docs/transpose/metrics/fanout-runs.jsonl` | transpose | metrics | REMOVE | — | 2026-09-07 |
| `docs/transpose/review/execution-stage-prompt.md` | transpose | agent prompt | REMOVE | — | — |
| `docs/transpose/review/execution-baseline.md` | transpose | stage review | REMOVE | — (all five divergences logged under slugs) | 2026-09-13 |
| `docs/transpose/review/execution-sender-registration.md` | transpose | stage review | REMOVE | — | 2026-09-13 |
| `docs/transpose/review/execution-all-of-algorithm.md` | transpose | stage review | REMOVE | TSan/allocation-count TU split rationale (`:90-103`) — also in `erasure-boundary` Log | 2026-09-14 |
| `docs/transpose/review/execution-collect-hook.md` | transpose | stage review | REMOVE | — | 2026-09-19 |
| `docs/transpose/review/execution-transpose-receipts.md` | transpose | stage review | REMOVE | Blog post `.md` and transclusion left unfinished (`:91-121`) — status, not decision | 2026-09-20 |
| `docs/transpose/review/grading-fidelity-review.md` | transpose | review | REMOVE | — (A-1, A-2, B-1, B-2 all became slugs/Log entries; D-1..D-5 drift fixed in `decisions.md`) | 2026-08-30 |
| `docs/transpose/review/p3200-reference-implementation-review.md` | transpose | review | REMOVE (check two items) | Medium findings "short-circuit specification internally inconsistent" (`:270-297`) and "array default-constructs results" (`:246-268`) have no visible slug resolution | 2026-09-11 |
| `docs/transpose/review/prior-art-when-all-range.md` | transpose | reference survey | UNSURE → KEEP-R | Cited *in place of* restatement by `all-of-failure-semantics` and `erasure-boundary` Logs; §2/§3/§6 would need folding into those slugs before removal | 2026-09-13 |
| `docs/tree_algorithms/README.md` | tree_algorithms | README | KEEP-R (rewrite) | — | — |
| `docs/tree_algorithms/DECISIONS.md` | tree_algorithms | decision record (numbered) | KEEP-D (convert to slugs) | — (it is the log; but `[S-3]` "ships in `include/`" at `:137` is now false) | 2026-10-12 |
| `docs/tree_algorithms/recursive-tree-algorithms.org` | tree_algorithms | explainer (article) | KEEP-E | — (carries two `# MEASURE:` flags `:383,390`) | — |
| `docs/tree_algorithms/references.bib` | tree_algorithms | reference | KEEP-R | — | — |
| `docs/tree_algorithms/notes/anchor-inventory.md` | tree_algorithms | reference (transclusion contract) | KEEP-R (fix `include/.../`) | — (skill says keep current) | 2026-10-12 |
| `docs/tree_algorithms/notes/extraction-plan.md` | tree_algorithms | plan | REMOVE after migration | §3 Foldable-superclass constraint (in skill); §5 "refold already is the direct fold" design insight + equivalence laws (in paper prose, not in DECISIONS); **§7 fixpoint compatibility checklist** (skill `:148-151` cites it as the living checklist); §7a essay venue/content; **§10 graded folds incoming obligation** (needs Steve; dangling `graded-fold-scope`) | 2026-10-12 |
| `docs/tree_algorithms/notes/punch-list.md` | tree_algorithms | worklist | REMOVE after migration | Resolved by design: "Required matrix is C++23-only; C++26 forward-compat is an allowed-to-fail advisory job, intentionally" (`:14`); open-needs-Steve: exemplar upstreaming, owner swap, clang-18 floor, modules ship/waive (`:19-29`) | 2026-07-17 |
| `docs/tree_algorithms/notes/representations-and-sequence-interfaces.md` | tree_algorithms | design note | UNSURE → KEEP-D (migrate §1, §4) | "Fix is a representation, not the interface" (`:24-46`); no-generic-in-order, pre/post-order generalize (`:73-85`); views carry a quotient contract (`:87-95`); **ruled** `fold_left` is Foldable's (`:163-174`); FringeTree `operator==` deferred (`:155-162`); placeholder names | 2026-07-17 |
| `docs/tree_algorithms/notes/article-reconciliation-memo.md` | tree_algorithms | memo | REMOVE | — (all dispositions executed in the `.org`) | — |
| `docs/tree_algorithms/notes/engineering-reality-sections.md` | tree_algorithms | drafts | REMOVE | — (Draft A is in the `.org` `:373-404`, Draft B in the paper §Costs) | — |
| `papers/blog/index.{md,org}` | transpose | README (series index) | KEEP-R | — | — |
| `papers/D3200R0.md` (front matter + sections) | transpose | paper | KEEP | — (`document: D3200R0`; sections include "Why not Monad", "`ap` as basis, `invoke` as interface", "Operation objects and the lookup modes") | — |
| `papers/algorithms-for-trees.md` (front matter + sections) | tree_algorithms | paper | KEEP | — (`document: D4322R0`, `date: 2026-07-17`; links `github.com/steve-downey/tree_algorithms`, retired) | 2026-07-17 |

Totals: 62 docs files + 7 others. KEEP-D 6 (+ hybrids), KEEP-E 10, KEEP-R 12, REMOVE 31, UNSURE 5.

---

## 3. Orphan decisions to migrate (source → proposed slug)

Grouped by target file (layout from §4b). "Status" is as the source states it.

### Cross-library (`docs/coordination/decisions.md`)

| # | Decision | Source | Status | Proposed slug |
|---|---|---|---|---|
| 1 | `ap` is a supported basis (`pure`+`invoke` or `pure`+`ap`, base derives the other); `invoke` is the only user-facing verb; `apply` is gone; simd `vec<callable>` argument | `SYNC.md:84-98,121-132`; `coordination-worklist-2026-07-13.md:89-128`; `provenance.md:77-87`; `SKILL.md:86-96` | DECIDED 2026-07-13, refined 2026-07-14 | `applicative-basis` |
| 2 | Monad deferred, not rejected; nothing proposed calls `bind`; every paper acknowledges the slot | `SYNC.md:100-119`; `SKILL.md:51-60` | DECIDED 2026-07-14 | `monad-proposal-status` |
| 3 | Repo surface ≠ paper surface; unproposed shipped code must be labelled evidence | `SYNC.md:151-158`; `extraction-plan.md:60-71`; `SKILL.md:61-66` | DECIDED 2026-07-13 | `unproposed-surface-labelling` |
| 4 | Fold family (`fold_map` + monoid) proposed in Paper D not A; Paper A proposes only Applicative/Traversable + mechanism | `SYNC.md:151-158`; `DECISIONS.md:113-144`; `extraction-plan.md:73-93` | DECIDED 2026-07-13 | `fold-family-ownership` |
| 5 | One repository: why reintegrated; layout `src/beman/<lib>`, `docs/<lib>`, `examples/<lib>`, flat `papers/`; namespaces unchanged; upstream repos retired, not synced back | `SYNC.md:46-82`; `README.md:28-33,35-47` | DECIDED 2026-10-12 | `repository-reintegration`, `repository-layout` |
| 6 | Tags carried as `<repo>/blog/...`; `import/<repo>/<branch>` for unmerged branches; `archive/cited-commits` keeps rebased-out commits reachable | `SYNC.md:65-71` | DECIDED 2026-10-12 | `retired-repository-history` (provenance) |
| 7 | One definition of the typeclass machinery, in beman.transpose; consumers link it; neither links the other | `machinery-deduplication.md:23-36`; `SYNC.md:6-24`; `SKILL.md:40-47` | DECIDED 2026-10-12 | `typeclass-machinery-home` |
| 8 | `overloaded` proposed by P3200, lives in transpose (duplicate entries today) | `decisions.md:3415-3442`; `machinery-deduplication.md:168-190` | DECIDED 2026-10-12 | `overloaded-home` (keep one) |
| 9 | Fallback (non-exhaustive) overload set: separate type, worse name, proposed only if required; candidate list; `overload_set` reserved; exhaustive/partial vs first-match/best-match terms | `machinery-deduplication.md:218-240` | DEFERRED 2026-10-12 | `fallback-overload-set` |
| 10 | fingertree takes transpose's names, no aliases; accepted `FingerTree<T>` type-identity change | `machinery-deduplication.md:158-166` | DECIDED 2026-10-12 | `fingertree-public-names` |
| 11 | Should the two BinaryTree ports unify? Leave (inverts build order) | `machinery-deduplication.md:210-217`; `SKILL.md:159-164` | OPEN | `binary-tree-ports` |
| 12 | Is `layer_fold` a Foldable instance by another name? | `machinery-deduplication.md:202-209` | OPEN | `layer-fold-and-foldable` |
| 13 | Public text stands alone — no internal identifiers in papers/posts | `SKILL.md:72-80`; `CRITIQUE-transpose.md:77-87` | DECIDED 2026-07-17 | `public-text-identifiers` |
| 14 | Paper files named by name, numbers in front matter only | `SKILL.md:182-187` | DECIDED 2026-07-17 | `paper-file-naming` |
| 15 | Everything publishes on sdowney.org; repo blog dirs are drafting locations | `STATUS.md:40-42`; `SYNC.md:134-141`; `SPEC.md:8`; `PLAN.md:94-97` | DECIDED 2026-07-13 | `publication-venue` |
| 16 | Typeclass-instance contract (shape unspecified, call-position validity, trivially copyable, by value, not a resource) — amended by `monoid-selection` on state | `CRITIQUE.md:768-993` | DECIDED 2026-07-20; AMENDED 2026-10-07 | `typeclass-instance-contract` |
| 17 | Derived operations are members (overridable); membership criterion "materially better implementation exists"; three-language table in paper | `CRITIQUE.md:891-904,997-1064` | DECIDED 2026-07-20 | `derived-operation-membership` |
| 18 | No recommended-practice notes in wording; rationale to paper, guarantees to wording | `CRITIQUE.md:953-993` | DECIDED 2026-07-20 | `wording-rationale-placement` |
| 19 | Pre-concede `-O0` cost, stratified (frames vs ABI); drop P3294 timing argument | `CRITIQUE.md:706-740,783-793` | DECIDED 2026-07-20 | `debug-build-cost-framing` |
| 20 | Toolchain floors: tree_algorithms gcc-14/clang-18 vs clang-18 CI failure vs transpose's measured cliff; required matrix C++23-only, C++26 advisory | `DECISIONS.md:40,198-200`; `punch-list.md:14,28`; `decisions.md:2845-2873`; `README.md:58-60` | OPEN (collides) | `toolchain-floor` (absorb `execution-toolchain-floor`), `cpp26-ci-policy` |
| 21 | Repository owner (steve-downey vs bemanproject org) | `punch-list.md:21-27`; `STATUS.md:12` | OPEN | `repository-owner` |
| 22 | Modules: ship, advisory, or waive | `punch-list.md:29`; `machinery-deduplication.md:139` | OPEN | `module-build-status` |
| 23 | Blog collection positioning and Monad-scarcity thesis; reflection-monad instance in fixpoint's orbit | `PLAN.md:6-15,42-55,89-93` | DECIDED 2026-07-21 | `blog-collection-positioning` |
| 24 | "front door" is not the house idiom; replace, do not thin | `CRITIQUE-transpose.md:92-97` | DECIDED 2026-07-21 | `house-idiom-front-door` (or fold into a style doc) |
| 25 | No structural grading work in fingertree; it is a client via Traversable | `transpose-grading-plan.md:368-394` | DECIDED (plan) | `fingertree-grading-scope` |

### tree_algorithms (`docs/tree_algorithms/decisions.md`)

| # | Decision | Source | Status | Proposed slug |
|---|---|---|---|---|
| 26 | `fold_left` is Foldable's derived operation, one definition from `fold_map`; trees reach it by being Foldable | `STATUS.md:27-29`; `representations-…md:163-174` | DECIDED 2026-07-17 | `fold-left-home` |
| 27 | Sequence-quotient surface status (`view_l`/`view_r`, five-op interface), "quotient" as vocabulary, FringeTree `operator==`, placeholder names | `STATUS.md:21-29`; `representations-…md:143-190` | OPEN ([S-6]) | `sequence-quotient-surface`, `quotient-vocabulary`, `fringe-tree-equality` |
| 28 | No generic in-order; pre/post-order generalize; order is the layer's contract | `representations-…md:73-85`; `recursive-tree-algorithms.org:502-527` | DECIDED (design) | `generic-traversal-order` |
| 29 | Fix is a representation, not the interface; `fold_fix` is `fold_with` at `unwrap_fix` | `representations-…md:24-46`; `extraction-plan.md:160-211` | DECIDED (design) | `fix-as-representation` |
| 30 | Fixpoint compatibility checklist (new surface stays gcata/gana-compatible) | `extraction-plan.md:260-279`; `SKILL.md:148-151` | standing rule | `catalog-compatibility` |
| 31 | Graded folds: scope question incoming from P3200 (`fold-grade-invariant` … `fuel-axis`) | `extraction-plan.md:395-452`; `transpose-grading-plan.md:337-366` | OPEN (needs Steve) | `graded-fold-scope` (the slug the plan already cites but never created) |

### transpose (`docs/transpose/decisions.md`)

| # | Decision | Source | Status | Proposed slug |
|---|---|---|---|---|
| 32 | Monad alternate complete bases (`pure`+`fmap`+`join`, `pure`+`kleisli`); lazy-sender acceptance tests; "no derivation may flatten on the caller's behalf" | `coordination-worklist-2026-09-07.md:186-260` | HELD (contingency) | `monad-alternate-bases` |
| 33 | Functor combinators (Identity/Const) as evidence; HKT via reflection far horizon | `coordination-worklist-2026-09-07.md:262-303` | HELD | `higher-kinded-lookup` |
| 34 | Tree Applicative default semantics monad-derived; zip alternate; range = nondeterminism | `CODING_RULES.md:10-15` | rule | `tree-applicative-default-semantics` |
| 35 | `all_of` is not proposed; a range `when_all` belongs to the execution papers | `transpose-execution-plan.md:939-942`; `execution-transpose-receipts.md:166-169` | DECIDED (plan) | `all-of-proposal-status` |
| 36 | Two unresolved review findings: short-circuit wording consistency; array default-construction | `p3200-reference-implementation-review.md:246-297` | unknown | check before deleting the review |

### fingertree (`docs/fingertree/decisions.md`, new)

Seed from `allocator-design.md` §1–§8 and `compile-time-design.md`: `allocator-template-parameter`, `allocator-covered-allocations`, `mixed-allocator-combination`, `allocator-equality-detection`, `uses-allocator-constructor`, `allocator-propagation-traits`, `append-allocator-fast-path`, `spine-shell-allocation`, `spine-element-representation`, `measure-monoid-parameterization`. The explainer prose stays in the design docs; the decision entries cite them.

---

## 4. The slug plan

### 4a. Slugs for every numbered identifier

**tree_algorithms `DECISIONS.md` (question-named; the current heading names the answer in six of eight cases):**

| Old | Proposed slug | Notes |
|---|---|---|
| Decision 1 — Scope: minimal core | `proposal-scheme-set` | Log: Amendment 2026-07-14 widened it (fold family) |
| Decision 2 — Naming | `recursion-scheme-verb-names` | Discrepancy "fixpoint still ships `cata`" → Log |
| Decision 3 — Box | `knot-indirection-type` | Amendment 2026-07-18 → Log ("Box remains the knot storage") |
| Decision 4 — explicit `fmap` + lookup header | `primary-verb-fmap-source` and `lookup-tier-home` | Amendment 2026-10-12 → Log of `lookup-tier-home`; Discrepancy "D5 citation" → Log |
| Decision 5 — composition not inheritance | `fix-layer-relationship` | |
| Decision 6 — value semantics / non-goals | `layer-ownership-semantics` | Amendment 2026-07-18 narrows "performance is a non-goal" → Log |
| Decision 7 / DEV-01 | `test-algebra-discrimination` | repo-wide test rule → cross-library file |
| Decision 7 / DEV-03 | `wrapped-type-ctad` | repo-wide |
| Decision 7 / DEV-04 | **merge into existing `derived-op-constant-evaluation`** | same question, transpose already answered it 2026-10-12 (`decisions.md:3445-3470`) |
| DEV-02 (N/A) | `distributive-law-element-types` | Status NOT-APPLICABLE; one paragraph |
| Decision 8 — copier answer sheet | `repository-generation-record` | provenance, not a decision; or move to `provenance.md` |
| Amendment 2026-07-14 | `fold-family-ownership` (cross), `direct-verb-names`, `fold-map-monoid-parameter`, `fringe-tree-location`, `lookup-object-set` | five questions in one amendment |
| Amendment 2026-07-17 | `tree-algorithms-paper-number` | provenance |
| Amendment 2026-07-18 | `materialized-layer-storage` | the `child_slot` finding with the measurements |
| Amendment 2026-10-12 | `lookup-tier-dependency` + `explicit-tier-isolation` + Log entries on `lookup-object-set` | |
| Discrepancies section | Log entries under the four slugs it names | |

**`[S-n]`:** S-1 → `direct-verb-names`; S-2 → `fold-map-monoid-parameter`; S-3 → `fringe-tree-location` (record 2026-10-12 move to `src/` as a Log entry); S-4 → `publication-venue`; S-5 → `tree-algorithms-paper-number` (resolved half) + `repository-owner`, `toolchain-floor`, `module-build-status` (open halves); S-6 → `fold-left-home` (ruled) + `sequence-quotient-surface`, `quotient-vocabulary`, `fringe-tree-equality` (open).

**`machinery-deduplication.md`:** Questions keep their slugs (`fingertree-public-names`, `consumer-lookup-names`, `layer-fold-and-foldable`, `binary-tree-ports`, `fallback-overload-set`; `overloaded-home` merged). Stages become history, not slugs: `ruling-before-code`, `fingertree-onto-transpose`, `transpose-constexpr-derivations`, `tree-algorithms-functor-onto-transpose`, `close-the-ledger` → the Outcome table (commit SHAs) goes into `SYNC.md`/history; `explicit-tier-isolation` is the one stage that is a standing decision (a build check exists) and gets an entry.

**`SYNC.md` entries:** see §3 rows 1–7. The SHA tables stay in `SYNC.md` renamed/retitled as history.

**Fixpoint `D1–D11` / `DEV-01..04` as cited in this repo:** replace each citation with the fixpoint repo path *and the rule in words* (e.g. "fixpoint's rule that distributive laws are variable templates on `F`"); this repo should not mint slugs for another repo's decisions. `SKILL.md:27` and `:76` are the two live sites; `DECISIONS.md:9-10,33,44,55,75,190-200` go away with the conversion.

### 4b. Target layout — options and recommendation

| Option | Pros | Cons |
|---|---|---|
| A. One file per library (status quo, plus a fingertree log) | Scoped by paper; small diffs; blame stays meaningful | Cross-library rulings have two homes (already happened: `overloaded-home` ×2); uniqueness needs a cross-file check anyway; SYNC/STATUS rulings have no home |
| B. One repo-wide `docs/decisions.md` | Uniqueness trivial; one grep; one reading order | ~5,000 lines on day one (transpose alone is 3,470); every library's reviewer wades through the others'; paper-scoped extraction for implementation-experience sections harder |
| C. Per-library files **plus** `docs/coordination/decisions.md` for cross-library rulings, one shared slug namespace | Each entry has exactly one home chosen by "which paper's text would change if this flipped"; division-of-labor, naming, test discipline, layout, floors live where the skill already looks; uniqueness enforced by a 15-line pre-commit check | Readers need the index (`docs/README.md`); an entry whose scope widens moves files (keep slug; leave a redirect stub, which `decisions.md` already does for WITHDRAWN entries) |

**Recommendation: C.** Concretely: `docs/transpose/decisions.md` (as is), `docs/tree_algorithms/decisions.md` (converted from `DECISIONS.md`, lower-case to match), `docs/fingertree/decisions.md` (new), `docs/coordination/decisions.md` (new; receives §3 rows 1–25 and the skill's "frozen" bullets as entries, so the skill becomes a map that *links* rather than a second copy). Placement rule written in each file's header: "an entry lives in the library whose paper would change; a ruling about two or more papers, the repository, or the process lives in coordination".

### 4c. Slug naming convention

- kebab-case, ASCII, 2–5 words, a noun phrase naming the **question**, never the answer (`decisions.md:3-5` already states this: `#empty-grade-spelling`, not `#bare-t`). Test: the slug must still read correctly if the decision is reversed.
- No library prefix by default; the file is the scope. Use a qualifier only to disambiguate a genuinely repeated question (`traverse-policy-surface` vs a future `fold-policy-surface`).
- No numbers, dates, letters, or abbreviations (`DEV`, `S`, `MF`, `D4322`, `R0`) in slugs. Dates go in `Status`/`Log`.
- Uniqueness is repo-wide across the four files, checked by pre-commit: every `^## [a-z0-9-]+$` heading across `docs/*/decisions.md` unique; every `decisions.md#slug` link resolves to one of them.
- Stability: a slug is never renamed or deleted. Reversal is a dated Log entry and a Status change in place (the log's current practice). A *different* question gets a new slug; the old one's Status becomes `SUPERSEDED by [new-slug]` and the new one's Decision opens "Supersedes [old-slug]". Withdrawn drafts keep a redirect stub (`execution-dependency-shape`, `demo-sender-fate` are the model, `decisions.md:2094-2143,2732-2766`).
- Statuses: `OPEN`, `PROPOSED`, `DECIDED yyyy-mm-dd`, `SUPERSEDED`, `WITHDRAWN`, `NOT-APPLICABLE`. Multi-part entries say which part is which (`collect-hook` is the model).
- Headings: the entry heading is the bare slug (GitHub auto-anchor = slug). No `{#…}`, no prose on the heading line.

### 4d. Entry template

```markdown
## slug-naming-the-question

**Question:** One sentence, phrased so either answer fits.
**Status:** DECIDED 2026-10-12   (or OPEN / PROPOSED / SUPERSEDED by [x](#x) / WITHDRAWN / NOT-APPLICABLE)
**Decided by:** Steve Downey, 2026-10-12   (or "default-as-drafted, stage …, not individually ruled")
**Scope:** transpose | tree_algorithms | fingertree | cross-library   (which paper's text changes if this flips)
**Context:** Two to five sentences: what forced the question, what was measured.
**Decision:** The ruling, in full, including any parts (1., 2., …).
**Why:** The load-bearing reasons; a later reader should be able to tell when a reason has stopped being true.
**Consequences:** What changed or must change: code sites, paper sections, wording, tests, other slugs amended.
**Sentinel:** (optional) the test or grep that fails if this is violated.
**Sources:** files/commits/SHAs this was recorded from (for migrated entries: the retired path and date, e.g. `tree-coordination SYNC.md entry 5, 2026-07-14`).
**Cross-refs:** [other-slug](#other-slug), [cross-library-slug](../coordination/decisions.md#cross-library-slug)
**Log:**
- 2026-07-13 — Raised …
- 2026-07-14 — Ruled …
- 2026-10-12 — Amended by [x](#x) …
```

This is `decisions.md`'s existing shape (`:11-12`) plus `Scope`, `Consequences` (already used informally), `Sources` and `Cross-refs` made explicit. Migrated entries must carry `Sources` so the deleted file's provenance survives.

### 4e. Migration checklist

1. Create `docs/coordination/decisions.md` and `docs/fingertree/decisions.md` with the header convention; rename `docs/tree_algorithms/DECISIONS.md` → `decisions.md` (update the one inbound anchor, `extraction-plan.md:33`, and `SKILL.md:134`).
2. Convert `DECISIONS.md` entry by entry per §4a; fold Amendments and Discrepancies into Logs; drop the fixpoint `D`/`DEV` citations in favour of path + rule-in-words.
3. Write the 31 orphan entries of §3 from their sources, each with `Sources:` naming the file about to be deleted. Resolve `overloaded-home` to one file; resolve the F9 statefulness conflict in `typeclass-instance-contract`'s Log; fix the F6 paper-number/floor collisions with `toolchain-floor` and a paper-numbers provenance entry.
4. Add the pre-commit check (unique `## slug` across `docs/*/decisions.md`; every `decisions.md#…` link resolves; no `{#` in docs).
5. Replace identifier references. Grep results on this checkout (`grep -rnE` over `docs .claude papers/*.md papers/blog src examples README.md`, excluding `papers/wg21/`):
   - `Decision [0-9]` — **43** hits: `DECISIONS.md` 23, `extraction-plan.md` 3, `engineering-reality-sections.md` 2, `machinery-deduplication.md` 2, and **10 in code**: `src/beman/tree_algorithms/{recursion_schemes.hpp,recursion_schemes_lookup.hpp,fold_map_lookup.hpp,binary_tree.hpp,rose_tree.test.cpp,fringe_tree.test.cpp,expression.test.cpp,binary_tree.test.cpp}`, `examples/tree_algorithms/{search_tree_on_fix,expression_algorithms}.cpp`; plus 1 each in `representations-…`, `punch-list`, `article-reconciliation-memo`.
   - `DEV-0` — **65** hits: `DECISIONS.md` 10, `SKILL.md` 5, `extraction-plan.md` 6, `representations-…` 3, `punch-list` 2, `MUSTFIX` 2, `CRITIQUE-fixpoint` 1, `machinery-deduplication` 1, `decisions.md` 1, `CLAUDE.md` 1, and **39 in code**: `recursion_schemes.test.cpp` 7, `binary_tree.test.cpp` 5, `recursion_schemes_lookup.test.cpp` 4, `rose_tree.test.cpp` 3, `functors.test.cpp` 3, `fringe_tree.test.cpp` 3, `expression.test.cpp` 3, `fold_map.test.cpp` 2, three examples 1 each.
   - `\[S-[0-9]` — **17** hits: `STATUS.md` 11, `SYNC.md` 5, `essay/SPEC.md` 1.
   - fixpoint `D1`–`D11` in fixpoint/design context — **14** hits: `DECISIONS.md` 9 (`:9,33,44,55,75,190,191,199,200`), `extraction-plan.md:279`, `SKILL.md:27,76`, `MUSTFIX.md:134`, `CRITIQUE-fixpoint.md:20`.
   - "SYNC entry" — **17** hits: `machinery-deduplication.md` 5, `STATUS.md` 5, `README.md`(coord) 1, `SKILL.md:77` 1, `DECISIONS.md:116` 1, `extraction-plan.md` 1, `coordination-worklist-2026-07-13.md:15` 1, `EDIT-LETTER.md` 1, `MUSTFIX.md` 1.
   - also: `§`-section references **128** (nearly all internal to the notes/plans being deleted; the live ones are `SKILL.md:148` "extraction-plan.md §7" and `DECISIONS.md:117`).
   For code comments: state the rule in words (the skill's own MF-4 guidance, `MUSTFIX.md:131-139`) — e.g. "// test algebra is order-sensitive on purpose: a commutative one cannot see a wrong traversal" — and add the slug only outside transcluded anchor regions.
6. Fix F4/F5/F7/F8 stale paths and vocabulary in the KEEP set (READMEs, `CODING_RULES.md`, `typeclass-object-pattern.md`, the two `.org` tables, `anchor-inventory.md`, `SKILL.md`); add a header note to `decisions.md` that pre-2026-10-10 Logs use `include/`, `*_typeclass`, `monoid_v` historically.
7. Retitle `SYNC.md` as the coordination history (SHA pins, reintegration record, dedup Outcome table) and stop appending decisions to it; `STATUS.md`, `CLAUDE.md`, worklists, plans, reviews, critiques, memos, `fork-essay.org`, the `.html`, `mrdocs.yml`, `fanout-runs.jsonl`, the addendum: delete. Keep `prior-art-when-all-range.md` and `anchor-inventory.md` as reference (or fold the former into its two citing slugs first).
8. Move the two essays (`trees-and-transposition-for-29.org`, `where-are-the-monads.org`) to `papers/blog/` so every published text has one home; regenerate exports there.
9. Run the link check again; run `grep -rnE 'Decision [0-9]|DEV-0|\[S-[0-9]|SYNC entry'` and expect zero outside `SYNC.md` history.

---

## 5. Reader-facing docs map after end of month

```
README.md                         what the three libraries are, build, where to read next (links below)
docs/README.md                    NEW — the map: one paragraph per library, the papers, "decisions live in
                                  docs/<lib>/decisions.md and docs/coordination/decisions.md", how to read a slug,
                                  the explainers, the invariants (link to the skill or inline them)
docs/coordination/
  README.md                       the map of papers A/C/D, fixpoint and trees as external evidence (no ~/src paths),
                                  the invariants (division of labour, naming, test discipline) — one copy, the skill links here
  decisions.md                    cross-library rulings (§3 rows 1–25), slugged
  SYNC.md  (or history.md)        SHA pins, reintegration record, dedup outcome table — history, append-only
  essay/                          gone; the essays live in papers/blog/
docs/transpose/
  README.md                       rewritten from the root README's facts; headers list from src/beman/transpose
  decisions.md                    as today, plus rows 32–36
  provenance.md                   kept
  wording-pipeline.md             kept
  CODING_RULES.md, codestyle.org  kept, de-duplicated against each other (one layout rule), apply→ap fixed
  typeclass-object-pattern.md + the five .org explainers   kept; or promoted to papers/blog when published
docs/tree_algorithms/
  README.md                       rewritten
  decisions.md                    converted from DECISIONS.md, plus rows 26–31
  recursive-tree-algorithms.org, references.bib            kept
  anchor-inventory.md             kept (transclusion contract), paths fixed
docs/fingertree/
  README.md                       rewritten (today a copier stub)
  decisions.md                    NEW, seeded from the design docs
  allocator-design.md, compile-time-design.md, measure-based-splitting.md   kept as explainers
papers/blog/
  index.org/.md + existing posts
  trees-and-transposition-for-29.org, where-are-the-monads.org   moved here
.claude/skills/tree-proposals-coherence/SKILL.md
  map + procedures + hazards; every "frozen" bullet becomes a link to a slug instead of a restatement
```

Removed: `STATUS.md`, `CLAUDE.md` (coordination), `machinery-deduplication.md` (Questions migrated, Outcome to history), `blog/{PLAN,MUSTFIX,CRITIQUE-*}.md`, `essay/{SPEC,CRITIQUE,EDIT-LETTER,DRAFT-REVIEW-2026-08-06,PUBLISHING-NOTES}.md`, `fork-essay.org`, `*.html`, both coordination worklists, both transpose plans, `decisions-execution-addendum.md`, `baseline-vocabulary-audit.md`, `review/*` (except `prior-art-when-all-range.md` if not folded), `mrdocs.yml`, `metrics/`, `notes/{extraction-plan,punch-list,article-reconciliation-memo,engineering-reality-sections,representations-and-sequence-interfaces}.md` — each only after its §3 rows have landed with `Sources:` lines.
