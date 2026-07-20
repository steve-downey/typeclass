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

# Part II — plan review (2026-07-20)

Review of the restructuring plan, and of a proposed new section. Steve
retains stet rights on everything below.

Obsolete finding: the earlier fact check flagged
`github.com/steve-downey/compile-time-forth` as a 404. It publishes
before the essay does, so the link stands. No action.

## 11. The three moves in the plan

All three are right. Two need a guard rail, and the plan has one
significant gap.

### 11a. "I want" → "I can have" at the close

Do it. The reframe earns something the opening version cannot: by the
time the reader arrives, the body has shown the machinery, so each line
reads as a receipt rather than a wish.

The guard rail is that "I can have" is a claim of achievement, and the
list does not currently survive being read that way.

- "as expressive as Python, Rust, Swift, or Haskell" — mostly cashes.
- "without a Maths degree" — cashes completely, and is the best line in
  the list. It *is* the thesis of packaging the vocabulary.
- "as correct and as sound as OCaml" — does not cash and cannot.
  Soundness is a property of a type system; C++'s does not become sound
  because these papers land. The defensible narrower claim is that
  idiomatic use of these algorithms does not require reaching for the
  unsafe iteration their equivalents need today.
- "as, or more, performant than NumPy or Rust" — needs a number or a
  link behind it. The evidence exists; the essay currently points at
  none of it.

Make the "I can have" list visibly **shorter** than the "I want" list
was. Cut what cannot be cashed rather than softening it into mush; the
asymmetry is honest and in-register. Some wants stay wants.

That opens a better move: the things that *cannot* be had yet are the
campaign's shape, not a weakness. Sequential composition — the Monad
slot — is exactly a thing deliberately not yet had. Folding it into the
close means the deferred-not-rejected acknowledgment the campaign
invariant requires stops being a defensive mid-essay section and becomes
part of an honest ending. This supersedes the placement proposed in §4.

Caution on "for anyone who gets that far": if this is the closing
argument, skimmers will scroll straight to it and read it first. Build
it to survive being read cold — a list, not a paragraph.

### 11b. Lede as a list of problems

Right call, and the fix for §1. Two things to hold to.

Three or four entries, one line each, each naming the *pain* — the loop
written again, the copy paid for, the recursion hand-rolled a fourth
time — not the facility. The existing sender line is already exactly
this shape and is the model for the rest. A problem list that names
facilities becomes a feature list, which is the failure mode.

Order by how widely the pain is felt, which probably puts the
`vector<optional>` / `vector<sender>` flip first. Do not force a
one-to-one map from problems to papers; two problems landing on
transpose is fine. But check that every paper is represented somewhere,
because a paper absent from the lede is implicitly demoted. That argues
for leaving the backtick out of the lede entirely and letting it be the
bonus track, consistent with §9.

### 11c. Fingertree on persistence and the hand-off

The sharpest idea in the plan, and the diagnosis behind it is right:
fingertree loses every single-operation comparison to a purpose-built
structure, so pitching it on any one operation pitches it into its
weakness. Persistence is the property with no bespoke competitor.

Two stress tests before building the lede entry on it.

**"Zero cost" will be attacked, correctly.** The hand-off is O(1) and
allocation-free, but ongoing operations cost more than `vector` —
allocation, indirection, cache behaviour. State that first rather than
letting a commenter state it; the concession move is the house idiom,
and being first to name the cost is what makes the claim credible. The
precise claim is about the copy that is not made, not about free
operations.

**The comparison that needs answering is `shared_ptr<const vector>`**,
because it is the closest competitor and is already cheap. It gets the
O(1) hand-off. What it does not get is continuing to work: freezing it
freezes everything, and resuming mutation costs a full copy. Fingertree
gives the O(1) hand-off *and* cheap continued mutation of your own
version. That is the differentiator and it should be explicit — it is
the difference between "persistent" sounding like an academic property
and sounding like the answer to a problem the reader has. Naming the
workarounds people use today (copy the vector; take a lock and block the
producer; hand-roll a snapshot scheme) lets them recognise their own
code.

**Scope risk.** Persistence sells the container, but the *measure* is
what makes it a fingertree rather than any persistent sequence, and the
measure is what connects it to the monoid vocabulary shared with the
rest of the campaign. An all-persistence pitch detaches it from the
campaign, which is the one thing this essay exists to prevent.
Persistence is the hook; the annotation story is the tie-in and needs at
least a clause — that changing the measure makes the same structure a
sequence, a priority queue, or an interval map.

