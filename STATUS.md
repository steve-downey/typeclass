# STATUS — cross-repo work board

Updated: 2026-09-10. One line per open item; details live in the owning
repo's document (linked). Order within each repo is execution order.

## Decisions needed from Steve (each links to its entry in `DECISIONS.md`)

- [paper-d-punch-list](DECISIONS.md#paper-d-punch-list) — existing punch-list items (repo owner swap, clang-18 floor,
  modules ship/waive) — `tree_algorithms/docs/notes/punch-list.md`.
  Partially resolved 2026-07-17 (Steve): the paper number is **D4322** —
  draft lives at `papers/algorithms-for-trees.md`, title
  "Algorithms for Trees"; the DnnnnR0 placeholder file is removed and
  cross-references updated. The other three punch-list decisions remain
  open.
- [sequence-quotient-surface](DECISIONS.md#sequence-quotient-surface) — sequence-quotient surface: status of fringe `view_l`/`view_r` +
  the five-operation quotient interface (D evidence vs D proposed vs
  Paper C's generic face), "quotient" as paper/essay vocabulary,
  FringeTree `operator==`, and the remaining placeholder names — all
  detailed in
  `tree_algorithms/docs/notes/representations-and-sequence-interfaces.md` §4.
  Partially resolved 2026-07-17 (Steve): `fold_left` is Foldable's
  derived operation, one definition from `fold_map`; trees reach it by
  being Foldable, never via a parallel view-based definition.

Resolved 2026-07-13 (see SYNC [direct-verbs-implemented](SYNC.md#direct-verbs-implemented)):

- [direct-verb-names](DECISIONS.md#direct-verb-names) — **`fold_with` / `unfold_with`** — fold/unfold with an explicitly
  supplied project/embed. Note: coheres with transpose's `*_with`
  convention read as "the ingredient you'd otherwise look up is passed
  inline" (there: a typeclass instance; here: the projection).
- [fold-map-monoid-passing](DECISIONS.md#fold-map-monoid-passing) — **combine + identity** as plain parameters; no Monoid trait in
  the primary API.
- [fringe-tree-location](DECISIONS.md#fringe-tree-location) — **`include/`** for the fringe tree.
- [publication-venue](DECISIONS.md#publication-venue) — **sdowney.org** is where everything publishes; the repo blog
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
   work is external-input only: real paper number ([paper-d-punch-list](DECISIONS.md#paper-d-punch-list)), essay URL
   for the companion citation, and the MEASURE re-measurement flags
   carried on the punch list.
5. ~~Representation round~~ **DONE 2026-07-17** (commit e57499b, merged
   to main) per Steve's 2026-07-16 direction: Fix-native BST example
   (`examples/search_tree_on_fix.cpp` — verbs where they fit, plain
   wrap/unwrap recursion where they don't), rose tree
   (`include/rose_tree.hpp` + tests; Fix-native, no Box, pre-order
   contract; pins that in-order is binary-only, not generic), fringe
   `view_l`/`view_r` mirroring Paper C's View vocabulary +
   `examples/sequence_algorithms.cpp` (quotient-interface derivations),
   design note `docs/notes/representations-and-sequence-interfaces.md`,
   anchor inventory extended. Opens [sequence-quotient-surface](DECISIONS.md#sequence-quotient-surface). Paper delta drafted in the
   2026-07-17 documentation pass (next item).
6. ~~Relationships essay~~ **DRAFTED 2026-09-07** — see the essay
   section below. Exhibit anchors created in tree_algorithms (da3d59f)
   and fingertree (bb48221); SYNC [essay-exhibit-pins](SYNC.md#essay-exhibit-pins).
7. ~~Push main to GitHub~~ **DONE 2026-07-17**: pushed through 8d02a60
   (representation round + docs pass). This meta-repo also now has a
   GitHub home: `steve-downey/tree-coordination` (private). The
   "superseded Catch2 stash" note turned out stale — the stash list is
   empty; nothing to drop.

## transpose (Paper A) — worklist: `docs/coordination-worklist-2026-07-13.md`

1. ~~"Why not Monad"~~ **DONE 2026-07-14** (commit b21e8da): deferred-
   not-rejected framing per SYNC [monad-deferred-not-rejected](SYNC.md#monad-deferred-not-rejected), with P0798/P2505 citations.
2. ~~Evidence labels~~ **DONE** (same commit): fold.hpp/monad.hpp
   header notes, provenance entry, non-goals bullets.
3. ~~Traversable superclass pin~~ **DONE** (same commit): documented
   deliberate constraint in traverse.hpp.
4. Cite essay when published (blocked on publication; the pin tag
   `blog/trees-and-transposition-for-29` at bb32a37 already exists on
   GitHub).
5. ~~Pure/apply ruling~~ **SETTLED 2026-07-14** (commits b21e8da then
   b77b4cd, after two refinements — SYNC [ap-basis-invoke-interface](SYNC.md#ap-basis-invoke-interface)): `ap` is a supported
   *basis* (`pure`+`invoke` or `pure`+`ap`, base derives the other) and
   a secondary operation; `invoke` is the sole user-facing interface;
   the spelling `apply` is gone. Binary tree example restored as the
   ap-basis exemplar; laws invoke-form with basis-coherence checks;
   simd invoke-only pin is the interface argument. 54/54 tests.
6. Standing: BinaryTree example cross-check with tree_algorithms —
   NOTE: semantics now stated n-ary; tree_algorithms' copy unaffected
   (it has no applicative instance) but any future one follows the
   n-ary formulation.

## fingertree (Paper C) — worklist: `docs/coordination-worklist-2026-07-13.md`

1. Track direct-verb naming (unblocked by [direct-verb-names](DECISIONS.md#direct-verb-names)); update pencils.
2. Companion-section hygiene when the C draft is written (cite essay;
   instances are implementation not wording; O(n) traverse honesty).
3. Standing: monoid vocabulary gate (`identity`/`combine`).
4. ~~Essay exhibit anchor~~ **DONE 2026-09-07** (commit bb48221 on
   main): anchor 78d295c0 in `examples/finger_tree.cpp`; tag
   `blog/trees-and-transposition-for-29` pushed to GitHub.

## trees/main (origin) — addendum: `docs/notes/standardization-plan-addendum-2026-07-13.md`

1. Fold addendum into `standardization-inverted-triangle-plan.md` on its
   next revision.
2. Extract fringe tree to tree_algorithms when Paper D item 2 starts
   (one-way; record provenance at destination).

## fixpoint — no open items

Evidence repo complete (S00–S16, 214 tests). Referenced by Paper D's
implementation-experience section; compatibility checklist consumed by
tree_algorithms extraction-plan §7.

## Essay — `essay/trees-and-transposition-for-29.org`

1. Publish via wctm ([publication-venue](DECISIONS.md#publication-venue)). In progress at `~/src/wctm` 2026-09-10:
   repo-table entries for tree_algorithms and fingertree added to
   `plugins/orgmode/conf.el`, post copied to
   `posts/trees-and-transposition.org`, all uncommitted; the site build
   is expected to turn up infrastructure work. Pins resolve from GitHub
   (SYNC [essay-exhibit-pins](SYNC.md#essay-exhibit-pins)).
2. Reconcile `essay/SPEC.md` (2026-07-14 revision) with what was written: the draft is
   prose-led with three exhibits, a prose map, and P4307 as a fourth
   paper. Decide whether SPEC is superseded by the draft or the
   six-exhibit piece is still a separate post.
3. Remaining revision items, tracked in the draft's own revision board
   (non-exporting comment blocks): call to action; "What I want" →
   "I can have"; the five missing points in "Why in the Standard"
   (optional anchor, authoring-support pattern, safety clause, regex
   objection, gradient); shrink the how section and stratify the
   debug-cost concession; decide on a standalone "why in the Standard"
   post. Verify list there: paper numbers against the mailing, P2727
   status, the two private backtick links, the godbolt parity check.
4. When published: transpose item 4 and fingertree item 2 (cite the
   essay); update the coherence skill's essay pointer.

## Blog collection (monadic-algorithms re-cast) — plan: `blog/PLAN.md`

1. Reviews of both existing series delivered 2026-07-21:
   `blog/CRITIQUE-fixpoint.md`, `blog/CRITIQUE-transpose.md`. The five
   blocking findings are planned as an execution pass in
   `blog/MUSTFIX.md` (ordered wg21-author-lists → stale-apply-spelling →
   unanchored-quoted-snippets → sender-integration-claim → header-comment-leak; three
   small decisions listed at its end unblock the two that need them).
   Fixes land in the source repos; a SYNC entry pins SHAs when done.
2. Spine post drafted: `blog/where-are-the-monads.org` — the
   Monad-scarcity thesis, the advance answer to "where's Monad?" for the
   campaign; cites Deane's reflection-monad post (elbeno.com,
   2026-03-04). Awaiting Steve's read.
3. Instances series (new writing): reflection-monad post first — needs
   gcc-16 consteval experiments; placement open (see PLAN.md questions).
4. Re-cast edits to both series indexes and framing posts, per the
   critiques and PLAN.md dispositions.

## Done

- 2026-09-07: essay drafted through the edit letter's structural moves
  (lede, map, merged paper sections, how/why-in-the-Standard order,
  close); three exhibits transcluded and pinned by tag in transpose,
  tree_algorithms, fingertree; `.md`/`.html` exported; PR #1 merged
  (dfcf3d9). SYNC [essay-exhibit-pins](SYNC.md#essay-exhibit-pins).
- 2026-07-17: tree_algorithms representation round committed (e57499b)
  and pushed with the docs pass (8d02a60).
- 2026-07-13: extraction plan, coherence skill, per-repo worklists,
  trees addendum, this meta-repo scaffolded; SYNC [fold-family-to-paper-d](SYNC.md#fold-family-to-paper-d).
