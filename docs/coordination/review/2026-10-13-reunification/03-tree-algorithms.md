# Review 03 — tree_algorithms (Paper D, D4322) and its seam with transpose

Repository: `/home/user/typeclass` at `71129ec` (2026-10-12 merge). All paths below are relative to that root unless absolute. "Verified" means I read the lines cited or compiled a probe with `clang++-18 -std=c++23 -Isrc`; "inferred" is marked as such.

## Executive summary

1. The two-tier split is **clean in code and now checked**: the five explicit-tier headers pull in zero `beman/transpose/` headers (verified with `clang++-18 -MM`; the CMake `#error` check at `src/beman/tree_algorithms/CMakeLists.txt:105-153` enforces it). The lookup tier includes exactly `transpose/functor.hpp` plus `detail/typeclass_base.hpp`.
2. The split is **less clean in the paper**: Paper D's "After" example in the primary tier uses `overloaded` (now P3200's facility) unattributed; the paper still calls the lookup objects "consumer-stub" and the lookup tier *defines* two typeclass lookup variables of its own (`layer_fold`, `project`) while saying it "never respecifies" the mechanism.
3. **`fold_map` exists twice with incompatible call shapes**: `beman::transpose::fold_map(f, structure, monoid_instance)` (operation object, monoid as an object with `identity()`/`combine()`) and `beman::tree_algorithms::fold_map<Result>(map_fn, combine, identity, …, tree)` (function template, monoid as two positional values, structure last). Semantics agree; signature, argument order, and monoid passing do not. Now that both live in one repo under `beman::`, this is a design smell worth fixing, not just documenting.
4. **transpose's `value_type`/`element_type` conventions do not fit base-functor layers**: `functor_object<ExprFFunctorMap<int>, ExprF<int>>` is `false` (verified) because `applicative_value_t` needs a nested `value_type`; tree_algorithms works around it by never using transpose's deep concepts and relying only on `has_instance_v`. The CRTP base buys the layers exactly one operation (`replace`), used once in a test.
5. The merge exposed concrete friction: five headers now hop namespaces to specialize `beman::transpose::functor` (avoidable: a qualified specialization from global scope compiles on clang-18 — verified), two `static_assert` messages still name the pre-rename `layer_fold_typeclass`/`project_typeclass`, and the module unit must include transpose headers in the global module fragment because transpose is not a module.
6. **Synopsis: no signature drift** — all 13 declarations in `papers/algorithms-for-trees.md:232-323` match the headers. But the synopsis omits the two lookup variables it claims to mirror, its HTML ledger has stale paths/dates, and the paper's prose predates the 2026-07-18 `child_slot` amendment (it still says every recursive position is a `Box<A>`, and its Costs section still calls performance a non-goal while DECISIONS.md records measured zero-overhead and says the paper should state it).
7. Pattern matching (P2688) meshes well with the variant-based layers (`ExprF`, `NatF`, `FringeTreeF`) — each algebra and each `fmap` body is a one-layer case analysis — but gives little to the struct-based layers (`BinaryTreeF`, `RoseF`, `NodeF`), whose only "case" is the engaged/absent child slot; `Box` would need to satisfy whatever optional-like protocol the match paper settles on. `overloaded` (best-match) and `match` (first-match) compete for the same job and the papers should say which is the long-run spelling.

---

## 1. The two-tier design

### Finding 1.1 — Explicit tier is mechanism-free in code (verified); the check is real
**Severity:** note (positive).
**Evidence:**
- `include/beman/tree_algorithms/fix.hpp:14-16` includes only `<utility>`; `box.hpp:14-16` only `<utility>`; `child_slot.hpp:14-21` `box.hpp`, `fix.hpp`, std; `recursion_schemes.hpp:14-18` `fix.hpp`, `<utility>`; `fold_map.hpp:14-15` `fix.hpp`, `recursion_schemes.hpp`.
- `clang++-18 -std=c++23 -Isrc -MM` on each of the five headers: 0 lines matching `beman/transpose`. On `recursion_schemes_lookup.hpp` the closure adds exactly `transpose/functor.hpp` and `transpose/detail/typeclass_base.hpp`.
- `src/beman/tree_algorithms/CMakeLists.txt:105-153` generates one TU per explicit-tier header that `#error`s if any `BEMAN_TRANSPOSE_*` include guard is defined (commit `05ea8b4`).
**Why it matters:** This is the paper's independence claim ("remains fully valid even if P3200 does not progress", `papers/algorithms-for-trees.md:193`) made checkable.
**Recommendation:** None for the headers. See 1.2 for what the check does not cover.

