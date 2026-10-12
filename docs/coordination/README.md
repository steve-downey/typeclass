# Coordination

Coordinating the intertwined tree-standardization work:

| Where | Role | Paper |
|---|---|---|
| `src/beman/transpose/` (this repo) | beman.transpose — mechanism + Applicative/Traversable | Paper A / P3200 ([`papers/D3200R0.md`](../../papers/D3200R0.md)) |
| `src/beman/fingertree/` (this repo) | beman.fingertree — persistent measured sequence | Paper C (unnumbered) |
| `src/beman/tree_algorithms/` (this repo) | beman.tree_algorithms — the fold family + direct verbs | Paper D (D4322, [`papers/algorithms-for-trees.md`](../../papers/algorithms-for-trees.md)) |
| `~/src/fixpoint/main` | smd::fixpoint — recursion-scheme catalog, evidence only | never proposed |
| `~/src/trees/main` | origin/pedagogy (CppNow26); one-way extraction source | — |

Monad is deliberately unproposed across the set (the "missing Paper
B"); the relationships essay (to be drafted here, published on
sdowney.org / the transpose blog) is the citable acknowledgment.

## What lives here

- `STATUS.md` — the cross-repo work board: every open coordination item,
  its owning repo, and its blocking decision. This is the scheduler.
- `SYNC.md` — ledger pinning each repo's SHA at each coordination
  decision, so "the coordinated state as of date X" is reconstructible.
- [`.claude/skills/tree-proposals-coherence/SKILL.md`](../../.claude/skills/tree-proposals-coherence/SKILL.md)
  at the repository root — the coherence skill (invariants, procedures,
  hazards). There is one copy now; the per-repo mirrors went with their
  repositories.
- `essay/` — drafts of the relationships essay.
- `CLAUDE.md` — orientation for agents working in this checkout.

## History

This was the separate `tree-coordination` repository: deliberately thin,
holding no copies of the code it coordinated, and pointing at sibling
checkouts under `~/src`. On 2026-10-12 it was merged, with transpose,
tree_algorithms and fingertree, into beman.typeclass, which made the
sibling-checkout arrangement and its subtree upgrade path moot; see the
[2026-10-12 SYNC entry](SYNC.md#2026-10-12--the-three-libraries-merge-into-bemantypeclass).
