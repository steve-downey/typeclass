# Edit letter — *Trees, Transposition, and a new Container for C++29*

For the next full draft. Working document: run through it top to bottom,
stet whatever you disagree with. Rationale for everything here is in
`CRITIQUE.md`; section numbers in parentheses point at it. You should not
need that file open to do the work.

---

## Where the draft stands

The voice is right, and the honesty is the draft's best asset — the
Unicode retraction in particular buys credibility that no amount of
advocacy would. The individual pitches are good. The `vector<sender<T>>`
line is the best sentence in the piece.

What the draft is not yet doing is the one job only it can do. Four
separate good pitches sitting in one post is not a campaign. Any of these
papers could be argued in its own paper; this essay exists to make the
case that they are one body of work, and that case is currently asserted
in the abstract and never made anywhere in the body.

**Everything below is downstream of that.** If you fix nothing else, fix
that.

## The diagnosis in one sentence

The reader finishes knowing four things exist and not knowing why they
arrived together, what each one owns, or what survives if the others die.

---

## Proposed running order

The current shape is *what it isn't → theory → paper abstracts → informal
pitches → links*. That front-loads the two weakest positions: an
apology and a lecture. Proposed instead:

1. **The problems.** Three or four, one line each. Cold open — no thesis
   paragraph in front of them.
2. **The map.** Who owns what, who leans on whom, what survives alone.
   The one expository passage, placed after the reader wants the goods.
3. **The papers,** one section each, pitch first, formal scope second,
   link third. One exhibit per paper.
4. **The how** — higher-order functions, internal iteration, typeclasses.
   Shrunk, and moved here because it is now answering "how does that
   work" rather than preloading theory.
5. **Why in the *Standard*.** Compact. The objection handler, placed once
   the reader knows what is being asked for.
6. **The close.** "I can have," the Unicode retraction, the deliberate
   holes, the C++29 timeline sentence, the call to action.

---

## The moves, in priority order

### 1. Write the map (§3, §11d)

The highest-value paragraph in the essay and it does not exist yet. It
needs to answer four things a reviewer will ask, and answer them in
prose, not a table of adjectives:

- **Why four papers and not one.**
- **What each owns.** P3200 owns the customization mechanism and proposes
  only what its three-domain proof forces. P4322 owns the recursive
  verbs. P4325 owns the container, the measures, and the allocator story.
  P4307 is a language change and owns the token.
- **What survives alone.** P4322 stays fully valid if P3200 stalls —
  the primary spellings take the ingredients explicitly and lookup is a
  separate convenience layer. Say this outright; it is the single most
  reassuring fact about the set and it is currently invisible.
- **Where the holes are** — see move 5.

Your plan reworked both ends of the essay and left the middle alone. The
middle is where this job gets done.

### 2. Rebuild the lede as the problem list (§1, §11b)

Three or four problems, one line each, each naming the *pain* — the loop
written again, the copy paid for, the recursion hand-rolled a fourth
time — not the facility that fixes it. A problem list that names
facilities is a feature list, which is the failure mode.

Order by how widely the pain is felt. The `vector<optional>` /
`vector<sender>` flip almost certainly goes first. Check that every paper
has a slot, because a paper missing from the lede is demoted by
implication — which is the argument for leaving the backtick out and
letting it be the bonus track.

**Fingertree is the hard one and your instinct is right.** It loses every
single-operation comparison to a purpose-built structure, so pitch it on
persistence and the hand-off. Two things that entry must do:

- **Concede the cost in the same breath.** The hand-off is O(1) and
  allocation-free; ongoing operations cost more than `vector`. Say it
  first. Being first to name the cost is what makes the claim credible.
- **Answer `shared_ptr<const vector>` explicitly.** It is the closest
  competitor and it also gets an O(1) hand-off. What it does not get is
  continuing to work — freezing it freezes everything, and resuming
  mutation costs a full copy. Fingertree gives the hand-off *and* cheap
  continued mutation of your own version. That is the differentiator, and
  without it "persistent" reads as an academic property.
