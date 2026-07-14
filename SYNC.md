# SYNC ledger

Pins each repo's state at each coordination decision. Append-only;
newest entry first.

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
