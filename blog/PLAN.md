# Blog collection — re-cast plan (2026-07-21)

Working name: the **monadic-algorithms collection**. A series of series,
published at once on sdowney.org, organized by one thesis:

> Few algorithms require Monad in an interesting way.

Positioning (Steve, 2026-07-21): this collection sits slightly to the
side of the campaign announcement (`essay/trees-and-transposition-for-29`)
but is closely related — it is the **partial, advance explanation of why
Monad is not a focus of the core work**. The papers' "Why not Monad"
sections and the campaign essay's Monad answer stay short because this
collection carries the long form. It also serves the exhibit essay's exhibit-holes
("the holes, shown not argued") as the argument that exists at
publication time.

Constraints carried from the coherence skill, restated for this work:

- Every post must be of actual use on its own — a reader takes something
  away without reading the rest. (The "each must earn its slot" test.)
- Public text stands alone: no internal planning identifiers in running
  text. Paper numbers and repo paths are fine.
- Vocabulary is frozen: `invoke` (never `apply`), `transpose` (never
  `sequence`), `identity()`/`combine()`, `fold_fix`/`unfold_fix`/`refold`
  in Paper-D territory; the fixpoint field guide may use classical names.
- Monad framing is always "deferred, not rejected," never rejection.

## The shape

```
Where Are the Monads?                    ← spine post (new, drafted)
├── Transpose, Traverse, and Typeclass   ← series (5 posts, exist)
│   Objects — the Applicative story
├── Recursion Schemes in C++26           ← series (15 posts, exist)
│   — the field guide; comonads on the
│   fold side, monads on the build side
├── Instances                            ← series (new, small)
│   — descend into the instances and
│   show each moderately useful;
│   Deane's reflection monad is the
│   payoff instance
└── (papers, campaign essay, exhibit     ← linked, not part of the
     essay — the formal layer)              collection
```

**The spine post** (`blog/where-are-the-monads.org`, drafted 2026-07-21):
the ledger of what each algorithm actually demands, a quick run-through
of Functor → Applicative → Monad as three grades of composition —
lifting preserves ∘, applicative combining is independent, and Kleisli
composition is the context-dependent ∘ (Steve's framing, 2026-07-21) (Functor almost
always, Monoid for queries, Applicative for traversal, comonads/monads
only structural and library-supplied), the concession that C++ is full
of monad *instances* while the *abstraction* has almost no algorithm
clients, the mapM→traverse precedent (McBride & Paterson), and the turn:
Ben Deane's observation that C++26 reflection ships a monad in the
language itself (`^^` is pure, `substitute` is fmap,
`extract<std::meta::info>` is join — elbeno.com, 2026-03-04), which is
the first client that cannot solve its problem with a per-type member.
Ends on the map of the collection.

**Series 1 — transpose (exists).** Already reads as the Applicative
story, but the accuracy pass is not light — see
`blog/CRITIQUE-transpose.md`: the sender passage in post 1 asserts an
integration that does not exist, post 4 quotes code absent from its
cited files, three posts still spell `apply` against the settled `ap`
ruling, and ordinal cross-references fossilize an abandoned numbering.
Re-cast proper: index and post 1 link up to the spine; post 2's
Monad-contrast paragraph is the thesis-in-embryo the spine quotes
rather than restates.

**Series 2 — fixpoint (exists).** The field guide stays a field guide.
Re-cast touches: Part 0 gains the up-link and one sentence placing the
guide on the thesis map; Part 11 (distributive laws) is the structural
evidence for the thesis — fold side comonadic, build side monadic, all
scaffolding — and should say so in one place; Part 13's ledger gains the
callback to the spine's ledger. No renumbering, no rewrites beyond what
the per-post critiques call out.

**Series 3 — instances (new, small).** The "descend into the instances"
layer the thesis promises. Candidates, in priority order:

1. **The reflection monad, made to work** — register
   `monad_typeclass<std::meta::info>` with `pure = ^^`-shaped lift,
   `bind = substitute ∘ extract`, then Kleisli-compose actual type
   functions with the shipped `kleisli` derivation. Deane showed the
   laws; this post shows the typeclass mechanism carrying a
   language-supplied instance, which is the collection's whole pitch in
   one program. C++26/gcc-16 code, so it lives in the fixpoint repo's
   orbit (evidence, never proposed) — placement to confirm.
2. **Either/Free/Cofree as working types** — largely covered by the
   existing free/cofree interlude; likely a cross-link plus a short
   standalone framing rather than a new post.
3. Further instance posts only if each clears the "of actual use"
   bar on its own.

## Publication mechanics

- Venue: sdowney.org, per the standing publication decision; drafts live
  in the repo blog dirs where the code is, exports as usual.
- Posted at once, cross-linked: spine ↔ series indexes ↔ posts. The
  spine post is the landing page for the collection.
- The campaign essay and the papers link to the spine post for the Monad
  question; the exhibit essay's exhibit-holes caption cites it.

## Order of work

1. Per-post critiques (in `blog/CRITIQUE-fixpoint.md`,
   `blog/CRITIQUE-transpose.md`) — done as review, applied per Steve's
   direction.
2. Spine post revision after Steve's read.
3. Light re-cast edits to the two series indexes and framing posts.
4. The reflection-instance post (needs compiler experiments first —
   does `substitute`/`extract` composition through the Monad CRTP
   actually work consteval on gcc-16?).
5. Publication pass: links, teasers, export.

## Open questions for Steve

- Title check: "Where Are the Monads?" for the spine post.
- Where does the reflection-instance code live — fixpoint (C++26 floor
  matches) or a new scratch repo? The coherence rule says the exotic
  catalog stays in fixpoint; a reflection monad instance is the same
  kind of evidence.
- Does the instances series subsume the existing free/cofree interlude
  by reference, or does the interlude stay purely inside the field
  guide?
- The spine post claims "the reference tree contains a retry program
  over a Freer monad, and it earns its keep" — happy with that as the
  honest counterexample, or should the Freer material stay quieter?