### 11d. The gap in the plan

The plan reworks both ends of the essay and leaves the middle alone. The
middle is where the "read these as a set" job actually gets done: the
cross-paper map (§3) and the merge of the duplicated Papers/What?
sections (§2). Neither is addressed. A better lede and a better close
around an unchanged middle produce a more readable essay that still does
not accomplish its stated purpose. **The map is the highest-priority
item in the next round, above both ends.**

### 11e. Two consequences of the reframe

The Unicode retraction becomes homeless — under a problems→machinery→
resolution frame it is neither. It fits the close well: one thing set
out for, then decided against. That strengthens "I can have" by showing
the list was filtered rather than accumulated.

The HOF / internal iteration / typeclasses section becomes the "how"
once problems lead, so it shrinks and sits after the map.

## 12. Proposed new section: why in the *Standard*

**Verdict: yes, and it answers the question that actually decides these
papers.** "Why should this exist" is winnable while still losing the
vote. Build it as follows.

### 12a. The argument is stronger than the frame

Batteries-included vs austere-prelude is a frame, not an argument. The
argument underneath — vocabulary has value only if shared, so a
vocabulary mechanism outside the standard is close to worthless — is the
strong thing. Lead with it; let the frame emerge. Concrete first.

The anchor precedent is `std::optional`. Nobody argues it should not be
standard because you could write your own; everybody *did* write their
own, and that was the problem — `boost::optional`, `folly::Optional`,
`absl::optional`, `llvm::Optional`, mutually incompatible at every API
boundary. `string_view` and `expected` tell the same story. That is the
exact shape of the typeclass claim: two libraries that each ship their
own Functor instance do not compose, and composition was the point.

This also disposes of the standard reply — "get field experience first,
ship it on GitHub." For a container that is correct advice and it works.
For shared vocabulary it is a catch-22: you cannot accumulate field
experience with sharing that is not shared.

### 12b. Do not claim the four papers need this equally

They do not, and conceding the gradient is more persuasive than claiming
uniform urgency.

- **P3200** — the pure case. Vocabulary and nothing else; near-worthless
  unstandardised.
- **P4322** — strong but weaker. The algorithms are usable from a
  third-party library; the hook that lets *your* tree join in only pays
  off if the algorithms are universal.
- **P4325** — the weak case for this argument; concede it outright. A
  persistent measured sequence is fine as a third-party library. Its
  standard-library case rests on different ground: containers crossing
  API boundaries, allocator/PMR integration, and the measure needing A's
  monoid vocabulary.
- **P4307** — off this spectrum entirely. A language change cannot be a
  library at all.

Spelling out the gradient doubles as campaign-map material: four
differentiated asks, not one undifferentiated one.

### 12c. The "eventually" argument — the climax, with facts checked

**Iterators is the proof.** Concept standardised 1998; the authoring aid
never was. Boost.Iterator's `iterator_facade` has been the de facto
answer for twenty-five years. `std::iterator` was deprecated in C++17 and
is being removed in C++26 ([P3365R1]), while its standard replacement,
Zach Laine's [`std::iterator_interface`][P2727], is still in flight at
R4. Twenty-eight years of "eventually," and the interim aid is removed
before the replacement lands. Verify P2727's status against the latest
mailing before publishing; it is the one claim a reviewer will check.

**Coroutines needs restating.** `std::generator` arrived in C++23 and
[`std::execution::task` (P3552R3)][P3552] was approved in Sofia for
C++26. The support did come — six years and two revisions after the
language feature. Still the point, but "two revisions late," not
"never"; the stronger phrasing will draw a correction.

**The use/authorship split is what makes the coroutine example work**
(Steve, 2026-07-20). `task` and `generator` are coroutine types you
*consume* — batteries, and welcome ones. Neither is support for *writing*
the type that owns the handle and the frame and exposes the
compiler-called extension points. That is the same split as iterators
exactly: using them has been fine since 1998; writing one correctly is
what Boost covered for twenty-five years. Stated that way the two are not
a strong example and a weak one — they are the same failure twice, and
the recent one is more damning. Nobody can wave off 2020 as the old
committee.

