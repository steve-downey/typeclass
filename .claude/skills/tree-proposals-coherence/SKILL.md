---
name: tree-proposals-coherence
description: Keep the coordinated tree-proposal ecosystem coherent when working in any one of its repos — tree_algorithms (Paper D), transpose (P3200/Paper A), fingertree (Paper C), the fixpoint evidence repo, or the trees/main origin repo. Use before changing shared vocabulary (verb names, typeclass/monoid spellings, lookup pattern), adding API surface, editing paper text about companion proposals, or extracting code between these repos.
---

# Tree-proposals coherence

Three WG21 proposals plus two research repos form one deeply intertwined
body of work. Each repo is independently reviewable, but names, division
of labor, and test discipline are shared. This skill is the map and the
invariants; check work against it before and after changes in ANY of
these repos.

## The map

| Repo | Role | Authoritative documents |
|------|------|------------------------|
| `~/src/tree_algorithms/main` (this repo) | **Paper D** — recursive tree algorithms: `fold_fix`/`unfold_fix`/`refold`, `Fix`/`Box` as exposition, adapters across representations | `docs/DECISIONS.md` (frozen), `papers/algorithms-for-trees.md`, `docs/notes/extraction-plan.md`, `docs/notes/punch-list.md`, `docs/notes/anchor-inventory.md` |
| `~/src/transpose/main` | **Paper A / P3200 (D3200R0)** — traversal, transposition, bundled customization, and the grading of fallible contexts. Owns the mechanism: typeclass-object lookup, Functor/Foldable/Applicative/Traversable/Monad bases, `traverse`/`transpose`/`invoke`, `Monoid`, and the grade algebra (`error_set`, `grade_join`/`grade_bottom`/`grade_subsumes`/`grade_subsume`) | `papers/D3200R0.md` (moved out of the vendored `papers/wg21/` tree 2026-09-06), `docs/decisions.md` (question-slug log), `docs/transpose-grading-plan.md`, `docs/provenance.md`, `docs/typeclass-object-pattern.md` |
| `~/src/fingertree/fingertree` | **Paper C / D4325R0** — persistent measured sequence. Owns the container, measures, allocator/PMR story | `docs/allocator-design.md`, `docs/compile-time-design.md`, `docs/measure-based-splitting.md` |
| `~/src/fixpoint/main` | **Evidence only, never proposed** — full recursion-scheme catalog (para/apo/zygo/histo/futu/…, distributive laws, gcata/gana/ghylo) proving the three verbs are the stable primitive layer | `docs/recursion-schemes-design.md` (D1–D11), `ops/DEVIATIONS.md` (DEV-01..04), `ops/PLAN.md` |
| `~/src/trees/main` | **Origin/pedagogy** — CppNow26 talk, source of all extractions. Extraction is ONE-WAY; extracted repos diverge deliberately and do not backport | `docs/notes/standardization-inverted-triangle-plan.md` (three-paper strategy), `foldable-applicative-traversable.org`, `CLAUDE.md` |

## Invariants — never violate silently

**Division of labor.**
- P3200 owns the customization mechanism and proposes only what its
  three-domain proof forces: Applicative and Traversable — plus, since
  2026-09, the grade algebra that makes fallible contexts compose
  (`[transpose.grade]`, `[transpose.errset]`, `[transpose.expected]`).
  Papers C and D *consume* the mechanism and never respecify it. The
  mechanism debate happens only in P3200, and so does the grading
  debate.
- **Grading is Paper A's axis; whether it reaches the fold family is
  open.** The rule that keeps the two apart: fold grades are
  shape-independent *iff* the grade algebra is a bounded
  join-semilattice, which is what lets recursive data types stay
  ungraded while their effectful clauses carry the grade. `Fix` stays
  ungraded. Anything that would make a fold's grade depend on runtime
  shape (fuel, cost — non-idempotent grades) is a perpendicular axis and
  belongs in compile-time-scheme, not here. Paper A's follow-on guidance
  for tree_algorithms is in `docs/transpose-grading-plan.md`; the scope
  question is open as `graded-fold-scope` in the coordination repo's
  `DECISIONS.md`.
- **Paper D proposes the fold family, both structural and elementwise**:
  `fold_fix`/`unfold_fix`/`refold` plus `fold_map` (decided 2026-07-13).
  Rationale: `fold_map`'s motivation is structures that aren't ranges —
  tree-shaped, not transpose-shaped; `ranges::fold_left` already covers
  flat sequences.