- **Keep the measure as a tie-in clause.** Persistence is the hook, but
  the annotation is what makes it a fingertree rather than any persistent
  sequence, and what connects it to the monoid vocabulary shared with the
  rest of the campaign. All-persistence detaches the container from the
  set, which is the one thing this essay exists to prevent.

### 3. Merge "The Papers" into "What?" (§2)

They are the same section twice and the committee-register version is
first. Merge to one section per paper: informal pitch, one sentence of
formal scope, link. The abstract belongs in the paper.

P4322's entry is currently one paragraph of problem statement and never
says what it proposes — the only paper with that gap. The draft's own
abstract has the sentence.

### 4. Move the "I want" list to the close as "I can have" (§6, §11a)

Right call. Two constraints.

**It must be shorter than the "I want" list was.** Cut what cannot be
cashed rather than softening it. "As correct and as sound as OCaml"
cannot be cashed — soundness is a property of a type system and C++'s
does not become sound because these papers land; the defensible narrower
claim is about not having to reach for unsafe iteration. "As, or more,
performant than NumPy or Rust" needs a number or a link behind it. The
Maths-degree line cashes completely and is the best line in the list.

**Build it to survive being read cold.** Skimmers will scroll straight to
the close. A list, not a paragraph.

### 5. Fold the deliberate holes into the close (§4, §11a)

Monad is deferred, not rejected: the typeclass design leads to a
consistent generic name for sequential composition eventually, `monad.hpp`
proves the mechanism carries it, and nothing currently proposed calls
`bind`. This supersedes my earlier suggestion of a standalone section —
in the close it stops being defensive and becomes part of an honest
ending. The things you *cannot* have yet are the campaign's shape.

The Unicode retraction lands here too, for the same reason: one thing you
set out to have and decided against. It shows the list was filtered
rather than accumulated.

### 6. Add the exhibits as placeholders (§5)

`a `f` b` is currently the only code, and not coincidentally the backtick
section is the most immediately graspable. Placeholders this pass,
anchors next. Targets:

- **Transpose across three domains** through one front door.
- **`fold_with` over a tree you already own** — layer struct, fmap,
  projection, no `Fix`, no conversion.
- **The fingertree hand-off** — this one carries the persistence pitch
  from move 2, so draft the two together.

Every exhibit transcluded from anchored, CI-covered source. Nothing
hand-typed, nothing "illustrative." If an exhibit needs to be smaller,
fix the source region.

### 7. Add the compact "why in the Standard" passage (§12)

The objection that decides these papers. Compact version here, full
treatment as its own post (see decisions, below). What the compact
version needs:

- **The argument, not the frame.** Vocabulary has value only if shared,
  so a vocabulary mechanism outside the standard is close to worthless.
  Batteries-vs-prelude is the frame; let it emerge.
- **`std::optional` as the anchor.** Nobody argues it should not be
  standard because you could write your own. Everybody did write their
  own, and that was the problem.
- **The twenty-eight-year iterator example.** Concept standardized 1998,
  authoring aid never; `std::iterator` removed in C++26 while
  `iterator_interface` is still in flight.
- **The gradient.** The four papers do not need this equally, and
  conceding that is more persuasive than uniform urgency. P3200 is the
  pure case; P4325 is the weak case and you should say so.

### 8. Loose ends

- Move the Unicode retraction out of position one (move 5 gives it a
  home).
- One sentence on why C++29 is plausible — the title promises a timeline
  the body never mentions.
- Decide the backtick framing (below).
- Add the call to action. "Papers are forthcoming. Code exists." is a
  good landing rhythmically but leaves a sympathetic reader with nothing
  to do.

---

## Decisions to make before or while drafting

These gate sections; the draft stalls without them.

1. ~~Is the typeclass surface member functions, or named callable data
   members?~~ **Resolved 2026-07-20 (§14): neither — the shape is
   deliberately unspecified, and the non-specification is part of the
   contract.** Consumers get call-position expression validity, small and
   freely copyable instances, no statefulness contract, and a typeclass
   that is not a resource. Consequence for move 7: **drop the
   P3294-is-C++29 timing argument entirely.** It ages badly and is no
   longer needed — the contract makes the question unobservable, which is
   a stronger and permanent answer.
