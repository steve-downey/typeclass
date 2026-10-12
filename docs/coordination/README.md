# tree-coordination

Meta-repository coordinating the intertwined tree-standardization work:

| Repo (sibling checkout) | Role | Paper |
|---|---|---|
| `~/src/transpose/transpose` | beman.transpose — mechanism + Applicative/Traversable | Paper A / P3200 (D3200R0) |
| `~/src/fingertree/main` | beman.fingertree — persistent measured sequence | Paper C (unnumbered) |
| `~/src/tree_algorithms/tree_algorithms.git` | beman.tree_algorithms — the fold family + direct verbs | Paper D (`D4322R0`) |
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
- `.claude/skills/tree-proposals-coherence/SKILL.md` — **canonical copy**
  of the coherence skill (invariants, procedures, hazards). Per-repo
  copies are mirrors; edit here first, then sync.
- `essay/` — drafts of the relationships essay.
- `CLAUDE.md` — orientation for agents working in this checkout.

## Why thin, and the subtree upgrade path

This repo deliberately contains **no subtree copies** of the five repos
yet. The sibling checkouts under `~/src` already give cross-repo grep
and editing; a thin repo has zero merge friction and can be pushed and
tracked independently, which is the immediate need.

If a browsable, versioned snapshot of the coordination-relevant material
becomes worth the cost, the upgrade path is `git subtree` of the
*document* directories only — not whole repos (their vendored trees,
CI, and build dirs don't belong here, and bidirectional subtree
push/pull across five active histories is real merge overhead):

```sh
git subtree add --prefix=mirror/transpose-papers  ~/src/transpose/transpose  main --squash
git subtree add --prefix=mirror/fingertree-docs   ~/src/fingertree/main      main --squash
# pull updates later:
git subtree pull --prefix=mirror/transpose-papers ~/src/transpose/transpose  main --squash
```

(Whole-repo subtrees with `--squash` keep this repo small but make
`subtree push` back to the sources unreliable; treat mirrors as
read-only if added, and keep edits flowing through the source repos.)
