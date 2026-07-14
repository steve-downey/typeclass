# SYNC ledger

Pins each repo's state at each coordination decision. Append-only;
newest entry first.

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
