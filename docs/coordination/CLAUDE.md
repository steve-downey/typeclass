# CLAUDE.md — docs/coordination

This directory coordinates the three libraries in this repository and
two outside checkouts (see README.md for the map). It holds coordination
artifacts only — no code. Until 2026-10-12 it was the separate
tree-coordination repository.

Rules for working here:

- Read `STATUS.md` first; it is the work board. Keep it current: when an
  item completes in a source repo, move it to Done here.
- Every coordination decision gets a dated, append-only entry in
  `SYNC.md` pinning each repo's SHA and pending-file state.
- The `tree-proposals-coherence` skill lives at
  `.claude/skills/tree-proposals-coherence/SKILL.md` in the repository
  root; there is one copy. Its invariants (division of labor, naming,
  repo-surface ≠ paper-surface, DEV-01/03/04 test discipline) bind work
  on all three libraries and in fixpoint and trees.
- Cross-library greps: transpose, tree_algorithms and fingertree are in
  this repository (`src/beman/<lib>/`, `docs/<lib>/`, `papers/`).
  fixpoint and trees are still sibling checkouts under `~/src`.
- Each library keeps its own conventions (`docs/transpose/decisions.md`
  and `CODING_RULES.md`, `docs/tree_algorithms/DECISIONS.md`); they
  govern changes to that library.
