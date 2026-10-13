# Organizing the typeclass case: teaching, background, and the original research

Written by the lead from direct reading of: functor.hpp, detail/typeclass_base.hpp, sequence.hpp, monoid.hpp
(monoid-selection), operation-objects-and-the-lookup-modes.org, typeclass-object-pattern.md,
prior-art-typeclasses-in-cpp, where-are-the-monads.org, essay/SPEC.md + the current essay draft, lean-model-sync.

## 1. What is actually being sold (one sentence, then the three audiences)

"A typeclass instance is a value: a stateless object holding a type's implementations of a named family of
operations, found by a per-concept variable template specialized on the type, and passed around, pinned, or
defaulted like any other value."

Everything in the repo that matters for the sale follows from "is a value": the four lookup modes, `_with`,
NTTP pinning, instance objects for monoids, inheriting from an instance to bring names into scope, stateful
instrumentation instances. The thing *not* following from it — and the thing a language would have given you —
is who decides which value. That is where the research lives.

Three audiences, three documents (not one):
- **LEWG/LEWGI**: why a *bundled* mechanism and not one CPO per operation (D3200R0 already carries this);
  why library-level is enough for C++29; what P2279's "we need a language mechanism" would change later.
- **Library implementers**: the CRTP derivation bases, basis coherence, constexpr derived ops, wording
  generation — the "how to write an instance" tier (typeclass-object-pattern.md is the seed; it is stale).
- **Working programmers**: the four lookup modes and the three algebras, taught by example. The blog
  series (5 posts) is this already; it needs the "why instances are values" post it never had.

## 2. Background literature, organized by the claim it supports

The repo cites 12 works across two .bib files (duplicated). The case needs roughly four tiers. Entries marked
(verify) are from memory and need checking before citation.

**Tier A — typeclasses as a language feature, and dictionary passing** (supports: "this is GHC's encoding")
- Wadler & Blott 1989 (cited). Hall, Hammond, Peyton Jones, Wadler 1996 "Type classes in Haskell" (TOPLAS) —
  the formal dictionary-passing translation. Peyton Jones, Jones, Meijer 1997 "Type classes: an exploration of
  the design space" — the overlap/coherence design axes, still the best framing.
- Oliveira, Moors, Odersky 2010 "Type Classes as Objects and Implicits" (OOPSLA) — the closest published
  relative of this design: dictionaries as first-class values, resolution as implicit search. Should be the
  anchor citation for "instance is a value".
- Kiselyov & Shan / Gonzalez "Scrap your type classes" (explicit records of functions) — the honest
  alternative, i.e. "just pass the object" (= your mode 2).
- Lämmel & Ostermann 2006 "Software extension and integration with type classes" (GPCE) — typeclasses vs
  the expression problem; relevant to algebras + `overloaded`.

**Tier B — resolution/coherence across languages** (supports the original research)
- GHC User's Guide, overlapping instances (OVERLAPPING/OVERLAPPABLE/INCOHERENT); Morris & Jones 2010
  "Instance chains"; Sulzmann et al. 2007 "Understanding functional dependencies via CHRs".
- Lean 4: Selsam, Ullrich, de Moura 2020 "Tabled Typeclass Resolution" (arXiv) — the algorithm; de Moura &
  Ullrich 2021 "The Lean 4 Theorem Prover and Programming Language" (CADE); Lean reference on instance
  priorities, `outParam`, default instances, local/scoped instances.
- Rust: RFC 1023 (orphan rules), the coherence/"overlap check" chapters of the reference; Chalk design docs.
- Scala: Odersky et al. 2017 "Simplicitly: foundations and applications of implicit function types" (POPL 2018).
- Coq/Agda: Mahboubi & Tassi 2013 "Canonical structures for the working Coq user"; Devriese & Piessens 2011
  "On the bright side of type classes: instance arguments in Agda" (ICFP).
- Swift: protocol witness tables (WWDC / "Understanding Swift Performance" — informal, cite sparingly).

**Tier C — C++'s own history with the problem** (supports: "the standard lost this once")
- Gregor, Järvi, Siek, Stroustrup, Dos Reis, Lumsdaine 2006 "Concepts: linguistic support for generic
  programming in C++" (OOPSLA) — concept maps and their resolution rules. N2081 / N2914 draft text.
  Stroustrup 2009 (cited). Dos Reis & Stroustrup 2006 "Specifying C++ concepts" (POPL).
