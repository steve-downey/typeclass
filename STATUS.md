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

1. ~~Direct verbs~~ **DONE 2026-07-14** (commit 6cba179, merged to main):
   `fold_with`/`unfold_with`, equivalence-law static_asserts, nonce-tree
   tests + example, adapter-vs-direct agreement law. 61/61 tests green.
2. ~~Fringe tree port~~ **DONE 2026-07-14** (commit 061c7fc, merged to
   main): direct-only adapter (no Fix, no Box — children by value),
   project/embed, Functor instance, layer_fold; embed maintains the
   cached-measure invariant via branch(); 67/67 tests green. Notable
   for the paper: same-sequence-different-shape folds equal — the
   monoid-law payoff test.
3. ~~Fold family~~ **DONE 2026-07-14** (commits e5939d2 + 225af3c,
   merged to main): explicit tier and lookup tier both shipped.
   Four-argument everyday spelling resolves layer_fold_typeclass /
   project_typeclass / functor_typeclass; instances registered for
   BinaryTree, FringeTree, and ExprF (constants-sum 6 vs eval 9
   contrast, compile-time pinned). DECISIONS.md carries the dated
   scope amendment. 68/68 tests green, 17 headers verify.
4. ~~Paper edits~~ **DONE 2026-07-14** (commit ddbab20, merged to main):
   the DnnnnR0 draft now proposes the six-verb family — new §Trees you
   don't own and §Folding and traversing trees, synopsis mirrors all
   anchored regions (re-diff ledger updated), fold_with naming table,
   four-representation implementation experience with the non-monoid
   test-policy anecdote, Monad-absence acknowledgment. Remaining paper
   work is external-input only: real paper number ([S-5]), essay URL
   for the companion citation, and the MEASURE re-measurement flags
   carried on the punch list.
5. Relationships essay — specified in `tree-coordination/essay/SPEC.md`
   (r2, exhibit-driven): six transcluded code exhibits carry the piece,
   prose ≤ 1,200 words as captions, code ≤ 90 lines from UUID-anchored
   CI-covered source only. Skim test, hallway/hand-sell test, truth
   test. Prerequisites: P3200's "Why not Monad"/"why not apply" sections
   (transpose items 1, 5) and exhibit anchors confirmed in transpose and
   fingertree repos (E1, E5 — anchors may need creating there). Publish
   per [S-4].
6. Push main to GitHub (steve-downey/tree_algorithms) when Steve is
   ready; local main is ahead at e5939d2. Note: a superseded local
   Catch2 fix attempt was stashed in `~/src/tree_algorithms/main`
   ("superseded local Catch2 module-path attempt").

## transpose (Paper A) — worklist: `docs/coordination-worklist-2026-07-13.md`

1. ~~"Why not Monad"~~ **DONE 2026-07-14** (commit b21e8da): deferred-
   not-rejected framing per SYNC entry 4, with P0798/P2505 citations.
2. ~~Evidence labels~~ **DONE** (same commit): fold.hpp/monad.hpp
   header notes, provenance entry, non-goals bullets.
3. ~~Traversable superclass pin~~ **DONE** (same commit): documented
   deliberate constraint in traverse.hpp.
4. Cite essay when published (blocked on [S-4]).
5. ~~Pure/apply removal~~ **DONE** (same commit): apply/ap/currying
   helpers/applicative_eval/monad-apply all deleted; dual-core
   instances reduced to the invoke core; binary tree example rewritten
   as shape-aware n-ary invoke (broadcast-and-intersect); laws
   invoke-form; absence pinned per instance. 54/54 tests.
6. Standing: BinaryTree example cross-check with tree_algorithms —
   NOTE: semantics now stated n-ary; tree_algorithms' copy unaffected
   (it has no applicative instance) but any future one follows the
   n-ary formulation.

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
