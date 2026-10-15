# Critique — the transpose series (5 posts + index)

Review of `transpose/papers/blog/` in two roles: as posts to publish,
and as material to re-cast under the Monad-scarcity thesis
(`blog/PLAN.md`). Reviewed 2026-07-21 against the shipping code in
`transpose/transpose/include/` and the frozen rulings.

The short version: no banned vocabulary anywhere — the earlier voice
pass worked — and the mechanism prose matches the headers closely. But
one post asserts an integration that does not exist, one post quotes
code that is not in the files it cites, three posts still spell the
operation `apply` against the settled ruling, and the series
cross-references itself by ordinals from an abandoned four-part
numbering. All fixable; the first one is the kind a hostile reader
finds in an afternoon.

---

## 1. The sender passage claims something false

`transposing-structure-and-context.org` line 133 quotes
`#include <beman/transpose/exec.hpp>` — a header that does not exist
and never has. Lines 136–141 present real `beman.execution` senders
with `ex::sync_wait`, and say "This one is real … not a stand-in." The
repo has no beman.execution dependency, and `sender.hpp` line 8
describes itself as a deliberately minimal stand-in for a P2300 sender.
The post asserts the exact opposite of the code's own comment, and the
no-type-erasure argument that follows is presented as a property of an
integration that is not there.

Two honest exits: build the integration, or rewrite the passage around
the stand-in ("the shape is the sender shape; the wiring to a real
scheduler is deliberately out of scope here"). Either is fine. The
current text is the one option that is not, because a reader can
falsify it by cloning the repo. `writing-algorithms` lines 253 and 269
("std::execution senders") overstate the same thing in milder form.

## 2. Two quoted snippets do not exist at their cited locations

In `writing-algorithms-with-typeclass-objects.org`:

- Lines 62–69, attributed to `examples/transpose/lookup_modes_example.cpp`: no
  `OptionalFunctorObject`, no `explicit_object_lookup_example` there —
  the file demonstrates mode 2 with an `fmap_with` lambda.
- Lines 80–86, attributed to `fold.test.cpp`: no
  `sum_with_nttp_lookup` in any test; the real NTTP demo is
  `fmap_plus_one` in `lookup_modes_example.cpp:17–20`.

Both trace to older docs, never to those files. This breaks the
series' own transclusion principle. Re-quote from the real files, or
add the missing examples to the example files and transclude those.

## 3. Stale `apply` — the operation was renamed out from under the posts

The `ap`-is-a-basis / `invoke`-is-the-interface ruling settled
2026-07-14; the posts were last touched 2026-07-13. About 20 lines
across three posts still spell the one-step operation `apply`:

- `how-traverse-and-transpose-work.org` 91–103 (five hits).
- `adapting-a-type-to-a-typeclass.org` 180–254 — the heaviest: the
  Applicative table lists `apply` and `ap` as if they were two
  different operations, and the simd section's punchline reads
  "`apply` and `ap` simply do not exist for it" where there is only
  one name now.
- `writing-algorithms-with-typeclass-objects.org` 230–274 (four hits).

Mechanical to fix, but it touches the adapting post's central table and
its best section (simd-forces-invoke), so fix with care, not sed.

## 4. Ordinal fossils from an abandoned four-part numbering

`writing-algorithms` line 22 says part two showed the instance recipe
(it is part three); `prior-art` lines 22, 33, 105, 136 point at "parts
two and three" for material that lives in three and four. Replace every
ordinal cross-reference with title links — they are already broken and
will break again when the collection reshuffles.

## 5. Companion-paper text: leaked letters and drifted identity

`writing-algorithms` lines 257–258 use "(Paper C)" and "(Paper D)" —
internal coordination letters — in public running text, and describe
Paper D as being about "a recursive fixpoint tree `Fix<F>`". The actual
paper ("Algorithms for Trees", D4322R0) states the opposite identity:
an algorithms paper, with the fixpoint machinery as evidence, not the
public face. Rewrite the tour to match, with real numbers or none. The
Paper C sentence has nothing to be checked against yet — fingertree has
no paper text — so it is unpinned and free to drift; keep it to one
capability clause.

## 6. Self-repetition across posts that must stand alone

No banned words; the tells here are a foreign idiom and duplication.
"Front door" appears 13× (seven in post one) — **not Steve's idiom at
all (ruling, 2026-07-21): replace it throughout, don't thin it.** Say
what it is in each place — the verb, the entry point, the one call —
rather than keeping a pet metaphor he wouldn't use once. Duplication:
"the whole X" 14× (six in adapting); "not a limitation to apologize
for" appears near-verbatim in posts one *and* two; "pile of
one-CPO-per-operation hooks" in three *and* four. Under the each-post-
stands-alone rule, every duplicated sentence gets assigned to exactly
one owner before republication. Also: `writing-algorithms` ends on a
four-sentence "Part one showed… Part four shows…" recap — the banned
summarizing shape; cut to one line and go straight to the part-five
question.

## 7. What the review found for the thesis

Monad appears exactly once in earnest — post two's contrast paragraph
(Monad as "a real and useful capability… simply a *different*
operation, and the traversal story does not need it"), which is the
scarcity thesis stated without its name. Post one's
independence-vs-chaining section is its twin. `monad.hpp` — the
evidence the mechanism carries Monad without strain — is never
mentioned in any post. And `prior-art` line 147 ("Senders and receivers
encode the same monadic structure yet again") is the
things-keep-turning-out-monadic observation already in the text, one
step from the Deane material. The spine post should quote these rather
than restate them.

## Per-post disposition for the re-cast

| Post | Disposition |
|------|-------------|
| index | Rewrite as sub-series hub under the collection index; the current abstract's "one front door" framing is the old thesis stated flat |
| transposing-structure-and-context | Keep as series opener; fix the sender passage (§1); the independence-vs-chaining section is the docking point for the spine link |
| how-traverse-and-transpose-work | Respell `apply`→`ap`; promote the Monad-contrast paragraph — "weaker is what makes it universal" belongs near the spine's thesis |
| adapting-a-type-to-a-typeclass | Pure instances-tier post; respell the table and simd section; add one sentence noting the same three-line opt-in carries Monad instances (monad.hpp) |
| writing-algorithms | Most work: fix quotes (§2), recap ending, ordinals, companion tour (§5). Consider splitting: lookup-modes + inherit-the-instance is the most practically useful standalone content in the series and could headline its own post |
| prior-art | Nearly untouched — best voice, fully standalone; one added paragraph connecting the survey to the scarcity frame via line 147 |

## Verified clean

`transpose` never `sequence`; `identity()`/`combine()` monoid spellings;
Traversable-without-Foldable respected and stated in agreement with the
header; Monad framing "different, not needed" — never rejection; the
zip_list, simd, and fold examples match the shipping code; the
`invoke`-from-`ap`-needs-currying claim matches `apply.hpp`'s
terminating-partial machinery precisely.
