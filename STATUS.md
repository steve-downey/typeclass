# STATUS — cross-repo work board

Updated: 2026-07-13. One line per open item; details live in the owning
repo's document (linked). Order within each repo is execution order.

## Decisions needed from Steve (unblock markers: [S-n])

- [S-5] Existing punch-list items (paper number request, repo owner
  swap, clang-18 floor, modules ship/waive) —
  `tree_algorithms/docs/notes/punch-list.md`.

Resolved 2026-07-13 (see SYNC.md entry 2):

- [S-1] **`fold_with` / `unfold_with`** — fold/unfold with an explicitly
  supplied project/embed. Note: coheres with transpose's `*_with`
  convention read as "the ingredient you'd otherwise look up is passed
  inline" (there: a typeclass instance; here: the projection).
- [S-2] **combine + identity** as plain parameters; no Monoid trait in
  the primary API.
- [S-3] **`include/`** for the fringe tree.
- [S-4] **sdowney.org** is where everything publishes; the repo blog
  dirs are where code and text come together, so draft wherever
  convenient and export as usual.

## tree_algorithms (Paper D) — plan: `docs/notes/extraction-plan.md`

1. Direct verbs: project/embed wrappers over `refold`, placeholder
   names; equivalence-law tests (`≡ fold_fix` via `unwrap_fix`;
   adapter-vs-direct agreement on BinaryTree); nonce-tree example.
   Needs [S-1] to freeze.
2. Fringe tree port from `trees/main/src/smd/tree/fringe_tree.hpp`;
   direct-verb exercise; cached-measure invariant tests. Needs [S-3].
3. Fold family: `fold_map` explicit-monoid primary + lookup convenience;
   instances (ExprF, BinaryTree, FringeTree); derivation-from-`fold_fix`;
   non-commutative-monoid tests (DEV-01). Needs [S-2].
4. Paper edits (extraction-plan §8): new sections, synopsis + anchors,
   companion-relationships expansion, Monad acknowledgment. After 1–3
   and [S-4].
5. Relationships essay draft (extraction-plan §7a) — draft in
   `tree-coordination/essay/`, publish per [S-4].

## transpose (Paper A) — worklist: `docs/coordination-worklist-2026-07-13.md`

1. "Why not Monad" section in D3200R0 (+ P0798/P2505 references).
2. Label `fold.hpp`/`monad.hpp` as unproposed evidence (headers,
   provenance.md, non-goals).
3. Verify + pin: Traversable has no Foldable superclass requirement.
4. Cite essay when published (blocked on [S-4]).
5. Remove pure/apply forms (`apply`, `ap`, currying helpers,
   monad-synthesized `apply`); primitives become `pure` + n-ary
   `invoke`; laws restated in invoke form; D3200R0 gains "why not
   apply" (ruling 2026-07-13, SYNC entry 3; worklist item 5).
6. Standing: BinaryTree example cross-check with tree_algorithms.

## fingertree (Paper C) — worklist: `docs/coordination-worklist-2026-07-13.md`

1. Track direct-verb naming (blocked on [S-1]); update pencils.
2. Companion-section hygiene when the C draft is written (cite essay;
   instances are implementation not wording; O(n) traverse honesty).
3. Standing: monoid vocabulary gate (`identity`/`combine`).

## trees/main (origin) — addendum: `docs/notes/standardization-plan-addendum-2026-07-13.md`

1. Fold addendum into `standardization-inverted-triangle-plan.md` on its
   next revision.
2. Extract fringe tree to tree_algorithms when Paper D item 2 starts
   (one-way; record provenance at destination).

## fixpoint — no open items

Evidence repo complete (S00–S16, 214 tests). Referenced by Paper D's
implementation-experience section; compatibility checklist consumed by
tree_algorithms extraction-plan §7.

## Done

- 2026-07-13: extraction plan, coherence skill, per-repo worklists,
  trees addendum, this meta-repo scaffolded; SYNC ledger entry 1.
