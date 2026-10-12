# SYNC ledger

Pins each repo's state at each coordination decision. Append-only;
newest entry first.

## 2026-10-12 (entry 2) — machinery deduplication begins

Decision (Steve): collapse the copied typeclass machinery onto
beman.transpose, per
[machinery-deduplication.md](machinery-deduplication.md). Rulings:
fingertree's provisional names go without aliases
([fingertree-public-names](machinery-deduplication.md#fingertree-public-names));
tree_algorithms' lookup objects take the plain names `functor`,
`layer_fold`, `project`
([consumer-lookup-names](machinery-deduplication.md#consumer-lookup-names));
`overloaded` moves into beman.transpose and P3200 proposes it
([overloaded-home](../transpose/decisions.md#overloaded-home)); the
non-exhaustive overload set is deferred
([fallback-overload-set](machinery-deduplication.md#fallback-overload-set)).
Recorded first in tree_algorithms' DECISIONS.md (amendment
2026-10-12), its extraction plan, transpose's decision log and the
coherence skill, before any code moves.

Start state: beman.typeclass `reintegrate` at `f1d18e7`.

## 2026-10-12 — the three libraries merge into beman.typeclass

Decision (Steve): transpose, tree_algorithms and fingertree must stay in
sync where they layer on or reuse machinery and design, and three
repositories were drifting. They are reintegrated into beman.typeclass
with full history; the papers stay separate, all under `papers/`. The
upstream repositories, and this ledger's own, are retired, not synced
back.

Merged at, with SHAs unchanged (so every SHA cited above still
resolves here):

| Source | Tip merged |
|---|---|
| beman.transpose `main` | `fe57e7bd` |
| beman.tree_algorithms `main` | `476cc661` |
| beman.fingertree `main` | `7850388e` (origin; the local clone was 3 behind) |
| tree-coordination `main` | `c6209700` |

Unmerged branches came along as `import/<repo>/<branch>`: transpose's
`docs/voice-pass-and-series-nav`, `review-fixes-2026-08-07` and
`lean-probe-harness`; tree_algorithms' `allocator-awareness`. Thirteen
transpose commits cited in these documents had been rebased out of every
branch; `archive/cited-commits` keeps them reachable. Blog tags carry
the repository as a prefix: `transpose/blog/...`,
`tree_algorithms/blog/...`, `fingertree/blog/...`.

Layout: `src/beman/<lib>/` (headers beside their tests), `examples/<lib>/`,
`docs/<lib>/`, `benchmarks/tree_algorithms/`, `papers/` flat because
MPark.WG21 requires it, and this directory at `docs/coordination/`. The
namespaces are unchanged. Not yet done: fingertree's private copies of
the typeclass machinery (`typeclass/`, `fixpoint/overloaded.hpp`) and
tree_algorithms' (`functor.hpp`, `fix.hpp`, `overloaded.hpp`) still
duplicate transpose's; collapsing them onto one definition is the point
of the merge and the next piece of work.

From here on, pinned states are beman.typeclass SHAs.

## 2026-07-14 (entry 5) — settled: ap is a basis, invoke is the interface

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

## 2026-07-14 (entry 4) — Monad framing refined: deferred, not rejected

Ruling (Steve): the Why-not-Monad argument must not read as rejection.
The typeclass design leads to a consistent generic name for sequential
composition eventually (per-type members are not generic — that is the
gap typeclasses close), and `monad.hpp` proves the mechanism carries it;
Monad is unproposed only because nothing currently proposed calls
`bind`. "For now." Reframed in: D3200R0 "Why not Monad", monad.hpp
header note, Paper D companion text, skill (both copies), transpose
worklist item 1, essay SPEC exhibit E6, trees addendum, provenance.

Also this session (transpose repo, branch remove-pure-apply-forms in
`~/src/transpose/apply-removal`): pure/apply removal executed per entry
3 — apply/ap/terminating_partial/applicative_eval deleted, Monad's
synthesized apply removed, dual-core instances (zip_list, array,
simd_lanes) reduced to the invoke core, laws restated invoke-form,
absence of apply forms pinned per instance, fold.hpp/monad.hpp evidence
labels, Traversable no-Foldable-superclass pin, D3200R0 gains "Why not
apply"/"Why not Monad" + non-goals, provenance entry. Build/test pending
merge.

## 2026-07-13 (entry 3) — pure/apply forms removed from the design

Ruling (Steve): the classic `apply` formulation (callable-in-context,
curried application) is a Haskell artifact and a misunderstanding in
C++; all such forms (`apply`, `ap`, currying helpers, monad-synthesized
`apply`) are removed from public surfaces. Applicative primitives are
`pure` + n-ary `invoke`; laws restated in invoke form. Epicenter:
transpose `apply.hpp` + instance headers + `monad.hpp`; mirrors in
fingertree `typeclass/applicative.hpp` and trees/main pedagogy Part 6;
fixpoint sweeps opportunistically. tree_algorithms unaffected
(functor-only surface). Work scheduled in transpose worklist item 5;
repo SHAs unchanged from entry 2.

## 2026-07-13 (entry 2) — [S-1..4] resolved; direct verbs implemented

Decisions (Steve): [S-1] direct verbs are `fold_with`/`unfold_with`;
[S-2] `fold_map` primary spelling takes combine + identity as plain
parameters (no Monoid trait in the primary API); [S-3] fringe tree goes
in `include/`; [S-4] everything publishes on sdowney.org — repo blog
dirs are where code and text come together, so drafting location is
logistics, not a venue decision.

Work: `fold_with`/`unfold_with` implemented in tree_algorithms
`recursion_schemes.hpp` with equivalence-law static_asserts, nonce-tree
tests (move-only `unique_ptr` carrier), adapter-vs-direct agreement law,
and `examples/nonce_tree_direct.cpp`. Also fixed a pre-existing
fetch-content configure defect (Catch2 extras/module-path recovery in
`tests/beman/tree_algorithms/CMakeLists.txt`). All on branch
worktree-bright-ray-n44x, uncommitted.

## 2026-07-13 — fold family to Paper D; Monad acknowledged out-of-band

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