**The extension-point surface is the concrete evidence**, because it is
countable and most readers have never counted it: `get_return_object`,
`initial_suspend`, `final_suspend` (noexcept or ill-formed),
`unhandled_exception`, `return_void` vs `return_value` where both is an
error and neither is a different error, `yield_value`, `await_transform`
(all-or-nothing; silently disables ordinary awaitables unless forwarded),
the frame allocation overloads including sized delete and the
`get_return_object_on_allocation_failure` nothrow path, `coroutine_traits`
as the alternate hook, then the awaiter protocol with `await_suspend`'s
three return conventions including symmetric transfer — and underneath
it all the ordinary RAII problem of a move-only handle owner that must
null out on move and destroy exactly once. Every one is compiler-called,
and the diagnostics range from unhelpful to absent.

**A third example worth adding**, because P3200 fixes it: customisation.
Twenty years of ADL-plus-traits, `tag_invoke` proposed and never
adopted, senders shipping their own mechanism. The gap P3200 addresses
is itself the longest-running unkept "eventually" in the library.

### 12d. The generalisation — this is the section's actual thesis

The part that gets deferred is always the **authoring** support, and it
gets deferred *because* it is never strictly essential. You can always
hand-write the promise type, the `iterator_traits`, the customisation
dispatch. Essential things force themselves into the paper; non-essential
ones get dropped to ship, and the "eventually" that should follow has no
forcing function behind it, so it does not arrive.

**The mechanism** (Steve, 2026-07-20): the people who could build it
afterward are permanently absorbed by the next large feature — everyone
who might do coroutine authoring support is busy with `std::execution`.
Neither cause is anyone's fault and neither self-corrects. Pattern plus
mechanism is an explanation; pattern alone is an anecdote. Keep it dry
and structural — the moment it reads as grievance it stops working.

Three instances make it a pattern. And it is precisely the category the
typeclass authoring machinery falls into, by Steve's own description of
it — which is the argument for shipping it *with* the vocabulary rather
than after.

**The safety tie-back.** A hand-written iterator that is subtly wrong
gives strange overload resolution. A hand-written coroutine type that is
subtly wrong gives a dangling frame or a leak that compiles clean. The
essay already argues that moving unsafe operations into small reused
blocks improves safety at little or no performance cost; authoring aids
are that argument applied to extension points. One clause, so the
section does not read as purely about ergonomics.

### 12e. Preempt the regex objection

The strongest counter to any "put it in the standard" argument is that
the standard is where libraries go to die. The first answer is that the
trap is ABI-frozen *implementations*, not concepts and customisation
points; regex's problem is that nobody can change the implementation.

The more persuasive answer is what the campaign already demonstrates.
The freeze risk is real, which is why P3200 proposes only what its
three-domain proof forces, why the exotic recursion-scheme catalog stays
in the fixpoint repo and ships in nothing, and why Monad is deferred.
**The scoping discipline is the response to the regex objection**, and
pointing at it turns a defensive section into evidence that this campaign
was designed by someone who takes the freeze seriously. It also ties the
section back to the map instead of floating free as policy commentary.

### 12f. The sharpest observation available

"Neither" is the line. C++ ships `valarray`, `locale`, `regex`, and
iostreams, and has no JSON, no HTTP, no Unicode algorithms, no graph, no
matrix. That is not a policy; it is an accretion of whatever got proposed
and championed. Which means "that doesn't belong in the standard" is
essentially never the application of a principle — there is no principle
to apply.

Two cautions. Keep it wry, not aggrieved; this is an argument made inside
the room. And it cuts both ways — no policy also means "sure, why not" is
not an argument for inclusion, so the section must land on what the
criterion *is*. The implicit answer is already good: standardise what is
worthless unshared; leave what works fine as a library to be a library.
Short, defensible, and it sorts the four papers honestly.

### 12g. The complexity objection, and the one that actually threatens it

The symmetry Steve raised (2026-07-20) is real and should not be talked
around: coroutine authoring support does not exist partly because nobody
has found a simple, obviously-correct design — and the same criticism
applies to the typeclass authoring side, which is complicated and uses a
lot of contemporary machinery.

**One distinction favours the typeclass case.** Coroutine authoring is
hard partly because the extension points are a compiler-called protocol
you do not control. The typeclass machinery is complicated, but it is
ordinary library code with ordinary semantics — no compiler protocol,
nothing the reader cannot step through. Worth a sentence, or "complicated
like coroutines" invites the reader to assume it is complicated *in the
way* coroutines are.