### Finding 1.2 — The explicit tier's *tests* and Paper D's *primary-tier examples* depend on P3200's `overloaded`
**Severity:** significant (independence claim in the paper).
**Evidence:**
- `tests/beman/tree_algorithms/recursion_schemes.test.cpp:7` and `fold_map.test.cpp:11` include `<beman/transpose/overloaded.hpp>`; every algebra in those tests is `std::visit(overloaded{…})`.
- `papers/algorithms-for-trees.md:44,67`: both the Before and After cells use `overloaded{…}`; `:78` passes `expr_fmap` — this *is* the explicit tier, yet the example's vocabulary is now proposed by P3200 (`docs/transpose/decisions.md` `## overloaded-home`, 3415-3443). The paper never says where `overloaded` comes from. The dedup ledger lists this as undone: `docs/coordination/machinery-deduplication.md:82` ("say so where the examples first use it") and `:138` ("Neither is written").
- `examples/tree_algorithms/fixpoint_tree_example.cpp:63-68` shows the plain `struct overloaded : Ts... { using Ts::operator()...; }` works for the same algebra without any library.
**Why it matters:** A reader checking "what remains valid if P3200 stalls" (`:534-535`) finds the primary example leaning on a P3200 facility. The claim is still true (any overload set works), but the text does not say so.
**Recommendation:** One sentence at first use: "`overloaded` is the overload-set helper P3200 proposes; any `struct : Fs... { using Fs::operator()...; }` serves the explicit tier." Optionally give the explicit-tier tests a test-local overload set so the test build matches the header check.

### Finding 1.3 — "Consumes, never respecifies" is true of `functor` and not quite true of `layer_fold`/`project`
**Severity:** minor (paper accuracy).
**Evidence:** `include/beman/tree_algorithms/fold_map_lookup.hpp:46-55` defines two new lookup variables `layer_fold<Layer>` and `project<Tree>` (`std::false_type` default, the P3200 pattern). `papers/algorithms-for-trees.md:188` says "following the same pattern"; `:191-192` says the headers "are explicitly *consumers* … never respecifies it"; `:459` calls them "the three consumer-stub lookup objects" — stale on two counts: none is a stub since `77180c5`, and `functor` is transpose's own, not Paper D's.
**Why it matters:** Paper D does propose two typeclass-object lookup variables of its own. That is fine, but it means Paper D's lookup tier carries mechanism-shaped surface that LEWG will see as *new* customization points, and the paper should own that rather than file it under "consume".
**Recommendation:** Rewrite `:459` ("the lookup tier registers layer functors with P3200's `functor<Layer>` and adds two lookup objects of its own, `layer_fold<Layer>` and `project<Tree>`") and add the two declarations to the synopsis (see 5.2).

### What a user writes in each tier (verified from tests/headers)

**Explicit tier — one lambda for `fmap`, passed at every call:**
```cpp
// tests/beman/tree_algorithms/recursion_schemes.test.cpp:139-153
template <typename A, typename F>
constexpr auto fmap_tree(F&& f, const TreeF<A>& tree) {
    using B = std::remove_cvref_t<std::invoke_result_t<F, const A&>>;
    return std::visit(overloaded{
        [](const Leaf& l) -> TreeF<B> { return Leaf{l.value}; },
        [&f](const Node<A>& n) -> TreeF<B> {
            return Node<B>{make_box<B>(std::invoke(f, *n.left)), make_box<B>(std::invoke(f, *n.right))}; },
    }, tree);
}
inline constexpr auto fmap_tree_fn = [](auto&& f, const auto& t) { return fmap_tree(std::forward<decltype(f)>(f), t); };
// :206
fold_fix<int>(subtract_algebra, fmap_tree_fn, tree);
// fold_map, six arguments — tests/beman/tree_algorithms/fold_map.test.cpp:120
fold_map<std::string>(show_digit, concat, std::string{}, list_layer_fold_map, fmap_list_fn, list);
```

**Lookup tier — register once (three declarations, with a namespace hop), then two-argument calls:**
```cpp
// include/beman/tree_algorithms/expression.hpp:83-115  (33 lines for one fmap)
template <typename A> struct ExprFFunctorImpl { template <typename Fn> constexpr auto fmap(this auto&&, Fn&& fn, const ExprF<A>& layer) { … } };
template <typename A> struct ExprFFunctorMap : transpose::derive_functor<ExprFFunctorImpl<A>> { using ExprFFunctorImpl<A>::fmap; };
} // namespace beman::tree_algorithms
namespace beman::transpose {
template <typename A> inline constexpr auto functor<beman::tree_algorithms::ExprF<A>> = beman::tree_algorithms::ExprFFunctorMap<A>{};
} // namespace beman::transpose
namespace beman::tree_algorithms {
// :200
constexpr auto eval(const Expr& expr) -> int { return fold_fix<int>(eval_algebra, expr); }
// fold_map over Fix — tests/beman/tree_algorithms/fold_map.test.cpp:217 (after registering layer_fold<ExprF<A>>, expression.hpp:175-194)
fold_map<std::string>(show_digit, concat, std::string{}, tree);
// fold_map over a tree in its own representation — fold_map.test.cpp:279
// requires project<BinaryTree<T>> and layer_fold<BinaryTreeF<T,A>> (binary_tree.hpp:225-268)
fold_map<std::string>(show_s, concat_s, std::string{}, bt);
```
The per-call saving (two arguments instead of three or six) is paid for by a per-type registration of 33 lines for `fmap` plus ~20 for `layer_fold` plus ~15 for `project`. transpose's own decision log already records a consumer (specgen) that stayed on the explicit tier at ~4 functor-shaped types and found it "sufficient, and reportedly not by a narrow margin" (`docs/transpose/decisions.md` `## explicit-parameter-tier-sufficiency`). tree_algorithms ships 5 representations and registers all of them — it is right at the scale where the trade is a wash, and the paper does not say so.

