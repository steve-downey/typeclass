# Critique — `trees-and-transposition-for-29`

Review of the draft blog post at
`essay/trees-and-transposition-for-29.org`, in its role as the piece that
must persuade a reader to take the four-paper campaign **as a set** and
be interested in and supportive of it.

Companion to the fact check (2026-07-19): citations were added to the
`.org` source and two factual errors fixed (McBride → McBride & Paterson;
"normalization never changes meaning" → *canonical* normalization only).
The `.html` export is stale and needs re-exporting from org.

The voice is right and the honesty is disarming. The problems below are
structural — this is the piece whose job is *"read these four papers as a
set, and root for them,"* and as drafted it does not yet do that job.

---

## 1. The lede is buried

The single best hook in the piece — `vector<sender<int>>` when you need
`sender<vector<int>>` — sits two-thirds of the way down, after two
sections about what the work *isn't* and a section of theory. A reader
who bounces after 200 words leaves knowing only that a Text type was
abandoned.

The Unicode retraction is genuinely interesting and worth keeping — it
preempts "where's Text?" from people who followed the SG16 work — but it
belongs *after* the pitch, not as section one.

**Direction (Steve, next round):** the lede should be a short list of the
concrete problems these libraries solve — "here are things I actually
need to do, and what gives me each." Problems first, machinery later.

- Transpose: the `vector<sender<T>>` → `sender<vector<T>>` flip; AoS→SoA
  for lanewise SIMD. One front door, three domains.
- Trees: run an algorithm over a recursive structure without flattening
  it and without hand-rolling the recursion each time; and do it over the
  tree you *already have*, no conversion.
- Fingertree is the hardest to pitch, because it solves so many problems
  that it is often the *second*-best answer to any one of them — after
  writing the bespoke structure for that one case. So do not pitch it on
  any single operation. **Lean on persistence**, and specifically on the
  zero cost of handing a fingertree off to background processing while
  still working on it: no copy, no lock, no coordination — the version
  you handed off is frozen and the version you keep mutating is a cheap
  share away. That is the property a bespoke container makes you pay for
  every time.

## 2. "The Papers" and "What?" are the same section, twice — wrong one first

The Papers section is pasted paper abstracts. "Shape-preserving traversal
for contextual computations" is committee register, and it lands exactly
where a blog reader's interest peaks. The What? entries are the better
sell in every case.

**Fix:** merge them. One subsection per paper: informal pitch first, one
sentence of formal scope, link. The formal abstract can live in the
paper, one click away.

Also: the P4322 entry is the only paper that never says what it
*proposes* — it is one paragraph of problem statement. The draft's own
abstract has the proposal sentence (the `fold_fix`/`unfold_fix`/`refold`
+ `fold_with`/`unfold_with` + `fold_map` family). Paste it.

## 3. The "campaign as a whole" is asserted, never shown

The abstract promises the papers "lean on each other, and extend or reuse
some machinery," and the essay never says *which* leans on which. This is
**the** thing a reader must take away to support the set rather than pick
at the pieces.

One short map paragraph does it:

- **P3200** owns the customization mechanism and proposes only what its
  three-domain proof forces — Applicative and Traversable.
- **P4322** owns the recursive verbs and stays fully valid if P3200
  stalls: the primary spellings take `fmap`/the monoid explicitly;
  lookup is a separate convenience layer that is explicitly a P3200
  consumer.
- **P4325** is the container where structure and measure pay off
  together — it consumes the mechanism, never respecifies it.
- Each survives alone.

`essay/SPEC.md` already names the four questions a LEWG(I) reviewer
should be able to answer from this material — why not one paper, what
each owns, what survives if the others die, where Monad and `apply` are.
The current draft answers roughly half of one of them.

## 4. Monad silence violates the campaign's own invariant

The coherence rule is that public campaign text acknowledges the open
"Paper B" slot: Monad is **deferred, not rejected**, the typeclass design
leads to a consistent generic name for sequential composition eventually,
and `monad.hpp` exists as evidence that the mechanism already carries it.
The essay currently goes silent on it.