**The reframe is already in the draft.** The essay says the derivation
machinery is complicated but consumers of an instance do not care — they
look up a named operation and never see how it was provided. That answers
this objection and is currently sitting in the theory section doing
nothing. The standard library is full of complicated machinery; `sort`,
`regex`, and `std::execution` are not simple and nobody proposed waiting
for simple versions. The bar was never implementation simplicity. Stated
fairly, the objection is about *specification* burden — complicated
things are hard to review and to word, and mistakes are permanent. That
is legitimate, and it is answerable with evidence, which is what the
fixpoint catalog and the four-representation implementation experience
are for. Aim at specification burden and the argument is about receipts
rather than taste.

**Tactically: make the authoring side separable and say so.** If the
vocabulary is what is worthless unshared and the authoring machinery is
the complicated part, structure the ask so the vocabulary survives the
authoring side being cut — the way the explicit-argument spellings
already survive without the lookup layer. Then say plainly that the
authoring support is proposed *now* rather than promised later, and that
cutting it is exactly the cut that produced twenty-eight years of no
iterator authoring aid. That converts a silent deferral into an explicit
decision someone has to own, which is more than the coroutine case got.

**The objection that must be answered is reflection, not complexity.**
Reflection landed in C++26. The most likely thing to kill the authoring
side in 2026 is not "it's complicated" — it is "wait two years,
reflection makes this trivial, don't standardise the pre-reflection
version." That is serious, it will occur to the room, and hearing it
first from someone else is the back foot. A position is required: either
reflection handles the mechanical enumeration but not the derivation
semantics, so it simplifies the implementation without moving the
interface; or the design is reflection-ready and the surface does not
move when the implementation is rewritten; or — worth honestly
considering — the authoring side should be rebased on reflection now and
shipping the pre-reflection version is the wrong call. Whichever it is,
it belongs in the section: "we deferred it waiting for the elegant
version" is the failure being argued against, and reflection is a
plausible elegant version rather than a hypothetical one. Raising and
answering it is far stronger than not mentioning it.

### 12h. Placement and size

It is an objection-handler, so it belongs **after the map**, once the
reader knows what is being asked for; early reads as defensive. It pairs
naturally with the "I can have" close — things that can be had, but only
in the standard, because the eventually does not arrive on its own.

**Size is a real problem.** This wants to be a policy essay, and
`SPEC.md`'s discipline says the map essay makes no new arguments and
links to where each argument lives. Recommendation: write the full
treatment as **its own post**. It has a wider audience than the campaign
essay, it will be argued about on its own terms, and it becomes a single
citable thing all four papers point at instead of each re-litigating the
question. The campaign essay then carries a compact version — the
vocabulary-is-worthless-unshared claim, the twenty-eight-year iterator
example, and a link.

[P3365R1]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3365r1.pdf
[P2727]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2727r3.html
[P3552]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3552r3.html

---

## Priority order for the next revision

1. **The campaign map** — who leans on whom, what survives alone (§3,
   §11d). Highest priority; the plan does not currently cover it, and it
   is the essay's actual job.
2. New lede: the problem list (§1, §11b), fingertree pitched on
   persistence and the hand-off, with the cost conceded and the measure
   tie-in kept (§11c).
3. Merge Papers + What? into one pitch-first section per paper; add the
   missing P4322 proposal sentence (§2).
4. Move "I want" → "I can have" close, shortened to what cashes, with
   the Monad hole folded in and the call to action (§6, §7, §11a).
   Supersedes the standalone Monad section proposed in §4.
5. Placeholder code blocks now; transcluded anchors next round (§5).
6. Move the Unicode retraction into the close; one C++29 timeline
   sentence (§8, §11e).
7. Compact "why in the Standard" passage after the map, with the full
   treatment spun out as its own post (§12).
8. Decide backtick framing (§9) and relationship to the exhibit essay
   (§10).

## 13. Reflection — Steve's position (2026-07-20) and assessment

### 13a. The position as stated

Reflection could emit a completely flat aggregate — a struct with exactly
the named operations — from a base set of implementation functions. No
runtime cost difference either way (**claim to check; tracked in §13f**),
so it reduces to compile-time cost of producing the typeclass. The
present mechanism's cost is not zero but not high: no real
metaprogramming, no SFINAE, just deducing-this and CRTP. No mechanism is
visible by which reflection replaces typeclasses entirely. There may be
room for `Derive{Functor,Foldable,Traversable,Lift}` — good prior art —
but those carry constraints meaning not all instances are derivable. And
"there should be only one way to do this" is a standard way of killing a
feature, usually unprincipled even when phrased as principle; q.v.
`optional`, `indirect`, `polymorphic`, `shared_ptr`, `unique_ptr`, all
overlapping on ownership.

