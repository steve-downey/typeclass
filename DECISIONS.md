# Decision log — tree-coordination

One section per question, headed by its slug; open questions live in the same
namespace with `Status: OPEN`. Answering one graduates it in place; the slug and
every link to it survive. Divergences append dated entries to the question's
Log. Entry shape: Question / Status / Decided by / Decision / Why / Log.
Every reference elsewhere in this repo is a link to the anchor here.

## direct-verb-names

**Question:** What are the direct (non-`Fix`) fold and unfold verbs called, the
ones that take the projection or embedding explicitly?
**Status:** DECIDED 2026-07-13
**Decided by:** Steve Downey
**Decision:** `fold_with` / `unfold_with`.
**Why:** Coheres with transpose's `*_with` convention read as "the ingredient
you would otherwise look up is passed inline" (there a typeclass instance,
here the projection).
**Log:**
- 2026-07-13 — decided; implemented in tree_algorithms `recursion_schemes.hpp`
  ([direct-verbs-implemented](SYNC.md#direct-verbs-implemented)).

## fold-map-monoid-passing

**Question:** How does the monoid reach `fold_map` in the primary API: a Monoid
trait, or plain parameters?
**Status:** DECIDED 2026-07-13
**Decided by:** Steve Downey
**Decision:** `combine` + `identity` as plain parameters; no Monoid trait in the
primary API. Lookup-based overloads live in a separate convenience layer.
**Why:** Keeps Paper D's primary verbs free of mechanism dependence, so the
tree paper stands if P3200 stalls.
**Log:**
- 2026-07-13 — decided ([direct-verbs-implemented](SYNC.md#direct-verbs-implemented)).

## fringe-tree-location

**Question:** Where does the fringe tree live in tree_algorithms?
**Status:** DECIDED 2026-07-13
**Decided by:** Steve Downey
**Decision:** `include/`.
**Why:** It is shipped surface, not an example.
**Log:**
- 2026-07-13 — decided; ported 2026-07-14 (commit 061c7fc).

## publication-venue

**Question:** Where do the essay and the blog series publish?
**Status:** DECIDED 2026-07-13
**Decided by:** Steve Downey
**Decision:** sdowney.org, via the wctm site repo. Repo blog directories are
where code and text come together, so drafting location is logistics, not a
venue decision.
**Why:** One venue all four papers can cite.
**Log:**
- 2026-07-13 — decided.
- 2026-09-10 — essay copy and repo-table entries in progress at wctm
  ([essay-exhibit-pins](SYNC.md#essay-exhibit-pins)).

## paper-d-punch-list

**Question:** The tree_algorithms punch-list rulings: paper number, repo owner
swap, clang-18 floor, modules ship or waive
(`tree_algorithms/docs/notes/punch-list.md`).
**Status:** OPEN (paper number decided 2026-07-17; the other three open)
**Decided by:** Steve Downey (paper number)
**Decision:** Paper number is D4322, title "Algorithms for Trees"; the draft is
`papers/algorithms-for-trees.md`. Owner swap, clang-18 floor, and modules
remain open.
**Why:** Paper number was needed to unblock cross-references.
**Log:**
- 2026-07-17 — paper number ruled; placeholder file removed.

## sequence-quotient-surface

**Question:** Status of the fringe `view_l`/`view_r` and the five-operation
quotient interface: Paper D evidence, Paper D proposed, or Paper C's generic
face; "quotient" as paper and essay vocabulary; FringeTree `operator==`; the
remaining placeholder names
(`tree_algorithms/docs/notes/representations-and-sequence-interfaces.md` §4).
**Status:** OPEN (partially resolved 2026-07-17)
**Decided by:** Steve Downey (the `fold_left` part)
**Decision so far:** `fold_left` is Foldable's derived operation, one
definition from `fold_map`; trees reach it by being Foldable, never via a
parallel view-based definition.
**Why:** One definition, one derivation path.
**Log:**
- 2026-07-17 — opened by the representation round; `fold_left` part ruled.