---

## 2. The fold family: two `fold_map`s

### Finding 2.1 — Two definitions of `fold_map`, same semantics, incompatible shape
**Severity:** significant (coherence of one codebase, one `beman::` surface).
**Evidence (verified):**

| | `beman::transpose` (`include/beman/transpose/fold.hpp`) | `beman::tree_algorithms` (`fold_map.hpp`, `fold_map_lookup.hpp`) |
|---|---|---|
| Kind | member of `derive_foldable` (`:209-211`) + operation object `fold_map_fn` (`:683-711`) | free function template (`fold_map.hpp:49-60, 72-85`; `fold_map_lookup.hpp:77-79, 91-93`) |
| Call | `fold_map(f, structure, monoid)`; 2-arg form defaults to `Monoid<result>` | `fold_map<Result>(map_fn, combine, identity, tree)`; explicit forms add `layer_fold, fmap_fn[, project]` before `tree` |
| Structure position | second | last |
| Monoid | one instance object, `m.identity()` / `m.combine(a,b)`, trailing (`docs/transpose/decisions.md` `## monoid-selection`) | two positional values `combine, identity` (in that order); paper: "no Monoid trait anywhere in the primary API" (`papers/algorithms-for-trees.md:172`) |
| Result type | deduced from `f` via `Impl::element_type` (`fold.hpp:77-79`) | explicit `<Result>` by convention; in fact deducible from `identity` (probe compiled: `fold_map(show, cat, std::string{}, t)` with no `<std::string>`) |
| Lookup key | `foldable<Container>` (`fold.hpp:651-652`) | `layer_fold<Layer>` + `project<Tree>` (`fold_map_lookup.hpp:46-55`) |
| Derived family | `length, fold_left, fold_right, combine_all, fold, any_of, all_of, empty, to_vector, find_first` (`fold.hpp:283-639`) | none; each derived query is written by hand as a `fold_map` (`fold_map.test.cpp:254-268`; `examples/tree_algorithms/sequence_algorithms.cpp:103-131` re-derives `fold_left`, `to_vector`, `reversed`) |

Semantics agree: both map each element then combine under an associative operation with identity, in instance-defined traversal order (`fold.hpp:143-155`; `fold_map.hpp:19-35`; `binary_tree.hpp:219-235`). The derivation direction is consistent (`fold.hpp:245-247` derives `fold_map` from `fold_right` as `combine(f(x), acc)`; tree layer folds combine children in contractual order).

**Why it matters:** Owner's framing: one codebase, coherent API docs. Today `fold_map` names an operation object in one namespace and a function template in the other, with the structure in different positions and the monoid passed two different ways. A user with `using beman::transpose::fold_map; using beman::tree_algorithms::fold_map;` gets a conflicting-declaration error (variable vs. function template with one name); a user reading both papers meets two "the `fold_map` primitive"s. The coherence skill makes `identity()`/`combine()` the one monoid spelling "used by fingertree and anything new" (`.claude/skills/tree-proposals-coherence/SKILL.md:99-102`); tree_algorithms' primary spelling is a second monoid vocabulary (two bare callables) in the same repo.

