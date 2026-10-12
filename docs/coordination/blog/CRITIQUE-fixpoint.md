# Critique — the fixpoint field guide (17 files)

Review of `fixpoint/docs/blog/` (parts 0–13, the interlude, the index,
`freer-future-work`) in two roles: as posts to publish, and as material
to re-cast under the Monad-scarcity thesis (`blog/PLAN.md`). Reviewed
2026-07-21 against the shipping code in `fixpoint/main/src/`.

The short version: the prose-to-code fidelity is exceptional — roughly
sixty named claims checked against headers, examples, and test fixtures,
and effectively all of them hold. The problems are a rendering leak, a
citation error involving your own paper, a voice tic at density, and a
structure built for serialization that the posted-at-once plan breaks.

---

## 1. The rendered posts leak internal identifiers — 48 instances

Running prose in all 17 org files is clean. But the posts transclude
shipping headers whose doc comments carry the internal markers — "design
§7.10", "D5: explicit", "DEV-03", "S13" — so the *rendered* posts
violate the public-text-stands-alone rule 48 times. Worst offenders:
part-11 (16), part-12 (10), part-5 (6), part-6 (4).

Since "the code in these posts is the shipping code, not a
simplification" is a stated series principle, there are only two honest
fixes: scrub the markers from the header comments in the fixpoint repo
(they survive fine as prose — "explicit by decision"), or trim
transclusion ranges to exclude doc comments. The former also fixes the
repo's own public surface, and is the right call.

## 2. Three WG21 citations have wrong author lists

- **P2988** (references.bib, cited from part-4): credited to
  "Kilpeläinen, Peter et al." P2988 is by **Steve Downey and Peter
  Sommerlad**. The series miscredits its author's own paper. It is also
  an orphan — part-4's body never cites it.
- **P3019** (part-1): credited to "Bhosale, Catmur, Berne"; actual
  authors are Jonathan Coe, Antony Peacock, Sean Parent.
- **P0522** (part-1): credited to "Spertus, Vandevoorde"; actually
  James Touton (Hubert Tong on the DR). Verify against wg21.link before
  publishing.
- Minor: data-fix credited to "Kholomiov and Kmett"; Hackage lists
  Kholomiov as author, Grenrus as maintainer.

All the academic citations (Meertens, Mendler, Böhm–Berarducci, Elgot,
Uustalu–Vene–Pardo, Hinze–Wu–Gibbons, Kiselyov–Ishii, van der
Ploeg–Kiselyov, P2300) check out.

## 3. The one detectable generation signature: the "worth/earns" tic

Twelve-plus instances across ten posts of the same personification move:
"repay attention", "worth naming", "deserves its moment", "earns its
comment", "earning its keep", "worth the price of admission", "repay
reading", "earns a fusion". Steve's own read (2026-07-21): the words
are, possibly, tics he has — the *pattern* is not. So the fix is
density, not vocabulary: keep two or three ("worth the price of admission" in
part-12 is the best), rewrite the rest as flat statements — "Get the
NTTP mechanics wrong and the whole mode is unusable" instead of "The
mechanics of that NTTP repay attention."

Related, lower priority:

