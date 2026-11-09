# Tree payload adapters

Stage 4 of the [coherence plan](coherence-plan.md), stacked on configured
reconstruction PR #47. Implementation uses branch
`tree-coherence-payload-adapters` in the reused conformance worktree and Debug
build directory.

## Behavior

`tree_typeclasses.hpp` registers payload Functor, Foldable, and Traversable
for BinaryTree, BinaryTreeFix, FringeTree, and RoseTreeFix. Expr supplies a
Foldable over integer constants. Canonical binary/rose Fix families acquire
payload metadata only in this consumer header. Structural layer metadata
continues to denote recursive child positions. Unsupported Fix forms, Nat,
and Expr's arbitrary payload mapping/traversal remain unregistered.

The lookup tier's monoid-object bridge delegates to the existing structural
fold, preserving mapper and monoid state. Foldable obtains derived operations
from transpose. Its declaration exposes the existing projections' payload-copy
requirement and the layer folds' copyable-carrier requirement.

Mapping and traversal instead record source positions by reference and own
representation-specific topology. Binary reconstruction distinguishes absent
left/right slots and uses each representation's ordinary node storage. Fringe
reconstruction preserves empty/leaf/branch shape and recomputes cached counts.
Rose reconstruction preserves each node's arity and child order. The common
sequencing step consumes transpose's vector primitive, preserving native
collection, pairwise fallback, and changing-carrier semantics. Deferred
reconstruction captures only owned shape. Fresh values move into result trees.

The expression algorithm example now uses the shared Foldable for length.
Its integer-only optional transformation is named and described as a restricted
example operation. Transpose's independent BinaryTree port remains separate;
an agreement test pins sparse topology and ordered effects. No duplicate
primitive machinery or library-to-fingertree dependency is introduced.

The [public guide](../tree_algorithms/typeclass-support.md) records the
supported matrix and ownership/copyability limits. Structural signatures and
paper synopsis are unchanged. The anchor inventory records the expression
example's narrower role. Evidence repositories were checked for related
vocabulary; these adapters remain a one-way consumer implementation, without
backporting or importing their machinery.

## Validation

- GCC 16 C++26 Debug: full build, 636/636 tests, examples, explicit-tier
  isolation, and all 57 standalone public-header checks passed without
  compiler diagnostics.
- GCC 16 and Clang 21 C++23: all 35 payload adapter cases (413 assertions
  each), all five existing layer conformance cases (105 assertions each),
  and the public support-guide example passed. This includes constant
  evaluation of binary/rose mapping, folding, and traversal, plus Expr length.
- All three installed-package consumers pass. The tree consumer exercises
  payload mapping, traversal, and Expr Foldable through installed headers
  and exported targets.
- A C++26 smoke test including both library umbrellas passes on GCC and
  Clang, comparing payload mapping and monoidal folds across their trees.
- Changed C++ files pass clang-format 22.1.8. Wording hashes and whitespace
  checks pass; this stage changes no transpose source or generated fragment.

The CI follow-up also exercises the GCC 16 C++23 module build. Its payload
suite passes all 35 cases (413 assertions), and its binary-tree suite passes
all 9 cases (57 assertions). Benchmarks and opt-in P2300 evidence remain
outside this phase's local validation.

## CI follow-up

Textual transpose and example headers precede imports in the payload tests.
The binary-tree tests explicitly include the payload consumer and check both
its complete-tree instance and the separate child-position layer instances,
matching the module's umbrella surface.

The first module build exposed two GCC issues. Naming the explicit object
parameter of transpose's `for_each` avoids a compiler crash when GCC merges
its dependent constraint from a textual header and a module. Its category
selection and delegated call are unchanged, and the existing explicit `T`
and `F` template arguments retain their positions. P3200 wording was
regenerated with specgen 0.15.0. Expr shares the constrained Foldable
registration with the other supported trees, rather than using a full
variable-template specialization whose definition GCC omitted when linking
module consumers. This adds no construction interface for Expr.

Binary shape recording completes recursive right-subtree work before
indexing its vector, so no access can span reallocation; this also addresses
the MSVC constant-evaluation failure without weakening the constexpr tests.
The stack uses the formatter pinned by CI, clang-format 22.1.8.

## Remaining scope

Stage 5 adds optional structural fingertree integration; stage 6 closes the
integrated context and packaging matrix. This stage adds no dependency on
fingertree and changes no transpose primitive or generated wording.