**Recommendation:** The explicit tier can take a duck-typed monoid object — `const Monoid& m` used as `m.identity()`, `m.combine(a, b)` — with **zero** transpose dependency (no include, no concept), which keeps `explicit-tier-isolation` intact and lets `transpose::sum_monoid<int>{}`, a user's `struct { identity(); combine(); }`, or specgen's `monoid{combine, identity}` drop in unchanged. Then `fold_map<Result>(map_fn, monoid, tree)` and `transpose::fold_map(map_fn, tree, monoid)` differ only in argument order — and that should be settled too (pick one; transpose's trailing-monoid order has a ruling, `## monoid-selection`). If the two positional callables are kept as a convenience, keep them as an overload, not the primary spelling.

### Finding 2.2 — "Foldable spans two repos with one proposal home" is now a smell to decide, not a hazard to manage
**Severity:** significant (design).
**Evidence:**
- The hazard text still speaks of "two repos" (`SKILL.md:165-168`); `papers/algorithms-for-trees.md:528` says the derived operations "live with the Foldable surfaces in the companion repositories — one definition each".
- In fact `fold_left` has two derivations in this repo: `include/beman/transpose/fold.hpp:312-355` and `examples/tree_algorithms/sequence_algorithms.cpp:103-114` (labelled example-local). `examples/tree_algorithms/expression_algorithms.cpp:50-111` builds an example-local `ExprFoldableMap`/`ExprTraversableMap` and its own `example_foldable_typeclass` lookup — a third pattern copy, noted but left alone in the ledger (`docs/coordination/machinery-deduplication.md:141`).
- The question is already filed and open: `docs/coordination/machinery-deduplication.md:202-208` `### layer-fold-and-foldable` — "Is `layer_fold<Layer>` a Foldable instance for the base functor under another name?"
- transpose's `Traversable` deliberately does not require Foldable (`include/beman/transpose/traverse.hpp:86, 391-398`), so registering Foldable instances for trees later creates no inversion.
**Why it matters:** The "evidence vs proposal" split made sense across repos. Inside one repo, `fold.hpp` (897 lines, ten derived ops, operation objects) sits beside a 96-line `fold_map.hpp` that proposes the primitive and re-derives the queries by hand in tests and examples. Neither consumes the other.
**Recommendation (decide, record under a slug):** Either (a) register `foldable<Fix<F>>` and `foldable<BinaryTree<T>>`/`foldable<FringeTree<T>>` instances in the lookup tier, implemented as `fold_map(f, t, m) = tree_algorithms::fold_map<monoid_value_t<M>>(f, m, t)` with `element_type` from the layer — so transpose's derived `length/any_of/to_vector/find_first` come free for every tree and the example-local Foldables disappear; or (b) state in both papers that tree_algorithms' `fold_map` is *not* Foldable's `fold_map` and give the mapping. (a) is small once 2.1 is done; it is the natural "layer_fold is the per-layer Foldable" answer to the open question.

### Finding 2.3 — `Result` "must be given explicitly" is inaccurate for `fold_map`
**Severity:** minor (doc accuracy; design choice worth stating).
**Evidence:** `fold_map.hpp:39`, `fold_map_lookup.hpp:74, 88`, paper synopsis comments. `Result` appears as `const Result& identity`, so it is deducible; a probe `fold_map(show, cat, std::string{}, t)` compiles on clang-18. (For `fold_fix` the claim is true: `Result` is only the return type.)
**Why it matters:** Deduction from `identity` has a hazard — `""` deduces `char[1]` — so writing `<Result>` is the right habit, but the reason given ("C++ cannot infer fold carriers through recursive calls", paper `:169`) does not apply to `fold_map`.
**Recommendation:** Reword to "should be given explicitly; deduction from `identity` is possible but a string literal deduces an array type."

---

## 3. Functor instances for layers; `Fix<F>`, `child_slot`, and the lookup

### How registration works (verified)
- Instances are keyed by the **applied** layer type `F<A>`, as a partial specialization of transpose's variable template over the free `A` (and `T`): `expression.hpp:113-115` (`functor<ExprF<A>>`), `binary_tree.hpp:205-207` (`functor<BinaryTreeF<T, A>>`), `fringe_tree.hpp:306-308`, `rose_tree.hpp:120-121`, `functors.hpp:94-95`, `benchmarks/tree_algorithms/fix_unique_tree.hpp:105-106`. Tests confirm the instance exists at both `F<int>` and `F<Fix<F>>` (`expression.test.cpp:106-107`).
- `Fix<F>` is keyed by the **unapplied** template `F` (`fix.hpp:31-34`, template template parameter). The two keys meet in `layer_fmap` (`recursion_schemes_lookup.hpp:42-49`), which deduces `Layer` from the value in hand and looks up `functor<remove_cvref_t<Layer>>`; `fold_fix` never needs a template-keyed lookup.
- Variant layers register an instance **on a `std::variant` specialization** in P3200's namespace (`functor<std::variant<Const<A>, Add<A>, Mul<A>>>`). Legal, and more specialized than any future generic `functor<std::variant<Ts...>>` transpose might add, so no ambiguity (inferred from partial-ordering rules; not probed).
- `child_slot` is orthogonal to lookup: `fmap` bodies call `make_slot<B>` directly (`expression.hpp:92-95`, `binary_tree.hpp:186-187`). At complete types `child_slot_t<A>` is `std::optional<A>`, which has its own `functor<std::optional<V>>` (`transpose/functor.hpp:313-315`); the layer fmaps correctly do **not** route through it, because at the knot the slot is `Box`, which has and needs no instance.

### Finding 3.1 — Key mismatch between `Fix` (template) and `functor` (type) costs alias binders and a deduction trait
**Severity:** minor (friction internal to tree_algorithms, but it is where the HKT emulation leaks).
**Evidence:** `binary_tree.hpp:156-166` (`BinaryTreeLayer<T>::template F`), `rose_tree.hpp:69-78`; `binary_tree.hpp:287-297` `binary_tree_element` trait because `T` "is a non-deduced context inside `BinaryTreeFix<T>` (it sits behind a nested alias template)"; call sites spell `wrap_fix<BinaryTreeLayer<int>::template F>(…)` (`examples/tree_algorithms/search_tree_on_fix.cpp:63`).
**Why it matters:** For the owner's planned research: Haskell's instance head is for the unapplied constructor (`instance Functor (BinaryTreeF t)`), Lean's likewise; C++ can only key the typeclass on the applied type, so the design carries two keys. Worth one paragraph in the paper's Costs section; today it is only a code comment.
**Recommendation:** Say it in the paper; consider whether `Fix` should also be keyed on a "layer binder" type (`Fix<BinaryTreeLayer<T>>` with `Layer::template F`) so both keys are types — a design question, not a defect.

### Finding 3.2 — Merge friction exposed in `77180c5` (verified in the diff and the current tree)
**Severity:** minor each; together they are the visible cost of consuming transpose's registration mechanism.
1. **Namespace hopping.** Every registration closes `beman::tree_algorithms`, opens `beman::transpose`, and reopens: `binary_tree.hpp:199-211`, `expression.hpp:109-119`, `fringe_tree.hpp:300-312`, `rose_tree.hpp:115-125`, `functors.hpp:89-99`, `benchmarks/tree_algorithms/fix_unique_tree.hpp:98-108`, and the three test files (`recursion_schemes_lookup.test.cpp:75-80, 145-150, 207-212`). Before the merge the variable lived in tree_algorithms' own namespace and no hop was needed. A qualified partial specialization from the enclosing (global) scope — `template <class A> inline constexpr auto beman::transpose::functor<mine::L<A>> = …;` — compiles on clang-18 (probe); **not verified on GCC** (only GCC 13 is installed here, below the GCC-14 floor). If GCC accepts it, the hop can go.
2. **Stale diagnostics after the rename.** `fold_map_lookup.hpp:66` says "specialize `beman::tree_algorithms::layer_fold_typeclass`"; `:96` says "`project_typeclass`". Both names were removed in the same commit. Also stale comments: `expression.hpp:33-34, 199` and `functors.hpp:33` ("functor_typeclass"), `recursion_schemes_lookup.test.cpp:30-32`.
3. **`layer_fmap` duplicates `transpose::fmap`.** `transpose::fmap(fn, layer)` works on layers (probe compiled); `layer_fmap` (`recursion_schemes_lookup.hpp:42-49`) exists to give the `static_assert` message. Fine, but the dedup plan suggested calling `transpose::fmap` (`machinery-deduplication.md:101`); note which was chosen and why.
4. **Module build.** `tree_algorithms.cppm:1-16` must include `transpose/functor.hpp` and `overloaded.hpp` in the global module fragment because transpose has no module unit; a cost that will recur for every transpose header the lookup tier adopts.
5. **The `Map` struct hides the base's `fmap`.** `struct ExprFFunctorMap : derive_functor<Impl> { using Impl::fmap; }` re-exposes `Impl::fmap` and thereby hides `derive_functor::fmap` (`transpose/functor.hpp:48-57`), so the base's only contribution to a layer instance is `replace` (`:62-88`) — used once, in `binary_tree.test.cpp:285`, and by no verb. The DEV-04 `constexpr` sweep on transpose's derived operations (`4e687ae`, recorded under `## derived-op-constant-evaluation`) therefore protected tree_algorithms' `static_assert`s only through `replace`. The idiom is transpose's own (`OptionalFunctorMap`, `functor.hpp:302-304`), so this is not tree_algorithms' fault; it is the cost of a CRTP base whose derived surface the consumer does not want.

### Finding 3.3 — DECISIONS.md body text contradicts its own amendment and uses numbered identifiers
**Severity:** minor (docs hygiene; owner's slug rule).
**Evidence:** `docs/tree_algorithms/DECISIONS.md:56` ("resolve `fmap` through `functor_typeclass<T>`") and `:133-134` list the retired names; the 2026-10-12 amendment (`:174-184`) renames them but the earlier text stands. Code comments cite numbered decisions: `recursion_schemes.hpp:28` and `recursion_schemes_lookup.hpp:24` ("Decision 4"), `binary_tree.hpp:51` ("Decision 6"), `fold_map_lookup.hpp:25`; tests cite "DEV-01/03/04" throughout (e.g. `fold_map.test.cpp:106`, `binary_tree.test.cpp:45-108`); `examples/tree_algorithms/expression_algorithms.cpp:7-8` cites "docs/DECISIONS.md, Decision 1" (path is now `docs/tree_algorithms/DECISIONS.md`, and the claim "functor-only lookup" is stale since the 2026-07-14 amendment).
**Recommendation:** Give DECISIONS.md slugs (e.g. `## explicit-fmap-primary`, `## lookup-tier-consumes-transpose`, `## child-slot-at-the-knot`, `## order-sensitive-test-algebras`, `## no-bare-ctad`, `## constexpr-derived-operations`) and re-point code comments and `SKILL.md:114-123` at them. DEV-0n names also live in the fixpoint repo's ledger; the slug can cite them once.

---

## 4. Costs of transpose's choices visible from the consumer side

### Finding 4.1 — `functor_object` cannot be satisfied by any layer: the nested-`value_type` requirement does not fit base functors
**Severity:** significant (this is the "design resistance" the owner asked for).
**Evidence:** `include/beman/transpose/detail/typeclass_base.hpp:82-100`: `applicative_value<T>` is defined only when `T::value_type` exists (plus `std::optional`). `functor_object<OBJ, CONTEXT>` (`transpose/functor.hpp:260-264`) and `functor_impl` (`:241-243`) both name `applicative_value_t<CONTEXT>`. Probe: `static_assert(functor_object<ExprFFunctorMap<int>, ExprF<int>>)` fails — "implicit instantiation of undefined template `applicative_value<std::variant<Const<int>, Add<int>, Mul<int>>>`". No layer type (`ExprF`, `NatF`, `FringeTreeF`, `BinaryTreeF`, `RoseF`) has a `value_type`, and none should: a layer's "element" for Functor purposes is the child position `A`, which is a template parameter, not a member typedef. tree_algorithms accordingly uses only `has_instance_v` (`recursion_schemes_lookup.hpp:44`; every `*.test.cpp`) and never transpose's concepts.
**Why it matters:** transpose's conformance vocabulary (`functor_object`, `functor_impl`) is unusable for the one consumer that registers the most Functor instances. The owner guessed that costs might be invisible from the consumer side; this one is.
**Recommendation:** Make `applicative_value` customizable by specialization (layers could then say `applicative_value<ExprF<A>> { using type = A; }`), or let the Functor concepts take the element type as an explicit parameter / probe `fmap` with a generic witness. Record the ruling under a slug in `docs/transpose/decisions.md` and add one layer to transpose's concept tests as the regression.

### Finding 4.2 — `element_type` (Foldable) is the second typedef the layers do not have, and the reason `layer_fold` is not a `foldable`
**Severity:** significant (same root as 2.2).
**Evidence:** `transpose/fold.hpp:71-79, 583-605`: the two-argument `fold_map`, `to_vector`, `find_first` exist only when `Impl::element_type` is declared. tree_algorithms' layer folds are templates over `Result` with the element type implicit in the layer (`binary_tree.hpp:226-230`: element is `T`; `expression.hpp:176-180`: element is `int`; `fringe_tree.hpp:368-372`: `T`). The element type *is* nameable per instance, so a `foldable<BinaryTreeFix<T>>` with `element_type = T` is feasible — it has not been attempted.
**Recommendation:** Fold into the decision in 2.2.

### Finding 4.3 — CRTP ceremony vs. one lambda
**Severity:** minor (cost statement for the paper).
**Evidence:** Registering `fmap` through transpose takes an `Impl` struct, a `Map` struct with a `using`, and a specialization in a foreign namespace (33 lines, `expression.hpp:83-115`); the explicit tier takes a 12-line lambda (`recursion_schemes.test.cpp:139-153`). The base's derived surface (`replace`) is unused by the library. The paper's Before/After (`:62-79`) shows the explicit tier — plausibly (inferred) because the lookup-tier registration would not fit in a `cmptable` cell.
**Recommendation:** The paper's Costs section should say the lookup tier's per-type cost in lines, and that a consumer at five representations pays about what it saves (see specgen data point, `docs/transpose/decisions.md` `## explicit-parameter-tier-sufficiency`).

### Finding 4.4 — Registration must happen in transpose's namespace (or qualified from an enclosing one)
**Severity:** note (research material).
**Evidence:** Finding 3.2 item 1; `transpose/detail/typeclass_base.hpp:24-29` ("lookup static and explicit", no ADL path).
**Why it matters:** For the planned comparison with Haskell overlapping instances and Lean 4 instance resolution: C++ variable-template specialization is *closed-world per namespace* (the instance must be declared where the primary lives, or qualified from an enclosing scope) and resolves by partial ordering (most specialized wins, ambiguity is an error), whereas Haskell instances are declared anywhere (orphans allowed, overlap resolved by `OVERLAPPING` pragmas) and Lean instances are declared anywhere with priority-ordered search. tree_algorithms is the test bed where this shows: every adapter header is a different "orphan instance" file.

### What did *not* hurt (verified)
- `has_instance_v` with the `std::false_type` default worked unchanged for tree_algorithms' own lookup variables (`fold_map_lookup.hpp:46-55, 64, 94`).
- Partial specialization over `A` (and `T`) is exactly what the layers need; no `rebind`-style trait was required anywhere in `src/beman/tree_algorithms/`.
- `constexpr` on derived operations was a cost transpose paid *for* the consumer (`4e687ae`), the one case in the merge where design resistance from tree_algorithms changed transpose.

---

## 5. Paper-vs-header drift

### Finding 5.1 — Synopsis signatures: no drift (verified declaration by declaration)
**Severity:** note (positive).
`papers/algorithms-for-trees.md:238-241` ≙ `recursion_schemes.hpp:41-42`; `:245-248` ≙ `:59-60`; `:252-255` ≙ `:75-82`; `:263-267` ≙ `:116-118`; `:269-272` ≙ `:140-142`; `:280-284` ≙ `fold_map.hpp:49-60`; `:286-292` ≙ `:72-85`; `:299-301` ≙ `recursion_schemes_lookup.hpp:58-59`; `:303-305` ≙ `:73-74`; `:307-310` ≙ `:88-89`; `:312-316` ≙ `fold_map_lookup.hpp:77-79`; `:318-320` ≙ `:91-93`. Modulo wrapping and trailing `;`, identical.

### Finding 5.2 — Synopsis omits what its own ledger says it mirrors; ledger comment is stale
**Severity:** minor (will bite on the next re-diff).
**Evidence:** `papers/algorithms-for-trees.md:222` lists anchor `94d2dd48` "lookup variables" as mirrored, but no `layer_fold`/`project` declaration appears in `:232-323`. The ledger paths are `include/beman/tree_algorithms/...` (`:207-221`; files are under `src/` since `bc30fa6`). `:225-230` "Checked against … 2026-07-14 … no header signature changes since" was not updated on 2026-10-12 even though `:294-297` prose was. `docs/tree_algorithms/notes/anchor-inventory.md` uses the same `include/...` paths throughout.
**Recommendation:** Add the two lookup-variable declarations (they are the registration surface a user must specialize), fix paths, add a 2026-10-12 line to the ledger.

### Finding 5.3 — Paper prose predates the 2026-07-18 `child_slot` amendment
**Severity:** significant (the paper misdescribes the shipped representation and omits its one measured result).
**Evidence:**
- `papers/algorithms-for-trees.md:145-147`: "Recursive positions inside base-functor alternatives are held through `Box<A>` … so that `F<X>` is a complete type even for incomplete `X`." Shipped layers hold `child_slot_t<A>` — `Box` only at the knot, `std::optional` elsewhere (`expression.hpp:54-64`, `binary_tree.hpp:149-154`, `child_slot.hpp:53-66`). The word `child_slot` does not occur in the paper or in `docs/tree_algorithms/recursive-tree-algorithms.org` (the article's `:413` still says "two nullable `Box<A>` children").
- `:476-491` Costs and `:504-506` Non-goals: "Performance work is an explicit non-goal for this revision", with `MEASURE` placeholders. `docs/tree_algorithms/DECISIONS.md:156-172` (amendment 2026-07-18) records 8.27 ms → 4.656 ms vs 4.642 ms hand-written on a 2^20-node fold, 0 allocations, and says "the paper should state the measured consequence: the verbs are zero-overhead against hand recursion over the same representation." `benchmarks/tree_algorithms/fold.bench.cpp:261-312` is the harness.
**Recommendation:** Rewrite `:145-147` around `child_slot` ("indirection exactly where the math requires it"), and replace the first `MEASURE` comment with the recorded numbers and the second with the benchmark's description.

### Finding 5.4 — Other stale text after the merge
**Severity:** minor each.
- `:459` "the three consumer-stub lookup objects" (see 1.3).
- `:539` reference URL `https://github.com/steve-downey/tree_algorithms` — repository retired (`SKILL.md:16-20`, `docs/coordination/SYNC.md:48-53`).
- `:528` "the companion repositories" — one repository now.
- `:44, 67` `overloaded` unattributed (see 1.2).
- `docs/tree_algorithms/README.md:9-12, 118-125` badges and vcpkg port `beman-tree-algorithms` point at `bemanproject/tree_algorithms`; `:76-77` names a `BEMAN_TREE_ALGORITHMS_BUILD_TESTS` option that `src/beman/tree_algorithms/CMakeLists.txt:68` does not read (it reads `BEMAN_TYPECLASS_BUILD_TESTS`).
- `docs/coordination/STATUS.md:57-58` still names `layer_fold_typeclass / project_typeclass / functor_typeclass` (transactional; will be removed).

---

## 6. Algebras under P2688 `match`

What the algebras look like today (verified): variant layers are `std::visit(overloaded{…}, layer)` with an explicit `-> Result` on each lambda (`expression.hpp:152-159`, `fringe_tree.hpp:373-379`, `functors.hpp:130-136`, `recursion_schemes.test.cpp:166-181`); the `fmap` bodies are the same shape (`expression.hpp:88-98`). Struct layers have no top-level case: the algebra is field access plus `slot ? *slot : identity` on the optional child slots (`binary_tree.hpp:231-233`, `rose_tree.hpp:148-152`, `examples/tree_algorithms/search_tree_on_fix.cpp:148-152`).

Caveat: P2688's concrete syntax is still moving; the sketches below use the `expr match { <Alt> [bindings] => …; }` shape and should be re-checked against the current revision before publication (the punch list already flags this: `docs/tree_algorithms/notes/punch-list.md:66`).

**Variant layers: a direct transliteration, shorter, same structure.**
```cpp
// expression.hpp:152-159 today -> with match
inline constexpr auto eval_algebra = [](const ExprF<int>& e) -> int {
    return e match {
        <Const<int>> [v]    => v;
        <Add<int>>   [l, r] => *l + *r;
        <Mul<int>>   [l, r] => *l * *r;
    };
};
// expression.hpp:86-99 fmap body
return layer match -> ExprF<B> {
    <Const<A>> [v]    => Const<B>{v};
    <Add<A>>   [l, r] => Add<B>{make_slot<B>(fn(*l)), make_slot<B>(fn(*r))};
    <Mul<A>>   [l, r] => Mul<B>{make_slot<B>(fn(*l)), make_slot<B>(fn(*r))};
};
```
This is the Haskell instance-by-clauses form (`fmap f (Add l r) = Add (f l) (f r)`) written directly; `overloaded` is the visitor/Church encoding of the same thing (`docs/coordination/essay/trees-and-transposition-for-29.org:59` says as much). The per-lambda `-> ExprF<B>` annotations that `std::visit` forces (common return type) collapse to one. `fold_fix` is untouched — the paper's claim at `:489-491` is right.

**Struct layers: `match` has almost nothing to grip.** `BinaryTreeF`, `RoseF`, `NodeF` have one shape; the only case analysis is engaged/absent on `child_slot_t<A>`, which would need the optional-like pattern (P1371 had `?`/`*`; whether and how P2688 keeps it must be checked). Two consequences:
- `Box` (`box.hpp:32-76`) has `explicit operator bool` and `operator*` but no `has_value()`; whichever protocol the match paper adopts for "optional-like", `child_slot`'s contract that boxed and inline slots "read the same" (`child_slot.hpp:40-42, 49-50`) must extend to it, or `BinaryTreeF<T, Fix<F>>` and `BinaryTreeF<T, int>` layers will match differently. This is a concrete design pressure on `Box` from pattern matching, worth a line in the paper.
- The rose layer (`std::vector<A> children`, `rose_tree.hpp:62-65`) gets nothing from `match`; the loop in `RoseLayerFoldMap` stays.
So the representation matrix the paper is proud of (five representations, `:459-465`) splits under `match`: the variant-based ones get the full benefit, the struct-based ones little. The paper currently says only "algebras get shorter" (`:491`).

**`overloaded` vs `match`: same job, different selection rule.** `overloaded` is best-match overload resolution with a consteval catch-all as diagnostic (`transpose/overloaded.hpp:27-35`); `match` is first-match. The dedup ledger already has the terminology (`machinery-deduplication.md:239`) and the gcc-16 finding that the catch-all's diagnostic is lost for every *value-returning* visitor because libstdc++'s return-type check fires first (`:186`) — and every algebra in this library returns a value, so the exhaustiveness story `overloaded` is proposed on never actually fires for Paper D's algebras (inferred from the ledger's check; not reproduced here). Under `match`, `_ => …` is a legitimate arm, which dissolves the deferred `fallback-overload-set` question (`:218-240`). Recommendation: P3200's forthcoming `overloaded` section should position it as the pre-`match` spelling of a one-layer algebra and say what survives if `match` lands (the typeclass registration and the verbs; not `overloaded`).

**Where typeclass and pattern matching are orthogonal, and where they meet.** `match` replaces the dispatch *inside* an instance's `fmap` and inside an algebra; it does not touch lookup (`functor<Layer>`, `layer_fold<Layer>`) or the verbs. The one place they meet is type-level: the lookup variables are themselves pattern-matched by partial specialization (`functor<BinaryTreeF<T, A>>` is a type pattern with two free variables), and that is the overload-resolution-as-instance-resolution mechanism the owner wants to compare with Haskell and Lean. tree_algorithms is the right test bed for that comparison because its instances are the only ones in the repo with two free pattern variables and the only ones registered on `std` types (`std::variant<…>`).

---

## Verified vs inferred

Verified by reading: everything cited with `file:line`. Verified by compilation (clang-18, `-Isrc`): explicit-tier include closure; `transpose::fmap` on a layer; `functor_object` false on `ExprF<int>` with the `applicative_value` cause; qualified partial specialization of `beman::transpose::functor` from global scope; `fold_map` `Result` deduced from `identity`. Not verified: GCC's acceptance of the qualified specialization (no GCC ≥ 14 here); the gcc-16 `overloaded` catch-all behaviour (taken from the ledger); P2688's current pattern grammar (sketches are illustrative). Inferred and marked: partial-ordering of `functor<std::variant<…>>` against a hypothetical generic specialization; the reason the paper's Before/After shows the explicit tier.