### 13b. The reframe that dissolves most of the objection

**Reflection is a code-generation mechanism; a typeclass is a vocabulary.**
Reflection can generate an instance; it cannot *be* the instance, and it
cannot be the thing consumers look up. So the two are not competitors at
any level. Reflection is a candidate *implementation strategy for the
authoring side*, competing with CRTP-plus-deducing-this — not with the
typeclass surface.

That shrinks "wait for reflection" from a standardisation objection to an
engineering choice. And if the output is a flat aggregate with named
operations either way, the surface is identical and the derivation
strategy is not an interface decision at all — a later revision can swap
it without breaking anyone. This is structurally the same argument that
carried `std::execution::task` (§12c): the machinery is instantiated in
user code, so its strategy can change between revisions without an ABI
break.

**Wording consequence, actionable:** specify the derived thing by its
*shape* — an aggregate providing these named operations with these
semantics — never by how it was derived. If the wording pins the
derivation mechanism, the reflection objection becomes real. If it pins
only the shape, the objection answers itself.

### 13c. The C++26 / C++29 fact, with the honest caveat

`std::meta::define_aggregate` in [P2996][P2996] completes a type with
**data members** only. Injecting *member function definitions* needs
token injection — [P3294][P3294], Revzin/Alexandrescu/Vandevoorde —
which is **C++29 material, not in C++26**.

So on the face of it, "wait for reflection" means waiting for the same
standard being targeted.

**But do not lean on this without checking the shape of the design.** If
the typeclass surface is *member functions with bodies*, the argument
holds and reflection cannot do this until C++29. If the surface is
*named data members of callable type* — which is what "a record that
collects a coherent set of named operations" and "structs, with named
operations" describe, and which matches "using a base set of
implementation functions" — then `define_aggregate` may well be able to
emit it in C++26, and the timing argument evaporates. Determine which
before using this in public.

Either way §13b still holds: it is an implementation strategy, and the
surface does not move.

### 13d. Where reflection could genuinely be better

Answering the question as posed — significantly better in some dimension:

- **Runtime:** no, on present understanding (§13f). Both are compile-time
  constructions that inline out.
- **Compile time:** unlikely to be decisive. Reflection is not free, and
  token injection is not free; the present mechanism avoids SFINAE and
  heavy metaprogramming, which is where compile-time cost usually lives.
- **Diagnostics:** *this is the real candidate.* CRTP error messages are
  notoriously bad, and a flat aggregate with named members may diagnose
  and tool considerably better than a derivation chain. If reflection
  wins anywhere, it wins here, and diagnostics is a dimension LEWG cares
  about.
- **Tooling / IDE:** minor win in the same direction.

Conceding diagnostics is cheap and buys credibility, because it is still
an implementation-swap argument, not a don't-standardise argument.

### 13e. `Derive{Functor,Foldable,Traversable}` — the prior art says coexist

The Haskell experience is the empirical answer: `DeriveFunctor`,
`DeriveFoldable`, and `DeriveTraversable` exist **and** hand-written
instances remain routine. Deriving did not eliminate authoring. The
constraints are real — contravariant occurrences of the parameter are not
derivable at all — but the deeper limit is that **derivation produces the
canonical structural instance, and one per type.**

Two counterexamples come from this campaign's own evidence, which is what
makes them worth using:

- **The lanewise/SIMD applicative is zip-shaped, not cartesian.** A
  structural derivation of Applicative over a sequence gives the
  cartesian product, not the lanewise zip. Haskell needs a `ZipList`
  newtype precisely because a type gets one instance. So derivation
  produces the *wrong* instance for one of P3200's three motivating
  domains.
- **Traversal order is contractual per instance** — in-order for binary
  trees. A structural derivation yields field order, not the contracted
  in-order. Derivation produces the wrong traversal for the campaign's
  exemplar tree.