- **Monad is deferred, not rejected** — the open "Paper B" slot
  (framing refined 2026-07-14). The typeclass design leads to a
  consistent generic name for sequential composition eventually
  (per-type members `and_then`/`transform`/`let_value` are not
  generic), and `monad.hpp` proves the mechanism carries it. It is
  unproposed only because nothing currently proposed calls `bind`.
  Never frame the absence as rejection of the abstraction, and never
  let a paper go silent on it: D3200R0 carries "Why not Monad," and
  the essay acknowledges the open slot.
- **Repo surface ≠ paper surface.** A beman repo may ship more than its
  paper proposes (beman.transpose ships `fold.hpp` and `monad.hpp`;
  D3200R0 proposes neither), but every unproposed component must be
  explicitly labeled evidence — the role the fixpoint catalog plays for
  Paper D. Shipped-but-silent is the failure mode to catch in review.
- Paper D's primary verbs take `fmap` explicitly (no mechanism
  dependence), and `fold_map`'s primary spelling takes the monoid
  explicitly; lookup-based overloads live in a separate convenience
  header that is explicitly a P3200 consumer.
- No normative dependence between Papers C and D in either direction.
- The exotic scheme catalog stays in the fixpoint repo. Nothing from it
  ships in a proposal; it is cited as evidence only.
- **Public text stands alone** (ruling, Steve 2026-07-17): papers and
  blog posts must not cite internal planning identifiers — decision-log
  numbers (D5, D8, DEV-01), question slugs, SYNC entries, or
  note filenames — in running text. Describe the reason or the effect
  instead, or say nothing; a reader must never need a second document
  open to follow the text. Real paper numbers (P3200, D4322R0) and
  repo file paths in implementation-experience sections are fine.
  Internal docs and worklists should cross-reference freely, with
  actual links preferred.

**Naming (frozen).**
- `fold_fix`/`unfold_fix`/`refold`; `cata`/`ana`/`hylo` are banned as
  identifiers everywhere in Paper D's repo, even as deprecated aliases.
- `transpose`, not `sequence`; `invoke` as the user-facing applicative
  verb; "recursive fold"/"recursive build" are prose, never identifiers.
- **`ap` is a basis; `invoke` is the interface** (rulings 2026-07-13/14,
  settled): instances opt in with `pure` + `invoke` OR `pure` + `ap` —
  both are legitimate bases, the Applicative base derives the missing
  one (currying cost confined to the derivation), and `ap` exists as a
  secondary operation wherever the context can hold a callable. The
  user-facing verb — papers, examples, teaching — is always the n-ary
  `invoke`: `std::simd::vec` (no `vec<callable>`) proves an ap-shaped
  *interface* would lock out a motivating domain. Laws are stated in
  invoke form with per-instance basis-coherence checks. The name is
  `ap`, never `apply`; never lead with it.
- Monoid operations are spelled `identity()` and `combine()` — in
  transpose AND fingertree AND anything new. Renaming is a multi-repo
  sweep, not a local edit.

**Design decisions (frozen; change the doc first, then the code).**
- `Fix` is composition (`F<Fix<F>> inner`), not inheritance.
- Hand-rolled constexpr `Box`, not `std::indirect`; paper presents
  `std::indirect` as the plausible future spelling.
- Value semantics; performance tuning, sharing, memoization, and stack
  safety are non-goals.
- Language floors differ deliberately: this repo C++23 (gcc-14+/clang
  floor under review), fixpoint repo C++26/gcc-16. Do not import
  C++26-isms into proposal repos.

**Test discipline (applies to all new code in any of these repos).**
- DEV-01: test algebras must be non-idempotent and order-sensitive;
  monoid tests need a non-commutative monoid (string concat).
- DEV-03: no bare CTAD on wrapped types (`Fix{x}`, `Box{x}` banned —
  name the full type).
- DEV-04: `constexpr` at declaration on every CRTP-derived operation;
  each verb covered by at least one `static_assert`.
- Every new verb/instance gets an equivalence law pinning it to an
  already-tested primitive (e.g. `gcata(dist_cata) ≡ fold_fix`,
  `fold_tree(…, unwrap_fix, t) ≡ fold_fix(…, t)`).

