# MUST FIX — execution plan (r1, 2026-07-21)

The five blocking findings from `CRITIQUE-transpose.md` and
`CRITIQUE-fixpoint.md`, planned as work. These apply to the posts *as
written* and are independent of the re-cast — the re-cast should start
from true text, so this pass lands first, in the source repos, under
each repo's own conventions.

Standing rules for the whole pass:

- Edit `.org` sources only; regenerate `.md`/HTML exports afterward.
  Never patch an export.
- Verify externally checkable facts (author lists) against the primary
  source at edit time, not from the critique.
- Each fix ends with a grep-based check that can be re-run cold.
- When the pass lands in both repos, add a SYNC.md entry here pinning
  the two SHAs, and move the STATUS item.

Recommended order: MF-3 (small, self-contained) → MF-5 → MF-2 → MF-1
(needs a decision) → MF-4 (widest blast radius, wants a quiet stretch).

---

## MF-1 — The sender passage asserts an integration that does not exist

**Repo:** transpose. **Files:**
`papers/blog/transposing-structure-and-context.org` (~lines 133–146);
`papers/blog/writing-algorithms-with-typeclass-objects.org` lines 253,
269 (milder overstatement).

**Decision (Steve): rewrite honestly now, or build the
beman.execution integration first.**

- **Recommended: rewrite now.** The integration is a feature with its
  own worklist, not a blocking fix; the honest version of the passage
  is still a good passage. The rewrite:
  1. Delete the phantom `#include <beman/transpose/exec.hpp>` block and
     the `ex::sync_wait` example.
  2. Transclude the actual stand-in from
     `include/beman/transpose/sender.hpp` — quoting its own "stand-in
     for a P2300 sender" comment is the disarming move, not a
     concession: the applicative instance is real, the scheduler wiring
     is deliberately out of scope.
  3. Restate the no-type-erasure argument as a property of the pattern,
     demonstrated on the stand-in — which it genuinely is.
  4. In `writing-algorithms` 253/269, "std::execution senders" →
     "P2300-shaped senders" (or "the sender-shaped stand-in").
- If instead the integration gets built: it is a vcpkg dependency, a
  real `exec.hpp`, an example under CI, and a provenance note — a
  separate work item that blocks republication until done. The passage
  then stays, re-quoted from the real example.

**Check:** `grep -rn 'exec\.hpp\|sync_wait\|not a stand-in'
papers/blog/` → zero hits; the transcluded block diffs clean against
`sender.hpp`.

## MF-2 — Two quoted snippets absent from their cited files

**Repo:** transpose. **File:**
`papers/blog/writing-algorithms-with-typeclass-objects.org` lines
62–69 and 80–86.

**Decision (Steve): re-quote reality, or make the quotes true.**

- **Recommended: re-quote reality.** Don't grow example surface to
  justify prose.
  - Explicit-object snippet (62–69): quote the actual mode-2
    demonstration — the `fmap_with` lambda in
    `examples/transpose/lookup_modes_example.cpp` — and adjust the surrounding
    two sentences to describe what it shows.
  - NTTP snippet (80–86): quote `fmap_plus_one`
    (`examples/transpose/lookup_modes_example.cpp:17–20`), the real NTTP-pinned
    demo, and drop the claim that it lives in `fold.test.cpp`.
- Alternative (only if the prose's specific shapes matter to the
  argument): add `explicit_object_lookup_example` and
  `sum_with_nttp_lookup` to the example/test files, under the test
  discipline that binds all new code (non-idempotent, order-sensitive
  algebras), then quote them.

**Follow-up, not blocking:** these were retyped quotes that drifted —
the failure mode the UUID-anchored transclusion pipeline exists to
prevent. Converting the transpose posts to anchored transclusion is the
durable fix; schedule it with the re-cast, not this pass.

**Check:** each fenced block in the post appears verbatim (modulo
stated namespace elisions) in the file its caption names.

## MF-3 — Wrong WG21 author lists, including your own paper

**Repo:** fixpoint. **Files:** `docs/blog/references.bib`; reference
sections of `part-1-fixpoint.org` and `part-4-para-apo.org`.

1. Verify each at the primary source *at edit time*: wg21.link/p2988,
   /p3019, /p0522; Hackage for data-fix.
