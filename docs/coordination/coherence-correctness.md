# Tree coherence: correctness repairs

Implementation of stage 1 of the [coherence plan](coherence-plan.md), on
`tree-coherence-correctness`, based on `5b3c0a6`.

## Changes

Unequal-allocator append rebuilds the right operand with the left operand's
stored monoid as well as its allocator. The regression covers empty, single,
and deep roots using a stateful modular sum and distinct PMR resources. The
review's `[1] ++ [4,4]` case retains modulus 5 and measure 4.

All five fingertree Traversable implementations delegate sequencing to
transpose's vector Traversable. They now prefer a native `collect`, including
on empty inputs, and retain pairwise composition for policies without it.
Delegation uses const lvalue vectors to preserve persistent element access.
The interval index sequences payload effects and captures owned endpoint
metadata for reconstruction.

The transpose binary-tree example records an in-order node list and an owned
shape descriptor, sequences each value effect once, and rebuilds from fresh
values inside the final context. Absent children contribute no effects.
Runtime collection can change the context carrier without encountering
recursive return-type or accumulator-assignment requirements. The additional
node and shape storage is O(n); recursive walking/rebuilding retains the
existing depth-dependent stack usage.

## Regression coverage

- Noncommutative Writer logs agree with binary-tree fold order for leaves,
  full trees, and left/right sparse trees; mapper invocation order agrees too.
- All five fingertree views preserve element order and interval endpoints
  through native collection, collection with a changing carrier, and pairwise
  composition, including empty inputs and persistent rvalues.
- Deferred binary and interval traversals execute after their source and
  mapper have been destroyed, under both sequencing routes.
- Optional failure and type-changing binary traversal continue to work.

## Validation

- GCC 16.2.1, C++26 Debug: complete build succeeded, **544/544 tests passed**,
  and all **53** standalone interface-header compilation steps passed.
- Clang 21 with GCC 16's standard library: all **20** binary-tree and
  fingertree traversal cases passed. The binary-tree tests use C++23 and the
  fingertree tests use C++26.
- GCC 16, C++23: all **6** binary-tree cases passed.
- `clang-format-21 --dry-run --Werror` on changed C++ files and
  `git diff --check` passed.

The worktree's build is `build/correctness`, configured with
`cmake/gcc-16-toolchain.cmake` and the fetch-content provider, reusing only
Catch2's source directory from the main checkout. Full-suite and header
results are in `build/correctness/ctest.log` and `headers.log` respectively.
Benchmarks and opt-in P2300 evidence were disabled; module configurations
were not exercised in this stage.

## Remaining scope

This stage repairs correctness under existing interfaces. It does not add
the tree payload/structural adapters, conformance metadata, or reconstruction
customizations proposed in later stages. Fingertree traversal still rebuilds
using its existing default result family; preserving custom measures,
allocators, monoid instances, and move-only values remains stage 3. Rope's
generic registration retains its existing string-result limitation until its
planned migration. No primitive typeclass definition, frozen API decision,
or mechanism-free tree-algorithm header changes in this stage.

The [original review](review/2026-11-08-coherence/README.md) remains evidence
about its recorded baseline; its failing observations are not rewritten as
though the defects had never existed.