**And the design already has the escape hatch**, which is worth saying
out loud: the `*_with` spellings pass the instance inline rather than
looking it up, so a non-canonical instance can be supplied without
newtype-wrapping the type. That is a genuine advantage over the Haskell
arrangement, and it makes deriving *complementary* rather than competing:
derive the easy cases, pass explicitly for the rest. The position is
constructive rather than defensive, which is a better place to argue
from.

### 13f. Tracked claim — runtime cost parity

**Claim:** there is no runtime cost difference between a
CRTP + deducing-this derived instance and a flat aggregate of named
operations (the reflection-emitted shape). **Status: unverified.**

Suggested check: a godbolt comparison in the transpose repo of a derived
instance against a hand-written flat aggregate standing in for the
reflection output, at `-O2` and at `-O0`, comparing codegen.

Expected honest caveat: parity at `-O2` is likely; **unoptimized builds
may show extra call frames through the CRTP base**. If so, state it —
"identical optimized codegen; the derivation chain is visible in debug
builds" is a defensible claim, and volunteering it costs nothing while
an unqualified "no runtime cost" invites a correction.

**Measure depth, not just presence.** Whether this is a footnote or a
finding depends on how many frames a single derived operation forwards
through. One or two is the same order as a ranges adaptor. Four or five
compounds per element and is worth knowing before someone else measures
it.

### 13f-2. Pre-conceding debug cost — ruling (Steve, 2026-07-20)

**Ruling: pre-concede the cost of "fast" debug builds.** The `-O0`
penalty is not peculiar to this machinery; it is endemic to the whole
abstraction-heavy direction the library has already taken. Ranges
adaptors and `std::execution` impose the same hidden costs, and MSVC's
debug iterators go further into ABI trouble.

**Stratify rather than lump — the precision is the argument.** The three
are not the same kind of problem:

1. **Extra call frames at `-O0`** (ranges, `std::execution`) — a
   performance problem in debug builds. Annoying, not incorrect.
2. **ABI incompatibility** (MSVC `_ITERATOR_DEBUG_LEVEL`) — TUs built
   with different settings do not link or, worse, link and misbehave.
   A correctness and linkage problem, categorically more severe.
3. **This machinery** — category 1, and at the shallow end of it.

Lumping all three invites "so you're as bad as debug iterators," when the
honest answer is that this is the mild category and the same category as
facilities already shipped and accepted. Say which category it is.

**The precedent worth citing** (verify before use): vendors have treated
the `-O0` cost of fundamental library facilities as a *compiler* problem
rather than a reason to reject the facility — Clang builtins for
`std::move` / `std::forward` / `std::addressof` exist precisely because
the call overhead at `-O0` was unacceptable for something that ubiquitous.
The response to abstraction penalty has been to fix the toolchain, not to
stop abstracting.

**And fix the comparison class.** The debug cost of this machinery is not
against zero; it is against the hand-written instance it replaces, which
has its own frames plus the correctness risk the machinery removes.
Framing it as machinery-vs-nothing concedes a comparison that was never
the right one.

### 13g. "Only one way to do it" — right, but hold it in reserve

The observation is correct and the ownership example is good: `unique_ptr`,
`shared_ptr`, `optional`, `indirect`, `polymorphic`, plus `vector` and
`reference_wrapper`, all overlap on managing a value, all justified by
different trade-offs, and nobody argues `indirect` is redundant because
`shared_ptr` exists.

But it is an *answer to a bad objection*, not a positive case. Leading
with it signals that the objection is expected and puts the argument on
the back foot. Lead with §13e — derivation cannot produce the instances
this campaign actually needs — and keep the only-one-way rebuttal for if
someone raises it.

There is also a principled version of the objection worth answering
directly rather than dismissing: *don't add a second way that is strictly
worse or merely different.* The answer is that deriving and hand-
authoring are not two ways to do one thing — one is a fast path for the
derivable subset, the other the general mechanism for everything else,
and Haskell demonstrates they coexist because they must.

[P2996]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r10.html
[P3294]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3294r0.html

---

## 14. The instance contract — ruling (Steve, 2026-07-20)

**Ruling.** It does not matter to the consumer of a typeclass instance
whether an operation is a named callable data member (a member CPO), a
member function, or a member template function. **And that
non-specification is itself an explicit part of the contract.** An
algorithm writer may assume almost nothing beyond being able to spell
`functor.fmap(g, thing)`; that instances are zero-sized or very small,
and freely copyable; and that there is no contract to treat them as
stateful, even though statefulness would be convenient for profiling.
**A typeclass is not a resource.**