- "honest/honestly/candid" ~12×, five in the interlude alone.
- Epigram landings on nearly every section. The flat landing is the
  voice; the *rate* is not, and a few go grand ("Once you see it here,
  you see it everywhere" — cut; end part-13 on "the tests are the
  territory," which is already there).
- The tricolon "a fixpoint type, a constexpr Box, and typeclass objects"
  appears verbatim three times (index, part-0, part-13). Once.
- Parts 2–5 and 7–13 are almost entirely impersonal ("the library…",
  "the design log defends it") against the owns-the-opinion habit. The
  interlude and freer-future-work show the target register. Part 3's
  naming argument and part-12's thin-ice section would gain most from
  "I" statements.

## 4. The series recaps itself three times

The index, part-0's roadmap, and part-13's "Field Guide in Review" are
the same material at three lengths, and part-13's version is the banned
summarizing-conclusion shape at full length. Keep the index; cut
part-0's roadmap to a paragraph; cut part-13's recap to two sentences
and let "What C++ Pushed Back On" — the genuinely synthetic section —
be the post.

## 5. Serialization scaffolding the re-cast breaks anyway

Every post ends on a next-part teaser; no post ends on an open question
or a casual sign-off. Posted at once as a series-of-series, all 17
next/prev chains need rewriting, which is the natural moment to fix the
endings too.

## 6. What the review found for the thesis

The honest count, from reading all 2,800 lines against the code: Monad
is load-bearing in exactly three places — gana/futu (part-11), the
Freer interpreter (freer-future-work), and the interlude's derivations.
Everything else is Functor or Comonad territory. The scarcity claim the
spine post makes is not a framing device; it is what the series already
demonstrates. Part-7's symmetry ledger ("every right-column entry is
gana at a monad") is the thesis in miniature and a candidate opening
artifact for the algorithms tier.

## Per-post disposition for the re-cast

| Post | Disposition |
|------|-------------|
| index | Rewrite as two-level collection index under the spine |
| part-0 | Superseded by the spine post; only the "private zoo" motivation paragraph survives, donated to the spine or the series lede |
| part-1 (Fix/Box) | Untouched; best-voiced technical post. Re-aim the forward pointer |
| part-2 (typeclass objects) | Keystone of the typeclass story; needs a sibling covering the Monad/Applicative/Traverse instances the code ships but no post covers |
| part-3 (classical trio) | Early algorithms-tier placement; the naming section is the cross-repo vocabulary anchor — keep synchronized with Paper D |
| part-4 (para/apo) | Split candidate: apo stays algorithms; the either section moves to instances and picks up the Monad-instance discussion it gestures at |
| part-5 (zygo/mutu) | Link surgery only |
| part-6 (prepro/postpro) | "Shape of the Delta" merges into part-11's opening |
| interlude (free/cofree) | Promote toward the front; split into 2–3 posts (Free/Cofree \| Church/Freer \| sender-receiver). The sender/receiver section is the landing pad for the Deane material |
| part-7 (histo/futu) | Merge part-8 into it: one course-of-values post |
| part-8 (dyna/chrono) | Merged into part-7 (it is explicitly "3+7 composed" and thin alone) |
| part-9 (Mendler) | Stands; pairs with the interlude's Freer section ("two ways to stop paying the Functor tax") |
| part-10 (Elgot) | Stands; promote "test the work, not the answer" to a visible beat — the most quotable engineering advice in the series, buried in scheme 14 of 20 |
| part-11 (generalized) | The largest structural change: moves from reveal-at-the-end to thesis-made-precise near the front of the algorithms tier; worst leak count (16) to fix first |
| part-12 (capstone) | Split the EnvT-instance section into the instances tier (it is about the registry, not the algorithm) |
| part-13 (conclusion) | Kill the recap; promote "What C++ Pushed Back On" to its own post; new conclusion answers the thesis with the honest count from §6 |
| freer-future-work | Truest to voice in the set; slots directly after the reflection post in the instances tier |

Net-new writing the re-cast requires, beyond the spine post: the
instance posts. `monad.hpp`, `apply.hpp`, `traverse.hpp`, `sequence.hpp`,
either/pair/Identity all have code and tests but no posts, and the Deane
reflection material appears nowhere in the repo yet.

## Small fixes

- freer-future-work line 101: a bare `*` org headline renders as an
  empty heading; use a rule or title it.
- freer-future-work says one_shot is "roughly thirty lines"; the class
  is ~75 lines in a 120-line header. Be exact.
- part-11: the dist-law roster lists `dist_apo` plainly; the header
  requires `.operator()<X>(...)` call syntax. The post makes no syntax
  claim, so not a mismatch — but one clause would forestall the
  correction email.
- part-7's abstract: "the most interesting data structures in the
  library" is the closest thing to applause in the series; "the two
  carriers that do real work" is flatter and more him.