- **Bernardy, Jansson, Zalewski, Schupp, Priesnitz 2008 "A Comparison of C++ Concepts and Haskell Type
  Classes" (WGP)** — the direct ancestor of the planned comparison; the new paper is its 2026 sequel with
  concepts-lite + variable templates on one side and Lean 4 on the other.
- Garcia et al. 2007 "An extended comparative study of language support for generic programming" (JFP) —
  the eight-language table; borrow its criteria list for the comparison grid.
- Siek & Lumsdaine 2011 "A language for generic programming in the large" (G, SCP).
- Customization-point lineage: N4381 (Niebler), P1895 tag_invoke, **P2279 Revzin "We need a language
  mechanism for customization points"**, P2547 Baker et al., P0847 deducing this (the mechanism the bases rely
  on). The repo's prior-art post covers libraries (FC++, cat, libfn, Flux) but not this paper trail.

**Tier D — the algebras** (already well covered in the repo)
- McBride & Paterson 2008; Gibbons & Oliveira 2009; Jaskelioff & Rypacek 2012 "An investigation of the laws
  of traversals" (laws.hpp's provenance); Bird et al. 2013 "Understanding idiomatic traversals backwards and
  forwards"; Meijer/Fokkinga/Paterson 1991; Hinze & Paterson 2006; Katsumata 2014.
- Pattern matching: P1371, P2688 (match), P2392 (Sutter, is/as) — current revision numbers and EWG status
  must be re-verified at publication (tree_algorithms punch-list already flags this).

Housekeeping: one `references.bib` at `papers/references.bib`, both papers and the blog pointing at it; the
tree_algorithms copy duplicates four entries of the blog copy today.

## 3. The original research: "who resolves the instance, and by what rules"

### 3.1 Framing
The library fixes *what* an instance is (a value) and *where* it is looked up (`functor<T>`). The language
fixes *how a specialization is chosen*. In C++ that is: partial-specialization matching of a variable
template, partial ordering when several match ([temp.class.order] applied through [temp.var]... verify
exact wording), constraint subsumption as the tiebreaker among equally specialized candidates
([temp.constr.order]), and ambiguity as a hard error. Coherence is enforced by the ODR — and across TUs it is
ill-formed-no-diagnostic-required, not an error. That last clause is the C++ answer to Haskell's orphan
instances and Rust's orphan rule, and it is the weakest answer of the three.

### 3.2 The comparison grid (rows = questions, columns = C++ var-template / GHC / Lean 4; Rust as a 4th
column if space)
1. Unit of registration: specialization of a library-namespace variable template / instance decl anywhere /
   instance decl anywhere, scoped or local.
2. Selection among candidates: most-specialized by partial ordering, then more-constrained / most specific
   head, else error; pragmas to permit overlap; **contexts never consulted** / priority, then declaration
   order, with backtracking (tabled search); contexts consulted.
3. Multiple instances for one type: impossible (one specialization per key); hence instance objects for
   monoids / newtype wrappers (Sum, Product) — the same pressure the repo felt and resolved differently /
   permitted; priorities and local instances pick.
4. Generic + specific instance ("overlapping"): C++ partial ordering handles `functor<R> requires range<R>` vs
   `functor<vector<T>>` — *but nothing in this codebase exercises it*; the cases where partial ordering is
   undecidable (two constrained specializations, neither subsuming) are the interesting ones / OVERLAPS
   pragma / priorities.
5. Coherence guarantee: ODR, IFNDR across TUs / global by default (orphan warning) / none sought —
   different instances can be found in different scopes by design.