## Procedures

**Before changing shared vocabulary** (verb names, typeclass/monoid/
measure spellings, lookup-object pattern, `element_type` convention):
grep all five repos for the identifier; list affected papers, headers,
blog posts, and org files; either update all in one coordinated pass or
record the divergence explicitly in the consuming repo's notes.

**Before relitigating a design choice** in this repo: read
`docs/DECISIONS.md`. If a change is genuinely needed, update DECISIONS.md
first (with rationale and source citations), then the code.

**When extracting code from `trees/main`**: extraction is one-way. Record
what was taken, from which commit, and every intentional deviation in the
destination repo (transpose uses `docs/provenance.md` — follow that
model). Never backport.

**When editing paper text about a companion paper**: read the companion's
current draft first (paths in the map above); papers drift. Keep the
independence claims accurate — Paper D must remain fully valid if P3200
stalls, and vice versa.

**When adding API surface to this repo**: check the fixpoint
compatibility checklist in `docs/notes/extraction-plan.md` §7 — new
surface must stay upgradeable to the unproposed catalog (plain callables
usable as gcata/gana algebras/coalgebras; layer Functor instance remains
the only hook carriers need).

**When touching the paper synopsis or anchored examples**: the synopsis
is a hand-maintained mirror of the headers — re-diff against
`recursion_schemes.hpp`/`recursion_schemes_lookup.hpp` on any signature
change, and keep `docs/notes/anchor-inventory.md` current.

## Known coordination hazards

- **Two BinaryTree ports exist**: this repo's
  `include/beman/tree_algorithms/binary_tree.hpp` and transpose's
  `examples/binary_tree.hpp`. Both derive from `trees/main`; neither is
  authoritative for the other. Semantics changes (traversal order,
  applicative behavior) must be cross-checked.
- **Foldable spans two repos with one proposal home**: beman.transpose
  ships the CRTP base and `fold_map` machinery in `fold.hpp` (repo
  surface, evidence only); the *proposal* is Paper D's. A's paper must
  not silently claim it; D's paper does. Do not grow a third definition.
  Discharged on A's side 2026-09: D3200R0's non-goals name Foldable,
  attribute the fold family to the companion paper, and label
  `fold.hpp`/`monad.hpp` as evidence rather than wording. Keep checking
  it — the failure mode returns the moment either paper's scope moves.
- **The mechanism has unlanded branches**: transpose carries
  `typeclass-operation-cpos` and `typeclass-instance-grounding`, either
  of which would change the lookup pattern Paper D's convenience headers
  consume (the `traverse-policy-surface` decision was taken specifically
  to keep a CPO `traverse` possible). Nothing downstream should mirror
  the lookup pattern while they are open; check `git branch` in
  transpose, not just its `main`, before copying the shape.
- **A's Traversable must not require Foldable as a superclass** —
  otherwise D proposing Foldable beneath an already-reviewed Traversable
  creates a dependency inversion. Check this whenever concept
  definitions in transpose change.
- **The relationships essay is the single map** (once published on
  sdowney.org or transpose's blog series): all three papers cite it
  rather than re-explaining the set; keep its claims in sync when any
  paper's scope shifts, and add its URL to this skill's map table.
- **Paper numbers in flux**: Paper D is D4322R0 ("Algorithms for Trees",
  assigned 2026-07-17; `DnnnnR0` was the placeholder); Paper C is
  D4325R0 ("A Persistent Measured Sequence for the Standard Library",
  assigned 2026-07-19); P3200 is reserved for Paper A (registrar entry
  dates to 2024 and still carries the placeholder title "Trees for the
  standard library" — the title updates on submission). Verify wg21.link
  references before publication.
- **Paper files are named by name, not number** (ruling, Steve
  2026-07-17): e.g. `papers/algorithms-for-trees.md`, never
  `papers/D4322R0-....md`. The WG21 upload system renames what is
  uploaded; names find things later and extend better (a Wording tex
  sub-doc factors out alongside). Numbers live in the YAML front matter
  and prose only.
- **fingertree pencils in `recursive_fold.hpp`/`recursive_build.hpp`** in
  its extraction memo; that interface design is owned HERE (direct
  verbs, extraction-plan §5) and the working names will change.
- **Traversal order is contractual** per instance (in-order for binary
  trees); an instance changing order is an observable break in every
  consuming repo.