Two sentences suffice, and the "deliberate hole" framing is a persuasion
*asset* — it shows the set was scoped, not accreted. (P3200's own "Why
not Monad" section is the link target.)

## 5. Almost no code, for an audience that code persuades

``a `f` b`` is the only code in the piece — and not coincidentally the
backtick section is the most immediately graspable. `essay/SPEC.md`'s
thesis is "code talks; when you're explaining, you're losing." That
applies to this post too.

**Direction (Steve):** code blocks must be added, transcluded from the
actual example sources in the referenced projects — never hand-typed —
the same UUID-anchored transclusion pipeline the papers and SPEC.md's
exhibit essay already use. Next round adds placeholder blocks; the round
after wires the anchors.

Highest-value exhibits to target:

- **Transpose, one verb three domains** — `transpose` over
  `vector<optional<T>>`, `vector<sender<T>>`, and lanewise values through
  the same call. (transpose repo, three-domain test.)
- **`fold_with` over a tree you already own** — someone's `unique_ptr`
  binary tree, three ingredients (layer struct, fmap, projection), no
  `Fix` in sight, no conversion. (`tree_algorithms/examples/…`, anchored.)
- **Fingertree persistence** — the hand-off: split/share, background the
  frozen version, keep mutating your own. This is the exhibit that sells
  the container on the property no single-operation demo can.

## 6. Two "I want" lines invite drive-by dismissal

"As correct and as sound as OCaml" and "as, or more, performant than
NumPy or Rust" are unfalsifiable as written and will be quoted back on
r/cpp. Either anchor them (the fixpoint repo's 214 tests and the
compile-time-scheme/-forth evidence exist for exactly this) or trim to
the wants that survive a link.

**Direction (Steve, under consideration):** move the "I want" litany to
the **end** and reframe as **"I can have"** — the closing argument for
anyone who reads that far. This is a good move: it converts an
aspirational opening (easy to dismiss) into an earned conclusion (the
post has by then shown the machinery that backs each claim). It also
gives the piece the call-to-action shape it currently lacks. Keep the
list short and make each "I can have" cash a claim the body already
demonstrated or linked.

## 7. No call to action

"Papers are forthcoming. Code exists." is a good flat landing
rhythmically, but the sympathetic reader is left with nothing to do. Name
the verbs: read the drafts (link them), try the repos, tell me where the
names are wrong, come to the LEWG/SG9 sessions. The reframed "I can have"
close (§6) is the natural host for this.

## 8. The title promises C++29; the body never mentions a timeline

One sentence on why '29 is plausible — drafts in the 2026-08-14 mailing,
Búzios in November — turns the title from a wish into a plan.

## 9. The backtick paper reads as a side quest

Title says trees, transposition, a container; P4307 arrives unannounced,
targets EWG not LEWG, and dilutes the "coherent set" story. It has a real
connection — infix spelling is what makes `combine`-heavy monoid code
readable — so either say that in one sentence or explicitly label it the
bonus track. Unexplained, it costs coherence.

## 10. Decide this post's relationship to the planned exhibit essay

`essay/SPEC.md` defines an exhibit-driven piece that all three papers
cite as "the single map." This draft overlaps that mandate in prose form.
If this ships first it becomes the de facto map — fine, but then its
claims are the ones the papers must stay consistent with. If it is the
teaser and the exhibit essay is the map, say so with a forward link and
keep this one short.

---

## One gift from the fact check

Ross Paterson co-authored *Bananas, Lenses, Envelopes and Barbed Wire*
(1991, the fold/unfold/fuse trio), *Finger Trees* (2006), **and**
*Applicative Programming with Effects* (2008) — one person's fingerprints
on all three pillars of the campaign. Dropped flat and unexplained, it is
very much an in-voice aside.

---

## Priority order for the next revision

1. New lede: the problem list (§1), fingertree pitched on persistence /
   zero-cost hand-off.
2. Merge Papers + What? into one pitch-first section per paper; add the
   missing P4322 proposal sentence (§2).
3. Add the campaign map paragraph — who leans on whom, what survives
   alone (§3).
4. Add the Monad "deliberate hole" acknowledgment (§4).
5. Placeholder code blocks now; transcluded anchors next round (§5).
6. Move "I want" → "I can have" close, with the call to action (§6, §7).
7. Move the Unicode retraction below the pitch; one C++29 timeline
   sentence (§1, §8).
8. Decide backtick framing (§9) and relationship to the exhibit essay
   (§10).
