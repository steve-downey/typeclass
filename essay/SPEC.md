# Relationships essay — specification (revision of 2026-07-14)

Working title: "Three papers and two deliberate holes"
Form: **exhibit-driven** — 6 code exhibits carry the piece; prose is
captions and connective tissue.
Budget: prose ≤ 1,200 words; code ≤ 90 lines total, no exhibit over 15.
One diagram (the map).
Venue: sdowney.org (per [publication-venue](DECISIONS.md#publication-venue)); cited by all three papers.

## The job, and the tension it must resolve

"Code talks" and "when you're explaining, you're losing" — both at
once. So: the essay *demonstrates* the coordinated set; it does not
explain it. Each claim is carried by a small, real code exhibit; the
caption states the claim in a sentence or two; the deep material is one
link away. A reader skimming only the code blocks should still get the
pitch. A reader who wants the mechanism gets a link, not a lecture.

The essay must leave a LEWG(I) reviewer able to answer:

1. Why three papers and not one?
2. What does each own; what survives if the others die?
3. Where is Monad? Where is `apply`?
4. Where's the deep material?

— but those answers arrive as captions on exhibits, not as sections of
argument.

## The truth mechanism (non-negotiable)

Every exhibit is **transcluded from a UUID-anchored region of compiled,
tested source** — the same org-transclusion pipeline the paper and
article already use (trees/main export setup; tree_algorithms commit
ea51746). No exhibit is retyped, simplified by hand, or "illustrative."
If an exhibit needs to be smaller than the anchored region, fix the
source region, not the quotation. This is what lets the piece sell hard
while staying checkable: every block in it runs in CI somewhere.

## The exhibits

Each: ≤ 15 lines, self-contained, individually titled, and named by a slug
that is also how the source repos' anchor inventories refer to it — because each
must also work as a **hand-sell prop**: one exhibit on a phone screen is
a hallway conversation. Source anchors to be confirmed/created at
draft time.

**exhibit-one-verb-three-contexts — One verb, three domains.** `transpose` flipping
`vector<optional<T>>`, `vector<sender<T>>`, and lanewise values through
the same front door. (transpose repo, three-domain test.)
Caption claim: this is the context axis, and it is one facility, not
three. → Paper A.

**exhibit-fold-vs-visit — The recursion is the algorithm's job.** The before/after from
Paper D: hand-rolled `std::visit` recursion vs `fold_fix` with a
non-recursive algebra. (Already anchored for the paper's cmptable.)
Caption claim: this is the structure axis. → Paper D.

**exhibit-yesterdays-tree — Yesterday's tree, no conversion.** The nonce tree's three
ingredients (layer struct, fmap, projection) then `fold_with` over
`unique_ptr` nodes. (examples/nonce_tree_direct.cpp, anchored.)
Caption claim: the verbs meet your types where they are; no blessed
tree, no Fix in sight. → Paper D.

**exhibit-fold-map-queries — One hook, the whole query family.** length / sum / any_of as
one-line `fold_map`s. (fold_map.test.cpp, DerivedQueries.)
Caption claim: elementwise folding is derived from the structural verbs
— small surface, wide reach. → Paper D.

**exhibit-measured-split — Split a sequence by what it means.** Measured split/search on the
persistent sequence in a few lines. (fingertree repo.)
Caption claim: where structure and context pay off in one concrete,
production-shaped container. → Paper C.

**exhibit-holes — The holes, shown not argued.** Two fragments side by side:
`monad.hpp`'s `bind`/`join` signatures from the reference tree —
evidence the mechanism carries Monad without strain, captioned
"deferred, not rejected: a consistent generic name is where typeclasses
lead, and nothing proposed needs it yet"; and the `invoke(f, a, b, c)`
call that works for `std::simd::vec` where `vec<callable>` is not even
a type — `apply` was never the primitive.
Caption claim: the omissions are decisions with citable rationale, and
one of them is an open slot.
→ P3200's "Why not Monad" / "Why not `apply`" sections.

## Structure

1. **Cold open: exhibit-one-verb-three-contexts.** Code before any thesis — desire first. (~100 words
   around it.)
2. **exhibit-fold-vs-visit, exhibit-yesterdays-tree** — the structure axis, and "works on your types." (~250
   words total.)
3. **The map** — the diagram plus a six-row table: paper / owns /
   explicitly-not / survives-alone. This is the only expository passage,
   placed *after* the reader wants the goods. (~250 words.)
4. **exhibit-fold-map-queries, exhibit-measured-split** — depth of the surface; the concrete payoff. (~200 words.)
5. **exhibit-holes.** (~150 words.)
6. **Going deeper** — link farm: two blog series indexes, the CppNow26
   talk, three papers, repos. Labels only. (~100 words.)

## The anti-book discipline (unchanged, sharpened)

- **No new arguments** — captions state conclusions; the argument lives
  in a paper section or blog post that exists at publication time. This
  makes P3200's two "why not" sections a publication prerequisite
  (transpose worklist items 1 and 5).
- **No teaching** — mechanism, lookup modes, F-algebras, laws: links to
  the transpose series (×5) and fixpoint series (×14).
- **No motivation prose** — each exhibit IS the motivation; each paper
  argues its own case.
- **One post, not a series.** Long-form growth goes into a series and
  becomes a link.

## Acceptance criteria

- The skim test: reading only the exhibit code + titles conveys the
  pitch.
- The hallway test: each exhibit stands alone as a hand-sell prop.
- The truth test: every exhibit transcluded from anchored, CI-covered
  source; zero hand-typed code.
- The four questions answerable from captions alone.
- Budgets held: ≤ 1,200 words prose, ≤ 90 lines code. A section that
  wants more prose donates it to a linked document.
- Each paper's companion section shrinks to two sentences + the link.