This resolves the §13c open question, and resolves it better than either
branch offered there.

### 14a. Consequence: drop the C++26/C++29 timing argument

If the shape is deliberately unspecified, the reflection objection does
not merely shrink (§13b) — it dissolves. The standard cannot be locked
into a derivation strategy it never specified, so implementations and
future revisions switch freely and no consumer can observe the
difference.

**So do not use the P3294-is-C++29 timing argument.** It was always
slightly cheap, and it ages badly — it becomes false in three years. The
contract argument never ages. Lead with the contract.

### 14b. State the constraint precisely: call position only

"Only being able to spell `functor.fmap(g, thing)`" is best stated as
**expression validity in call position, and nothing else.** That is what
makes the shapes interchangeable, and it is worth spelling out what it
forbids, because algorithm writers will want these:

- `&functor.fmap` — no. Ill-formed for a member function template,
  ambiguous for an overload set, fine for a data member. Must be out.
- `std::invoke(functor.fmap, …)`, or passing the operation to another
  higher-order algorithm — **no, and this is the one that will hurt.**
  It works for a callable data member and breaks for a member function.
  The workaround is a lambda closing over the instance; say so, because
  otherwise someone writes it, it compiles against one shape, and breaks
  on the other.
- `decltype(functor.fmap)` — no.
- `requires { f.fmap(g, x); }` — **yes, and this must work**, since it is
  how the concept is written.

### 14c. Enforcement: the concept can only check expressions

The contract is not merely documentation — it is enforced by what the
constraining concept is allowed to express. If the concept checks call
expressions only, and never nested types or member pointers, then no
algorithm can accidentally acquire a dependence on the shape. Write the
concepts that way deliberately, and say that is why.

### 14d. The evidence play — demonstrate it, do not promise it

Provide the same typeclass in **both shapes** — one instance with member
functions, one with callable data members — and run the entire algorithm
suite against both. If both pass, the non-specification is demonstrated
rather than asserted, and the reflection objection is pre-answered by a
running test instead of an argument. Cheap to build, and it is exactly
the kind of implementation experience the paper wants.

### 14e. "Zero-sized or very small" — the parenthetical earns its keep

The two shapes are **not** equivalent on size, and this is why the
qualifier is needed:

- A CRTP type with member functions is genuinely empty.
- A flat aggregate of N callable data members is **not** empty — distinct
  members need distinct addresses, so it is at least N bytes.
- Unless the generator applies `[[no_unique_address]]` to each member, in
  which case it can be empty again.

So parity is achievable but only deliberately. Worth knowing before
someone measures it and reports that the reflection-emitted form is
bigger.

For normative wording: specify the *semantic* properties (copyable,
copies interchangeable, no observable state) and leave size to a
recommended-practice note. "Very very small" is not specifiable;
"trivially copyable, and implementations are expected to make instances
empty" is.

### 14f. "Not a resource" — the signature consequence

Pin it: **instances are taken by value.** Never by reference, never by
forwarding reference, never moved. No ownership, no lifetime coupling to
the algorithm, no RAII, freely created and destroyed at any point, copies
interchangeable. That follows directly from "not a resource" and it is a
signature-level decision that should be stated once and applied
everywhere.

### 14g. Profiling is achievable inside the contract

The aside about statefulness being convenient for profiling has a clean
resolution that does not require weakening anything. Since algorithms may
copy instances arbitrarily, instance-held counters are per-copy and
useless. But an instance holding a **non-owning handle** to external
state — a pointer to a counter — is still small, still freely copyable,
and still not a resource. So profiling works through indirection rather
than through instance state, entirely within the contract as ruled.

Worth one sentence in the paper, because "no state" reads as "profiling
impossible" and that would be a false deterrent.

### 14h. Synopsis hazard

The paper's synopsis has to show *some* shape, and whichever it shows
will be read as normative. It needs an explicit note that the shape shown
is exposition-only and the actual requirement is call-expression
validity. Otherwise the wording quietly re-specifies what §14 deliberately
left open.

---

## Open questions for Steve

- Runtime-parity claim (§13f) — needs the godbolt check before it goes in
  public text.
- Is the authoring side actually separable from the vocabulary in the
  P3200 wording, as §12g assumes tactically?
- Full policy post as a separate piece, or compact passage only (§12h)?
- P2727's current status, for the twenty-eight-year claim (§12c).
