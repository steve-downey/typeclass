# Draft review — `trees-and-transposition-for-29` (2026-08-06)

Review of the uncommitted working-tree revision of
`essay/trees-and-transposition-for-29.org` against the existing notes:
`essay/SPEC.md`, `essay/CRITIQUE.md`, `essay/EDIT-LETTER.md`, and the
`tree-proposals-coherence` skill invariants.

**Summary.** The revision executes the *reordering* moves from the edit
letter — new problem-list lede, Unicode retraction and "I want" moved to
the back, backtick demoted to a bonus track in the links — but none of
the *writing* moves. The middle is untouched, which is precisely the
failure mode CRITIQUE §11d warned about: "a better lede and a better
close around an unchanged middle produce a more readable essay that
still does not accomplish its stated purpose." One coherence invariant
is currently violated (Monad silence).

---

## What the revision accomplished (against the edit letter)

- **Lede rebuilt as problems** (move 2) — done and well done. Four
  pain-first lines, no facility names, `vector<sender>` first, every
  paper represented, backtick correctly absent. Matches §11b almost
  exactly.
- **Unicode retraction moved out of position one** (move 8) — done; it
  now sits just before the close.
- **"I want" moved to the end** (part of move 4) — position only.
- **Backtick bonus-track label** (decision 3) — partial: the link list
  now says "Bonus tracks," but P4307 still sits in "The Papers" with
  full standing, and the one-sentence tie-in (infix makes
  `combine`-heavy monoid code readable) is absent.

## What remains — in the edit letter's own priority order

1. **The map does not exist** (move 1, top priority in both notes).
   Nothing in the draft says who owns what, who leans on whom, or what
   survives alone. The abstract still asserts "they lean on each other"
   without ever saying which — the exact gap CRITIQUE §3 opened with.
   The §16b poster case (fingertree's O(1) `length` motivating Paper D's
   overridable members) is available as a concrete cross-paper
   connection and unused.
2. **"The Papers" and "What?" are still two sections, committee register
   first** (move 3). P4322's entry is still the only one with no
   proposal sentence — one paragraph of problem statement.
3. **Fingertree body pitch unchanged** (move 2's second half). The
   lede's persistence line is right, but the "Fingertrees" section still
   pitches generic persistence: no cost concession, no
   `shared_ptr<const vector>` answer, no hand-off framing. The measure
   tie-in is present ("clever annotation of the interior nodes").
4. **"I want" is moved but not reframed** (move 4). Still titled "What I
   want," still the full list: the OCaml-soundness and NumPy/Rust lines
   the edit letter said to cut-or-anchor are verbatim.
5. **Missing entirely:** the "why in the Standard" passage (move 7);
   exhibits or placeholders (move 6 — ``a `f` b`` is still the only
   code); call to action and C++29 timeline sentence (move 8); theory
   section unmoved and unshrunk.

## Coherence-skill findings

- **Monad silence is an invariant violation, not just an editorial
  gap.** The skill is explicit: "never let a paper go silent on it… the
  essay acknowledges the open slot." The draft has zero mention of
  Monad / deferred sequential composition. Per edit-letter move 5, two
  sentences in the close discharge it.
- **Paper numbers need a verify pass before publication.** The skill
  records Paper D as **D4322R0** and Paper C as **D4325R0**; the essay
  writes P4322 and P4325. Correct *if* it publishes at/after the
  2026-08-14 mailing, but the skill's hazard note says to verify
  wg21.link references first — wg21.link/P3200 currently resolves to the
  placeholder title "Trees for the standard library," not the title the
  essay gives it. P4307's number appears nowhere in the coordination
  notes; verify it is real, not penciled.
- **Monoid definition is slightly wrong as written.** "The magic of
  `monoids`, types with associative binary operations" describes a
  semigroup — the identity element is missing, and identity /
  `identity()` is half of the frozen monoid vocabulary. One clause
  fixes it.

## Small things

- Typo in the showcase position: "You **need have** a pure function"
  (lede, line 38). Also "Some times" (line 142).
- The HTML export is stale — dated 2026-07-19, before the restructure
  (it lacks the new lede); the edit letter already flagged re-export as
  required.
- The edit letter's five gating decisions: only #1 (instance contract)
  is resolved. #2 (standalone policy post), #3 (backtick framing in
  body), #4 (relationship to the SPEC.md exhibit essay — this draft
  still doesn't declare itself teaser or map), and #5 (which wants
  survive as "I can have") remain open, and #3–#5 block the remaining
  sections.

---

**Net:** the revision is real progress on the frame, but the essay's
stated job — "read these four as a set" — is still done nowhere in the
text. If only one thing happens next round, it is the map.