6. Higher-kinded keys: `functor<vector<T>>` keys the *applied* type; `Fix<F>` needs the unapplied template
   (tree_algorithms' two-key cost) / kind `* -> *` native / `Type → Type` native.
7. Output parameters / associated types: nested `value_type`/`element_type` conventions (the
   `applicative_value_t` cost both consumers hit) / associated type families, fundeps / `outParam`.
8. Where lookup is written: library namespace only (specialization must be in `beman::transpose`) / anywhere.
9. Explicit instance passing: mode 2 and `_with` / `TypeApplications` does not pass dictionaries; reflection
   or newtype / `(inst := …)` named-argument syntax, `@` explicit.
10. Diagnostics on missing instance: constraint failure naming the operation (operation-object-wording) /
   "No instance for…" / "failed to synthesize instance".

### 3.3 Experiments the paper must run (none exist in the repo today)
- E1: constrained generic instance (`template <std::ranges::input_range R> … traversable<R>`) coexisting with
  the vector instance; verify partial ordering picks vector on GCC/Clang/MSVC; record the diagnostic when a
  user adds a second constrained instance that neither subsumes nor is subsumed.
- E2: cross-TU incoherence: two TUs, one with a user specialization for `std::optional<Widget>`, one
  without; show the silent IFNDR; then show whether a module exporting the specialization closes the hole.
- E3: the "many monoids on int" case written three ways: C++ (instance object as parameter, 2026-10-07
  design), Haskell (newtypes Sum/Product), Lean 4 (priorities + local instances); compare call-site noise and
  what each language *prevents*.
- E4: HKT: `fmap` over `Fix<F>` in all three; what C++ pays for keying on the applied type.
- E5: compile-time cost of lookup vs CPO/tag_invoke on the existing fanout benchmark
  (docs/transpose/metrics/fanout-runs.jsonl is the seed).
The Lean side already has a sibling repo (steve-downey/lean-graded, lean-model-sync); the graded-monad model
is a natural E3/E4 host rather than starting a new Lean project.

### 3.4 Why this is library-level, and what the language paper would be
One section, not a proposal: what P2279-style language customization would subsume (modes 1 and 4), what it
would not (modes 2 and 3 — instances as values is the part a language mechanism tends to *remove*), and why
that argues for standardizing the library shape first. This is the strongest "why not wait for the
language" answer available and it is not written anywhere yet.

## 4. Typeclasses, algebras, and pattern matching

The structural observation: a typeclass instance is chosen by **type-level** pattern matching (partial
specialization *is* matching on type structure); an algebra passed to `fold_fix` does **value-level**
matching on one layer. The two are orthogonal and compose — that sentence is the thesis of the section.

Concretely (from tree_algorithms' reviewer, verified against the headers):
- Variant-based layers (`ExprF`, `NatF`, `FringeTreeF`) transliterate to `match` directly; the algebra becomes
  `layer match { <Const> [v] => …; <Add> [l, r] => … }`.
- Struct-based layers (`BinaryTreeF`, `RoseF`) gain little beyond structured bindings; the win is in sum
  types, so the paper should show one of each.
- `overloaded` (best-match overload resolution, order-independent) and `match` (first-match, ordered,
  exhaustiveness-checked) compete for the same job. State the trade: `overloaded` works today and composes
  (Swierstra 2008 "Data types à la carte" style), `match` gives exhaustiveness and gets shorter. The deferred
  "fallback overload set" question dissolves under `match` — say so.
- `Box`/`child_slot` as *patterns*: whatever indirection/dereference pattern protocol P2688 lands with
  decides whether `Fix` nodes destructure cleanly. This is design pressure on child_slot's "reads the same"
  contract; track it.
- Typeclass + match: the algebra for `fold_map` over a sum layer is `match` + the monoid instance's
  `combine`; the Foldable instance for a user type is then three lines. That is the hallway exhibit.

Status hedge: every pattern-matching claim must be checked against the paper trail at publication; the notes
already carry "as of this writing" wording and the punch-list carries the item.

## 5. Where this lands in the repo
- A new explainer `docs/typeclass/…` (or `docs/coordination/typeclass-essay.org`) — the "instances are
  values" piece that the three papers cite instead of each re-explaining the mechanism. This replaces the
  stalled six-exhibit SPEC or absorbs it: the exhibits become the examples in §1's working-programmer doc.
- The comparison paper is a *fourth* document, research not proposal; it cites D3200R0 for the mechanism and
  is cited by it for "why these rules". File it with the papers, named by name
  (`papers/instance-resolution-in-three-languages.md` or similar), per the name-not-number rule.
- The pattern-matching section belongs in Paper D (it already has the hedged paragraph) with a cross-ref from
  the essay; it does not need its own paper.
