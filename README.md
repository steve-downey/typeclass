# beman.typeclass: Typeclass Based Libraries for C++

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

<!-- markdownlint-disable line-length -->
[![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#the-beman-library-maturity-model)
[![Continuous Integration Tests](https://github.com/bemanproject/typeclass/actions/workflows/ci_tests.yml/badge.svg)](https://github.com/bemanproject/typeclass/actions/workflows/ci_tests.yml)
[![Lint Check (pre-commit)](https://github.com/bemanproject/typeclass/actions/workflows/pre-commit-check.yml/badge.svg)](https://github.com/bemanproject/typeclass/actions/workflows/pre-commit-check.yml)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)
<!-- markdownlint-restore -->

Three libraries that share one design: typeclass objects as the customization mechanism, and the Functor / Applicative / Foldable / Traversable vocabulary built on it.
They live in one repository because they layer on the same machinery, and a change to that machinery has to be checked against all of them at once.
Each is still its own CMake target, its own installed package, and its own paper.

| Library | Namespace | What it is | Paper | Notes |
|---|---|---|---|---|
| `beman.transpose` | `beman::transpose` | Shape-preserving `traverse` and `transpose` for contextual computations, and the typeclass-object mechanism itself | P3200, [`papers/D3200R0.md`](papers/D3200R0.md) | [`docs/transpose/`](docs/transpose/README.md) |
| `beman.tree_algorithms` | `beman::tree_algorithms` | The fold family over recursive trees: `fold_fix`, `unfold_fix`, `refold`, and the direct verbs `fold_with` / `unfold_with` | D4322, [`papers/algorithms-for-trees.md`](papers/algorithms-for-trees.md) | [`docs/tree_algorithms/`](docs/tree_algorithms/README.md) |
| `beman.fingertree` | `beman::fingertree` | A persistent measured sequence | not yet drafted | [`docs/fingertree/`](docs/fingertree/README.md) |

How the three relate, and the invariants that keep them coherent, are in [`docs/coordination/`](docs/coordination/README.md).

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## History

Each library was developed in its own repository: `steve-downey/transpose`, `steve-downey/tree_algorithms`, and `steve-downey/fingertree`, coordinated from `steve-downey/tree-coordination`.
Those repositories are retired.
Their histories were merged here with every commit's SHA unchanged, so commit references in the decision logs and coordination ledger still resolve.
Published blog posts were pinned to tags in those repositories; the tags are carried here with the repository's name as a prefix (`transpose/blog/graded-gadgets`).

## Layout

```txt
src/beman/<library>/    headers, tests and CMake for each library, side by side
examples/<library>/     runnable examples, several transcluded into papers and posts
benchmarks/<library>/   benchmarks (tree_algorithms)
papers/                 every paper, built by the vendored MPark.WG21 in papers/wg21/
papers/blog/            the blog series
docs/<library>/         design notes and decision logs for each library
docs/coordination/      cross-library status, the coordination ledger, and the essay
```

`src/` is the single include root: headers are included by their canonical names, `<beman/transpose/traverse.hpp>`, as they will be once installed.

## License

The libraries are licensed under the Apache License v2.0 with LLVM Exceptions.
The `beman.fingertree` design documents are additionally offered under CC BY-SA 4.0; see [`docs/fingertree/LICENSE.CC-BY-SA-4.0`](docs/fingertree/LICENSE.CC-BY-SA-4.0).

## Building

### Requirements

* The C++23 standard is the minimum for `beman.transpose` and `beman.tree_algorithms`, with GCC 14, Clang 19, current AppleClang or MSVC.
* `beman.fingertree` needs `std::inplace_vector` at C++26, which today means GCC 16.
  The build probes for it and skips fingertree where it is missing; force the choice with `-DBEMAN_TYPECLASS_BUILD_FINGERTREE=ON|OFF`.
* CMake 3.30 or later.
* (Tests and benchmarks only) Catch2 3.
* (Optional) [beman.execution](https://github.com/bemanproject/execution), for transpose's P2300 evidence: `-DBEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE=ON`.

The CI matrix in [`.github/workflows/ci_tests.yml`](.github/workflows/ci_tests.yml) is the authoritative list of what is tested.

### With a preset

```bash
cmake --workflow --preset gcc-release
cmake --list-presets=workflow
```

### With the Makefile

```bash
make TOOLCHAIN=gcc-16 test        # configure, build, run every test
make TOOLCHAIN=gcc-16 compile-headers
make wording-check                # P3200 wording still matches the headers
make papers                       # build every paper
make docs                         # API docs for all three libraries, into docs/html
make TOOLCHAIN=gcc-16 testinstall # install, then build a downstream project against it
make bench                        # tree_algorithms runtime benchmarks, Bench config
make help
```

### Options

| Option | Default | Effect |
|---|---|---|
| `BEMAN_TYPECLASS_BUILD_TESTS` | top level | Build the tests |
| `BEMAN_TYPECLASS_BUILD_EXAMPLES` | top level | Build the examples |
| `BEMAN_TYPECLASS_BUILD_BENCHMARKS` | top level | Build (not run) the benchmarks |
| `BEMAN_TYPECLASS_BUILD_FINGERTREE` | probed | Build `beman.fingertree` |
| `BEMAN_TYPECLASS_USE_MODULES` | `OFF` | Build `beman.tree_algorithms` as a C++ module |
| `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE` | `OFF` | Build the P2300 sender adapter and its tests |

## Using the libraries

Each library installs as its own CMake package:

```cmake
find_package(beman.transpose REQUIRED)
target_link_libraries(yourlib PUBLIC beman::transpose)
```

and likewise `beman.tree_algorithms` / `beman::tree_algorithms` and `beman.fingertree` / `beman::fingertree`.

## Development

See the [Contributing Guidelines](CONTRIBUTING.md).