2. Corrections, pending that verification:
   - **P2988** — Steve Downey, Peter Sommerlad (currently "Kilpeläinen,
     Peter et al.").
   - **P3019** — Jonathan Coe, Antony Peacock, Sean Parent (currently
     "Bhosale, Catmur, Berne").
   - **P0522** — James Touton (currently "Spertus, Vandevoorde"); note
     the DR handling if the entry claims more.
   - **data-fix** — Anton Kholomiov as author; Oleg Grenrus is
     maintainer, not co-author, if Hackage agrees.
3. The P2988 entry is also an orphan — part-4 never cites it.
   **Decision (Steve), small:** cite it where part-4 discusses
   `std::expected`/reference semantics if it genuinely supports a
   sentence there; otherwise delete the entry. Recommended: delete
   unless a natural sentence already wants it — a citation added to
   justify a bibliography entry is backwards.

**Check:** every `\cite`/`[cite:@…]` key in the two posts resolves to a
bib entry; every bib entry is cited from some post; author fields match
the primary sources fetched during the edit.

## MF-4 — 48 internal identifiers leak into rendered posts via header comments

**Repo:** fixpoint (the fix is in the *headers*, not the posts).
**Scope:** doc comments in transcluded headers — worst first:
`generalized.hpp` (16), the capstone/`dist_laws.hpp` cluster (10),
`zygo.hpp`/`pair.hpp` (6), `prepro.hpp` (4), plus `mendler.hpp`,
`elgot.hpp`, `histo.hpp`, `apo.hpp`, `box.hpp` (S13/S15), and any
others the inventory turns up.

Steps:

1. **Inventory before editing.** `grep -nE 'design §|D[0-9]+:?|DEV-0[0-9]|S0[0-9]|S1[0-6]'
   src/smd/ --include='*.hpp'`, then classify each hit: inside a
   transcluded region (must fix), elsewhere in a shipped header (fix
   anyway — the critique's recommendation is that the repo's public
   surface deserves the same rule), or in a `.t.cpp`/internal doc
   (leave; internal cross-referencing is allowed and useful).
2. **Rewrite markers as self-contained prose**, preserving the
   information, not just deleting it:
   - "D5: explicit" → "explicit by decision" (or the one-clause reason).
   - "DEV-03" → state the rule: "no bare CTAD on wrapped types".
   - "design §7.10" → "see the design document's generalized-schemes
     section" or drop where the comment stands alone without it.
   - "S04's convention" → describe the convention in the clause.
   The D/S/DEV numbering stays authoritative in `docs/` and `ops/`;
   headers describe, docs cite.
3. **Hazard: transclusion ranges.** Before editing, identify how each
   post pins its transcluded region (UUID anchors vs line ranges). If
   anything is line-range-pinned, editing comments shifts lines
   silently — re-render every post and diff the transcluded blocks
   against the intended regions afterward. This is the step that makes
   MF-4 want a quiet stretch rather than a quick pass.
4. Comments only — zero code changes; the full test suite must pass
   unchanged (214 tests) as the no-op proof.
5. Re-export all 17 posts.

**Check:** the marker grep from step 1 → zero hits in shipped headers;
the same grep over the regenerated `docs/blog/*.md` → zero hits; test
count unchanged.

## MF-5 — Stale `apply` in three posts (settled ruling: `ap` basis, `invoke` interface)

**Repo:** transpose. **Files/sites:**

- `how-traverse-and-transpose-work.org` 91–103: five hits; the
  textbook one-step primitive keeps its explanation but lands on the
  name `ap`, with `invoke` stated as the interface.
- `adapting-a-type-to-a-typeclass.org` 180 (the table) and 221–254:
  the careful one. The Applicative row becomes — primitive: `pure` +
  (`invoke` or `ap`); derived: whichever of `invoke`/`ap` you did not
  supply, plus `map`, `lift`, `zip_with`, … (re-derive the exact list
  from `apply.hpp` at edit time, not from memory). The simd punchline
  becomes "`ap` simply does not exist for it."
- `writing-algorithms-with-typeclass-objects.org` 230, 233, 234, 274:
  four hits, mechanical.

Not sed: each site is prose teaching the operation, and the ruling has
a shape (never *lead* with `ap`; `invoke` is the user-facing verb) that
the surrounding sentences must keep. Do it by hand, post by post, then
re-read the adapting post's simd section whole — it is the series'
best argument and must still read as one.

**Check:** `grep -n '~apply~' papers/blog/*.org` → zero;
`grep -n '~ap~\|~invoke~' papers/blog/*.org` spot-checked against
`apply.hpp`'s actual surface; the coherence skill's vocabulary gate
passes (`invoke` never `apply`, `ap` never led with).

---

## Not in this pass (deliberately)

The voice passes, ordinal-fossil repairs, recap cuts, duplicated-
sentence assignments, and companion-paper-tour rewrite are re-cast
work — they change with the new structure anyway and are planned in
`PLAN.md` and the two critiques. Mixing them into the accuracy pass
would make the diffs unreviewable.

## Decisions needed before starting

1. MF-1: rewrite the sender passage honestly (recommended) or build
   the beman.execution integration first?
2. MF-2: re-quote the real examples (recommended) or add the named
   examples to the repo?
3. MF-3: cite P2988 from part-4's expected discussion, or drop the
   orphan entry (recommended: drop unless a sentence already wants it)?

Everything else is unambiguous and can start on your word.