2. **Standalone policy post, or compact passage only?** (§12h) I
   recommend both: the full treatment has a wider audience, will be
   argued about on its own terms, and becomes one citable thing all four
   papers point at instead of each re-litigating it.
3. **Backtick: bonus track, or integrated?** It has a real connection —
   infix spelling is what makes `combine`-heavy monoid code readable. One
   sentence either way, but unexplained it costs coherence (§9).
4. **This post vs the exhibit essay in `SPEC.md`** (§10). If this ships
   first it becomes the de facto map and its claims are what the papers
   must stay consistent with. If it is the teaser, say so and keep it
   short.
5. **Which "I want" lines survive as "I can have"** (move 4).

---

## Standing rules for this pass

- **Concede costs before anyone raises them.** Fingertree's ongoing cost,
  the `-O0` penalty, the derivation machinery's complexity. The
  concession move is the house idiom and it is what makes the rest
  credible.
- **When you concede the debug-build cost, stratify** (§13f-2). Extra
  frames at `-O0` is the ranges/`std::execution` category. MSVC's debug
  iterators are an ABI category and categorically worse. Yours is the
  former, shallow end. Lumping them invites "so you're as bad as debug
  iterators."
- **No internal planning identifiers in public text** — your own ruling.
  No decision-log numbers, status markers, SYNC entries, or note
  filenames. Real paper numbers and repo paths are fine.
- **No new arguments the papers do not already carry.** Captions state
  conclusions; the argument lives in a paper section or a linked post.
- **Cut anything unfalsifiable.** Anchor it or drop it.

## Keep these

Revision loses things. Do not lose:

- The Unicode retraction. It is the most disarming thing in the piece.
- The flat landings. "That turns out to be unnecessary." "Boost is not."
  shaped sentences.
- "Without having to go back to University and get a Maths degree." Best
  line in the draft.
- The `vector<sender<int>>` hook — promote it, do not rewrite it.
- The Tom Lehrer header.
- The Paterson aside is available if you want it: he co-authored
  *Bananas* (1991), *Finger Trees* (2006), and *Applicative Programming
  with Effects* (2008) — one person on all three pillars. Dropped flat
  and unexplained, it is in register.

## Verify before publishing

- **P2727 `iterator_interface` status** against the latest mailing. It is
  the one claim in the standards argument a reviewer will check.
- **`compile-time-forth` pushed** — currently 404, publishes first.
- **Runtime-parity godbolt check** (§13f) — derived instance vs flat
  aggregate, `-O2` and `-O0`, measuring forwarding *depth* per operation,
  not just presence of frames.
- **`std::move` / `std::forward` builtins** — the "vendors fixed the
  toolchain rather than rejecting the facility" precedent, from memory
  and unverified.
- **Re-export the `.html`.** It is stale against the citations already in
  the `.org`.

## Already done — do not redo

Citations are in the `.org` source: UAX #15, McBride & Paterson (JFP
2008), Hinze & Paterson (JFP 2006) plus `Data.Sequence`, Meijer/Fokkinga/
Paterson (1991), P2558 and P2011. Two errors already fixed — McBride &
Paterson, and *canonical* normalization.

---

## Checklist

- [ ] Map paragraph written (owns / leans / survives alone / holes)
- [ ] Lede rebuilt as three or four problems, pain not facilities
- [ ] Fingertree entry: cost conceded, `shared_ptr<const vector>`
      answered, measure tie-in kept
- [ ] Papers + What? merged, pitch first
- [ ] P4322 proposal sentence added
- [ ] Theory section shrunk and moved after the map
- [ ] "Why in the Standard" compact passage added
- [ ] "I want" → "I can have," shortened to what cashes
- [ ] Monad hole + Unicode retraction folded into the close
- [ ] Call to action, C++29 timeline sentence
- [ ] Three exhibit placeholders marked
- [ ] Backtick framing decided
- [ ] Unfalsifiable claims cut or anchored
- [ ] Five decisions above resolved
