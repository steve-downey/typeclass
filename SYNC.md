# SYNC ledger

Pins each repo's state at each coordination decision. Append-only;
newest entry first. Each entry is headed by its slug (the link anchor) with
the date on the line below; refer to entries by slug, never by position.

## transpose-grading-landed

**2026-09-10** — P3200's grading campaign landed; tree_algorithms re-checked against it.

No new ruling. This entry pins eight weeks of beman.transpose work that
went in after [ap-basis-invoke-interface](#ap-basis-invoke-interface),
and records the cross-repo checks run afterwards.

**What landed** (transpose `main` at 6366591, 2026-09-06): the grading
campaign — a compile-time grade index for effectful computations,
`error_set<E...>` (nominal, normalized, a bounded join-semilattice under
union), a graded `std::expected` model, and a decision log at
`docs/decisions.md` in the question-slug form this repo has since
adopted. `traverse` gained a trailing defaulted applicative-object
policy parameter (short-circuiting vs accumulating) on the `ranges`
`comp = {}, proj = {}` precedent. D3200R0 gained a "Coherent grading for
fallible contexts" rationale section and three wording clauses
(`[transpose.grade]`, `[transpose.errset]`, `[transpose.expected]`),
generated from the headers by beman.specgen into `papers/wording/`. The
paper also moved out of the vendored MPark.WG21 tree: it is
`papers/D3200R0.md` now, not `papers/wg21/D3200R0.md`.

**Checks run against tree_algorithms** (main at da3d59f; 77/77 green,
every UUID anchor in its inventory still paired in source):

- Frozen naming holds. `invoke` is still the interface and `ap` the
  basis, with no `apply` verb on the surface; `traverse`/`transpose` and
  `traverse_with`/`transpose_with` are intact, so Paper D's `_with`
  rationale still cites something true; `monoid_identity`/
  `monoid_combine` and the lookup variable templates are unchanged.
- `docs/typeclass-object-pattern.md` was not touched, so the pattern
  Paper D's convenience headers mirror has not moved.
- The Traversable-must-not-require-Foldable pin is still in
  `traverse.hpp`, comment and all.
- The two-BinaryTree hazard is clear: transpose's
  `examples/binary_tree.hpp` changed by clang-format only.
- Division of labor improved rather than drifted: D3200R0's non-goals
  now name Foldable, attribute the fold family to the companion paper,
  and label `fold.hpp`/`monad.hpp` as evidence. Two claims in
  tree_algorithms' extraction plan ("D3200R0 mentions neither", "never
  mentions Foldable") were stale as a result, and are updated in place
  with dated notes (476cc66).

**What it opens.** transpose's grading plan carries a section addressed
to tree_algorithms — fold grades shape-independent under a bounded
join-semilattice, two fold entry points (monadic and applicative, never
one that silently picks), widen once, recovery annotated at the fold
boundary, fuel as a perpendicular axis. It was gated on stages that have
all now shipped. Recorded as §10 of
`tree_algorithms/docs/notes/extraction-plan.md`; the scope question is
[graded-fold-scope](DECISIONS.md#graded-fold-scope).

**In flight, not landed** — before anything downstream mirrors the
mechanism: transpose branches `typeclass-operation-cpos` (2026-08-02)
and `typeclass-instance-grounding` (2026-09-08). The first would change
the lookup pattern Paper D's convenience headers consume; the
`traverse-policy-surface` decision in transpose's log was taken
specifically to keep a CPO `traverse` possible.

**Correction, same morning.** The local transpose checkout was 43
commits stale when the above was written, and one of the two branches
called in flight had already merged: `typeclass-instance-grounding`
(PR #46, transpose main now 26f6ab8, 2026-09-08). `typeclass-operation-cpos`
is still open, 7 commits ahead. What merged is repo surface, and labeled
as such — the paper, the examples, and `docs/typeclass-object-pattern.md`
are untouched by all 43:

- Foldable's evidence surface grew a **mutually-derivable basis pair**:
  an Impl now provides `fold_map` *or* `fold_right`, the base derives
  the other, in the shape `Applicative::invoke` already models. Paper D
  proposes `fold_map` alone; the second basis is an input to D's
  Foldable wording when it is written, not a change to it.
- Named monoid carriers (`Sum`, `Product`, `Max`, `Min`, `Any`, `All`)
  and a new `induced_monoid.hpp`, the latter opening with
  EVIDENCE, NOT PROPOSED WORDING and the reason the categorical
  "monoid in the category of endofunctors" is inexpressible at
  `Monoid<T>`. `identity()`/`combine()` are unchanged, and the
  numeric-carrier default registrations are being retired in favour of
  choosing a named carrier.
- Deep object concepts (`traversable_object` and kin) that check a
  class's whole surface rather than probing one operation. The
  Traversable-not-Foldable pin survives verbatim and is now cited by
  the concept.
- A held backlog, `docs/coordination-worklist-2026-09-07.md`: four
  items from a 2026-09-07 design discussion, **all HELD, none
  scheduled**. Its item 2 would replace the Maps' `using`-selection
  with per-operation probing — the lookup pattern Paper D's convenience
  headers mirror. Watch it; do not mirror ahead of it.

| Repo | Branch | SHA | Notes |
|---|---|---|---|
| transpose/main | main | 26f6ab8 | grading + instance grounding; `typeclass-operation-cpos` still open |
| tree_algorithms/main | main | 476cc66 | extraction plan §10 + the stale-claim fixes |
| tree-coordination | todo | this entry | board and coherence skill updated alongside |

## essay-exhibit-pins

**2026-09-10** — essay drafted; exhibits pinned by tag in three repos.

The relationships essay exists as a draft: `essay/trees-and-transposition-for-29.org`
("Trees, Transposition, and a new Container for C++29"), revised
2026-09-05..07 against `essay/EDIT-LETTER.md` and merged to this
repo's main via PR #1 (dfcf3d9). It departs from `essay/SPEC.md`
(2026-07-14 revision):
prose-led rather than exhibit-led, three exhibits not six, the map is
a prose section ("Why four papers", the discovery story reversed)
rather than a diagram plus table, and P4307 (backtick) is carried as a
fourth paper with equal standing. SPEC is not updated; whether it
is superseded or the exhibit essay is still a separate piece is open
(STATUS, essay section, the SPEC reconcile item).

Decisions recorded in the draft's revision board: instance contract
resolved 2026-07-20 (shape deliberately unspecified; P3294 timing
argument dropped); backtick integrated, not a bonus track; this post
is the de facto map. Still open: standalone "why in the Standard"
post, the "I can have" reframe, the call to action.

Exhibit pins. Each exhibit is an `orgit-file` transclusion pinned to an
annotated tag `blog/trees-and-transposition-for-29` in its source repo;
all three tags are on GitHub. New UUID anchors were added for it, both
comment-only commits on main:
- tree_algorithms da3d59f: anchor 78958c3f around the shape algebra and
  `fold_with` call in `examples/nonce_tree_direct.cpp`; inventory row
  added. exhibit-yesterdays-tree = 04e450a4 (existing) + 78958c3f.
- fingertree bb48221: anchor 78d295c0 around build / `split_at_measure`
  / original-intact in `examples/finger_tree.cpp`. exhibit-measured-split
  (the draft had labelled it as the SPEC's second exhibit; the slug
  removes that mismatch).
- transpose: tag at bb32a37, the revision the published
  transpose-at-work post is pinned to; exhibit-one-verb-three-contexts =
  anchors 05ce0358,
  680950c9, e916da84 in `examples/transpose_post_example.cpp`.

Publication path is the wctm site repo (per [publication-venue](DECISIONS.md#publication-venue)), whose org compiler
resolves pins from GitHub by repo name. As of this entry wctm has the
repo-table entries for tree_algorithms and fingertree and a copy of the
post, all uncommitted (see table).

| Repo | Branch | SHA | Notes |
|---|---|---|---|
| tree-coordination | main | dfcf3d9 | PR #1 merged; `.md` and `.html` exports committed (e34096b) |
| transpose/main | main | 6366591 | tag blog/trees-and-transposition-for-29 → bb32a37; clean |
| tree_algorithms/main | main | da3d59f | tag → da3d59f; + modified `docs/notes/extraction-plan.md` |
| fingertree/fingertree | main | bb48221 | tag → bb48221; clean |
| fixpoint/main | main | 17077b9 | + modified `docs/blog/part-{1,2,3}-*.md`, untracked `test.png` (MUSTFIX pass in progress) |
| trees/main | main | 43d7106 | clean |
| wctm | main | 628e752 | + modified `plugins/orgmode/conf.el`, `projects.eld`; untracked `posts/trees-and-transposition.org`, `.meta` |

## ap-basis-invoke-interface

**2026-07-14** — settled: ap is a basis, invoke is the interface.

Two refinements from Steve landed the pure/apply ruling in its final
form. First, one-step application is a reasonable implementation detail
(not banished from implementation space); second, `ap` is "a perfectly
cromulent basis function for Applicative and should be supported that
way." Final design, in beman.transpose main at b77b4cd (54/54 tests):
instances opt in with `pure` + `invoke` OR `pure` + `ap`; the base
derives the missing direction (currying confined to the derivation);
`ap` is also a secondary derived operation where the context can hold
callables; the user-facing verb everywhere is n-ary `invoke`; the name
`apply` stays gone. The binary tree example is restored as the living
ap-basis exemplar. D3200R0's section is now "`ap` as basis, `invoke`
as interface." Skill invariant updated (third and final wording);
memory updated.

## monad-deferred-not-rejected

**2026-07-14** — Monad framing refined: deferred, not rejected.

Ruling (Steve): the Why-not-Monad argument must not read as rejection.
The typeclass design leads to a consistent generic name for sequential
composition eventually (per-type members are not generic — that is the
gap typeclasses close), and `monad.hpp` proves the mechanism carries it;
Monad is unproposed only because nothing currently proposed calls
`bind`. "For now." Reframed in: D3200R0 "Why not Monad", monad.hpp
header note, Paper D companion text, skill (both copies), transpose
worklist item 1, essay SPEC exhibit-holes, trees addendum, provenance.

Also this session (transpose repo, branch remove-pure-apply-forms in
`~/src/transpose/apply-removal`): pure/apply removal executed per
[apply-forms-removed](#apply-forms-removed) — apply/ap/terminating_partial/applicative_eval deleted, Monad's
synthesized apply removed, dual-core instances (zip_list, array,
simd_lanes) reduced to the invoke core, laws restated invoke-form,
absence of apply forms pinned per instance, fold.hpp/monad.hpp evidence
labels, Traversable no-Foldable-superclass pin, D3200R0 gains "Why not
apply"/"Why not Monad" + non-goals, provenance entry. Build/test pending
merge.

## apply-forms-removed

**2026-07-13** — pure/apply forms removed from the design.

Ruling (Steve): the classic `apply` formulation (callable-in-context,
curried application) is a Haskell artifact and a misunderstanding in
C++; all such forms (`apply`, `ap`, currying helpers, monad-synthesized
`apply`) are removed from public surfaces. Applicative primitives are
`pure` + n-ary `invoke`; laws restated in invoke form. Epicenter:
transpose `apply.hpp` + instance headers + `monad.hpp`; mirrors in
fingertree `typeclass/applicative.hpp` and trees/main pedagogy Part 6;
fixpoint sweeps opportunistically. tree_algorithms unaffected
(functor-only surface). Work scheduled in transpose worklist item 5;
repo SHAs unchanged from [direct-verbs-implemented](#direct-verbs-implemented).

## direct-verbs-implemented

**2026-07-13** — four decisions resolved; direct verbs implemented.

Decisions (Steve): [direct-verb-names](DECISIONS.md#direct-verb-names): direct verbs are
`fold_with`/`unfold_with`; [fold-map-monoid-passing](DECISIONS.md#fold-map-monoid-passing): `fold_map` primary spelling takes combine + identity as plain
parameters (no Monoid trait in the primary API); [fringe-tree-location](DECISIONS.md#fringe-tree-location):
fringe tree goes in `include/`; [publication-venue](DECISIONS.md#publication-venue): everything publishes on sdowney.org — repo blog
dirs are where code and text come together, so drafting location is
logistics, not a venue decision.

Work: `fold_with`/`unfold_with` implemented in tree_algorithms
`recursion_schemes.hpp` with equivalence-law static_asserts, nonce-tree
tests (move-only `unique_ptr` carrier), adapter-vs-direct agreement law,
and `examples/nonce_tree_direct.cpp`. Also fixed a pre-existing
fetch-content configure defect (Catch2 extras/module-path recovery in
`tests/beman/tree_algorithms/CMakeLists.txt`). All on branch
worktree-bright-ray-n44x, uncommitted.

## fold-family-to-paper-d

**2026-07-13** — fold family to Paper D; Monad acknowledged out-of-band.

Decisions: (1) `fold_map` + monoid proposed in Paper D, not Paper A;
Paper A proposes only Applicative/Traversable + mechanism. (2) Monad
stays unproposed; acknowledged via out-of-band relationships essay;
D3200R0 gains a "Why not Monad" section. (3) Repo surface ≠ paper
surface principle adopted. (4) Paper D grows direct (non-Fix) verbs as
project/embed wrappers over `refold`.

| Repo | Branch | SHA | Notes |
|---|---|---|---|
| trees/main | main | 7830759 | + untracked `docs/notes/standardization-plan-addendum-2026-07-13.md` |
| transpose/transpose | main | 33f6c8a | + untracked `docs/coordination-worklist-2026-07-13.md` |
| fingertree/main | main | 0a1d79b | + untracked `docs/coordination-worklist-2026-07-13.md` |
| fixpoint/main | main | 8ad39f4 | no pending work; evidence repo complete (S00–S16) |
| tree_algorithms | worktree-bright-ray-n44x | 06646ff | + uncommitted `docs/notes/extraction-plan.md`, `.claude/skills/tree-proposals-coherence/` |
