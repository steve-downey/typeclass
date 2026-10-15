# 04 — Infrastructure review: build, CI, install, modules, paper/blog tooling, API docs

Reviewed at HEAD `71129ec` (merge `018e006` + essay merge). Repository read-only. Where I say
**verified** I ran it or read the exact text; where I say **inferred** I am reasoning from
configuration I could not execute here. Local toolchain: gcc 13.3, clang 18.1.3, cmake 3.28
(system) and cmake 4.3.2 (installed into a scratch venv via `uv`, as `pyproject.toml` pins) — all
below the project's compiler floor (GCC 14 / Clang 19), so I could configure, enumerate targets,
install and run the explicit-tier isolation check, but not compile the libraries' tests.

## Executive summary

1. **The CMake tree is one coherent project** — one `project()`, three `beman.<lib>` /
   `beman::<lib>` targets, three installed packages, consistent option prefix, and
   `tree_algorithms`/`fingertree` genuinely link `beman::transpose`. Verified by configuring with
   CMake 4.3.2 and installing: `beman.transpose` and `beman.tree_algorithms` packages are produced,
   and the latter's config does `find_dependency(beman.transpose)`.
2. **Everything around the CMake tree still thinks it is a single standalone library.** The
   Makefile's `docs`, `mrdocs` and `testinstall` targets point at files that do not exist;
   `installtest/` covers only transpose and is broken by its own relocation; `cmake/` is an
   older fork of `infra/cmake/` (two dependency providers, two `Config.cmake.in`); the gcovr
   filter matches a path that never existed in this repo.
3. **The fingertree C++26 gate is unsound at the edge**: on a compiler CMake has no C++26 flag
   for (gcc 13 here) the probe is a hard configure *error*, not the promised skip. Verified.
4. **API docs do not exist as a product.** Two Doxyfiles (transpose's is the 2943-line exemplar
   file with stale paths and the placeholder brief "An Example of Everything"; fingertree's
   points at `include/beman`), none for tree_algorithms, an untouched exemplar `mrdocs.yml` for
   `beman::optional`, and a gh-pages workflow that fails on every push to `main` because
   `docs/Doxyfile` does not exist (known since July per the tree_algorithms punch list). There
   are zero Doxygen groups/pages/anchors anywhere, so there is nothing to collide — and no
   structure. Doxygen will also ingest the `//!` specgen wording blocks as documentation.
5. **CI is broad on compilers but has three real gaps**: nothing runs `make wording-check`, nothing
   runs the install test, and the vendored `papers/wg21` test suite is never executed. The
   vendored workflows under `infra/.github` and `papers/wg21/.github` are dead, not live.
6. **Paper tooling is sound for one paper and hard-wired to it**: `gen-wording.sh` hard-codes the
   eight transpose headers and `transpose.manifest`; `papers/Makefile` wires the wording stamp
   only to D3200R0; `make papers` requires `specgen` even though fragments are checked in,
   contradicting `wording-pipeline.md`; and `wording-pipeline.md` and `defaults.yaml` disagree on
   whether a pandoc defaults file appends or replaces the filter list.
7. **Blog tooling leaks the owner's filesystem**: five posts and the essay transclude from
   `orgit-file:~/src/typeclass/typeclass::...`; `pins.md` is referenced four times and does not
   exist; the two 5.7k-line CSS files are referenced by nothing.

---

## 1. Is the single codebase one coherent build?

### 1.1 Naming is consistent — with two documentation exceptions (minor)

**Verified.** Pattern is uniform across the three libraries:

| | transpose | tree_algorithms | fingertree |
|---|---|---|---|
| target | `beman.transpose` (`src/beman/transpose/CMakeLists.txt:11`) | `beman.tree_algorithms` (`:14`/`:22`) | `beman.fingertree` (`:16`/`:24`) |
| alias | `beman::transpose` (`:12`) | `beman::tree_algorithms` (`:30`) | `beman::fingertree` (`:32`) |
| package | `beman_install_library(beman.transpose …)` (`:51`) | `… DEPENDENCIES "beman.transpose"` (`:62-66`) | `… DEPENDENCIES "beman.transpose"` (`:72-76`) |
| test prefix | `beman.transpose.tests.*` | `beman.tree_algorithms.tests.*` | `beman.fingertree.tests.*` |
| standard | `cxx_std_23` (`:14`) | `cxx_std_23` (`:19,23`) | `cxx_std_26` (`:22,26`) |

Project options use `BEMAN_TYPECLASS_*` (`CMakeLists.txt:22-50`), with one library-scoped option
`BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE` (`CMakeLists.txt:54-58`) — defensible, since it is
transpose-only. The vcpkg port is a single `beman-typeclass` carrying three packages
(`port/vcpkg.json.in`, `port/portfile.cmake.in:26-44`) and does the right fixup dance.

Verified by install (CMake 4.3.2, clang 18, fingertree skipped by probe): `lib/cmake/beman.transpose/`
and `lib/cmake/beman.tree_algorithms/` with `-config`, `-config-version`, `-targets`; 38 headers under
`include/beman/{transpose,tree_algorithms}/`; the tree_algorithms config emits
`find_dependency(beman.transpose)`.

Exceptions:

- `CONTRIBUTING.md:131-137` documents `BEMAN_TYPECLASS_INSTALL_CONFIG_FILE_PACKAGE` and
  `find_package(beman.typeclass)`. Neither exists: there is no `beman.typeclass` target or package,
  and the install module's per-package switch is `BEMAN_TRANSPOSE_INSTALL_CONFIG_FILE_PACKAGE` etc.
  (`infra/cmake/beman-install-library.cmake:65-68, 249-262`).
- `.beman-tidy.yaml:13-25` disables `cmake.library_name`, `cmake.library_alias`, `directory.sources`,
  `directory.tests`, `file.names` with the reason recorded. Good; this is the honest statement that
  the repo is not a Beman single-library repo.

### 1.2 `installtest/` covers one library and is broken by the move (significant)

**Verified.**

- Only `installtest/transpose/` exists; no install test for `tree_algorithms` or `fingertree`
  (`find installtest -type f` → four files, all under `transpose/`).
- `Makefile:333-336` runs `cmake -S installtest -B installtest/.build` but `installtest/CMakeLists.txt`
  does not exist (`ls` fails); the CMakeLists is at `installtest/transpose/CMakeLists.txt`.
- `installtest/transpose/CMakeLists.txt:11-14` hard-codes
  `${CMAKE_CURRENT_LIST_DIR}/../.install/lib/cmake/beman.transpose` with `NO_DEFAULT_PATH`. Before the
  merge the test sat at `installtest/`, so `../.install` was the repo-root prefix the Makefile
  installs to (`Makefile:7`, `INSTALL_PREFIX?=.install/`). Now it resolves to
  `installtest/.install/…`, which nothing populates. I confirmed `find_package` fails when pointed via
  `CMAKE_PREFIX_PATH` (ignored because of `NO_DEFAULT_PATH`), and succeeds when I rebuilt the expected
  `../.install` layout in the scratchpad — i.e. the installed package itself is fine; the harness is
  not.
- `installtest/transpose/README.md:5` says `make TOOLCHAIN=gcc-16 installtest`; the Makefile target is
  `testinstall` (`Makefile:331`). `installtest` would fall through to `.DEFAULT` and be passed to
  `cmake --build --target installtest`, which fails.

**Why it matters.** The install test is the only check that a consumer can `find_package` each library
and that the `find_dependency` chain (`tree_algorithms → transpose`, `fingertree → transpose`) works.
The reusable CI workflow installs and `ls -R`s but never configures a consumer against the prefix
(fetched `reusable-beman-build-and-test.yml@1.7.4`, "Build step"), so today nothing tests consumption.

**Recommendation.** One `installtest/CMakeLists.txt` with three subprojects (or one project with three
executables: `find_package(beman.transpose)`, `find_package(beman.tree_algorithms)`,
`find_package(beman.fingertree)` guarded by `QUIET`/option so a build without fingertree still
passes), prefix taken from `CMAKE_PREFIX_PATH` rather than a hard-coded relative path, and a Makefile
target name that matches its README. Add it as a job step (`cmake -S installtest -B … -DCMAKE_PREFIX_PATH=/opt/beman.package`)
to `ci_tests.yml` or run it after `make install` in a Makefile-driven job.

### 1.3 The three config headers agree — two of three are dead (minor)

**Verified.** `config.hpp` and `config_generated.hpp.in` are the same text modulo prefix in all three
libraries (`src/beman/*/config.hpp`, `src/beman/*/config_generated.hpp.in`). The
`#cmakedefine01 BEMAN_X_USE_MODULES()` form (function-like macro) works: I ran `configure_file` with
the variable ON and unset and got `#define BEMAN_X_USE_MODULES() 1` / `… 0`; the real configure produced
`#define BEMAN_TRANSPOSE_USE_MODULES() 0` and `#define BEMAN_TREE_ALGORITHMS_USE_MODULES() 0`.

But:

- `BEMAN_TRANSPOSE_USE_MODULES` is never set by any CMake file and never read by any transpose header
  (`grep USE_MODULES src/beman/transpose/*.hpp` → only `config*.hpp`). It is generated, always 0, and
  unused.
- `BEMAN_FINGERTREE_USE_MODULES` is read by `fingertree.hpp:8` but forced `OFF` at
  `src/beman/fingertree/CMakeLists.txt:7`, so the `fingertree.cppm` branch is unreachable.
- Only tree_algorithms (`tree_algorithms.hpp:8`, `fix.hpp:8`, `child_slot.hpp:8`, `recursion_schemes.hpp:8`,
  `todo.hpp:8`) actually uses its macro.

Keep per-library config headers (each installed package must stand alone), but either wire the
transpose/fingertree macros to something or drop the dead `#cmakedefine01` lines so a reader does not
infer module support that does not exist.

### 1.4 Modules: one of three built, only in CI, and inconsistently plumbed (significant)

**Verified from configuration; not built here (no gcc 15/16, clang 22).**

- `tree_algorithms.cppm` is built only with `-DBEMAN_TYPECLASS_USE_MODULES=ON` (`CMakeLists.txt:45-49, 62-64`;
  `src/beman/tree_algorithms/CMakeLists.txt:5, 13-20`). CI enables it in exactly four rows, all at C++23
  `Debug`: gcc 16 (`ci_tests.yml:76`), gcc 15 (`:99`), clang 22 libstdc++/libc++ (`:147`), msvc (`:195`),
  plus the vcpkg `modules` feature (`:211-212`, `port/portfile.cmake.in:13`). Never at C++26, never
  Release/sanitized.
- `fingertree.cppm` (11 lines) is dead code: `set(BEMAN_FINGERTREE_USE_MODULES OFF)` with the reason in
  the comment (`src/beman/fingertree/CMakeLists.txt:4-7`).
- transpose has no interface unit; `ci_tests.yml:53` says so.
- The tree_algorithms unit includes `beman/transpose/functor.hpp` and `overloaded.hpp` in the global
  module fragment (`tree_algorithms.cppm:8-9`). That is correct for attachment, but it means a consumer
  that `import beman.tree_algorithms;` still needs the transpose *headers* on its include path and
  gets them as textual includes — the "module" is a module over a header-only dependency. Worth stating
  in the README's options table (`README.md:93`).
- Inconsistent gating: `examples/fingertree/CMakeLists.txt:12-17` and `benchmarks/tree_algorithms/CMakeLists.txt:31-36`
  gate `CXX_MODULE_STD ON` on `BEMAN_TYPECLASS_USE_MODULES`, while the fingertree library and tests gate
  on `BEMAN_FINGERTREE_USE_MODULES` (always OFF). So with the project option ON, fingertree examples
  are built with `import std` enabled against a non-module fingertree. Probably harmless, but it is the
  kind of mismatch the punch list flagged pre-merge (`docs/tree_algorithms/notes/punch-list.md:37`,
  `BEMAN_EXEMPLAR_USE_MODULES`).
- `docs/tree_algorithms/notes/punch-list.md:30` (July 2026) reports the modules build "red across the
  board in CI". I cannot tell whether that is still true; `.github/workflows/ci_tests.yml` no longer
  carries a comment about it. Please check the gcc-16/clang-22 modules rows before relying on them.
- The explicit-tier isolation check (`src/beman/tree_algorithms/CMakeLists.txt:118-156`) is nicely
  self-generating and **compiled clean under clang 18 here** (5 TUs, 0 failures) — it works even on a
  below-floor compiler because it only needs the headers to preprocess.

### 1.5 The fingertree C++26 gate (significant)

`src/beman/CMakeLists.txt:9-25` probes `<inplace_vector>` inside `block()` with
`CMAKE_CXX_STANDARD 26` + `CMAKE_CXX_STANDARD_REQUIRED ON`.

**Verified failure mode.** With gcc 13 (CMake knows no C++26 dialect for it) configure aborts:
`Target "cmTC_…" requires the language dialect "CXX26" … does not support this` — a hard error from
`check_cxx_source_compiles`, not the "skipped" message. With clang 18 (dialect known, header absent) it
correctly prints `beman.fingertree: skipped, std::inplace_vector is not available at C++26` and
configures 53 tests. So the gate is sound only for compilers that already have a C++26 mode; below the
project's stated floor (GCC 14, Clang 19) that is every supported compiler, so this is a robustness
issue rather than a CI failure — but `README.md:59-60` promises "the build probes for it and skips
fingertree where it is missing", and it does not for any pre-C++26 toolchain. Fix: drop
`CMAKE_CXX_STANDARD_REQUIRED` in the probe (or `check_cxx_compiler_flag(-std=c++26)` first) so an
unknown dialect yields `FALSE` instead of an error.

**Premise is doubtful (inferred).** `src/beman/CMakeLists.txt:5`, `ci_tests.yml:39-41` and
`README.md:59` all say only libstdc++ 16 ships `<inplace_vector>`. GCC 15's release notes page does not
list it either way (fetched `gcc.gnu.org/gcc-15/changes.html`, no mention), but libstdc++'s
`std::inplace_vector` implementation (P0843R14) was committed for GCC 15 to my knowledge. If so, the
gcc 15 C++26 rows (`ci_tests.yml:83-92`) are already building and testing fingertree, and the comments
understate coverage. Check a gcc 15 CI log for the "skipped" line; adjust the comments and the README
floor statement accordingly.

**Design observation.** The gate makes `BEMAN_TYPECLASS_BUILD_FINGERTREE` default to a probe of the
*default* toolchain, and the vcpkg portfile then has to discover after the fact which packages were
installed (`port/portfile.cmake.in:26-44`). That is the right pragmatic shape for now, but a consumer of
the vcpkg port has no feature to request fingertree and no error if it silently was not built. Consider
a `fingertree` vcpkg feature that sets `-DBEMAN_TYPECLASS_BUILD_FINGERTREE=ON` so absence is loud.

### 1.6 Two toolchain/dependency-provider trees (significant)

**Verified.**

- Presets and CI use `infra/cmake/*` (`CMakePresets.json:13, 42, 52, …`; the reusable workflow passes
  `-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake` and
  `infra/cmake/{gnu,llvm,llvm-libc++,appleclang,msvc}-toolchain.cmake`).
- The Makefile uses `cmake/*` (`Makefile:47-50` → `cmake/$(TOOLCHAIN)-toolchain.cmake`; `:57`
  `_cmake_top_level?="./cmake/use-fetch-content.cmake"`) while *also* adding `-DCMAKE_PREFIX_PATH=$(CURDIR)/infra/cmake`
  (`:80`).
- `cmake/use-fetch-content.cmake` is an older fork of `infra/cmake/use-fetch-content.cmake` (diff:
  different variable prefix, no per-dependency `cmake_args`, no Catch2 `extras/` module-path fix). The
  root `CMakeLists.txt:80-92` carries a workaround ("recover it from FetchContent's global properties")
  for exactly the gap the infra version already closes. One provider should go; the infra one is the
  one CI exercises.
- `cmake/Config.cmake.in` is dead: `beman_install_library` reads
  `${CMAKE_CURRENT_FUNCTION_LIST_DIR}/Config.cmake.in` (`infra/cmake/beman-install-library.cmake:298`),
  and the installed config I inspected is the infra template. The `cmake/` copy would not even work
  (no `find_dependency`, different `@…@` names).
- `cmake/clang-flags.cmake:5,12` pins `CMAKE_CXX_STANDARD 20` / `-std=gnu++20 -stdlib=libc++` while
  `cmake/gcc-flags.cmake:5,12` pins `26` / `-std=gnu++26`. The per-target `cxx_std_23/26` features then
  append a second `-std=` later on the command line, so it works (last wins) but a Makefile clang build
  and a Makefile gcc build are not the same configuration, and neither is what CI runs.
- `cmake/gcovr.cfg.in:10` filters `.*/include/beman/typeclass/.*` — a path that never existed in this
  repo (there is no `include/`, and no `beman/typeclass/`). `make coverage` (`Makefile:302-306`) will
  produce an empty report. CI's Coverage row does not use this file (the reusable workflow writes its
  own `gcovr.cfg`), so this only affects local coverage.
- Versioned toolchain files exist for gcc 12-16 and clang 16-22 (`cmake/`), i.e. below the README floor
  (gcc 12/13, clang 16-18). They are harmless but imply support that `README.md:58` denies.

**Recommendation.** Keep `infra/cmake` as the one dependency-provider/toolchain base; reduce `cmake/` to
the project-specific multi-config flag files (`gcc-flags.cmake`, `clang-flags.cmake`, `x64-linux-custom.cmake`,
the versioned `<cc>-NN-toolchain.cmake` wrappers that `make TOOLCHAIN=` needs) and delete
`cmake/use-fetch-content.cmake`, `cmake/Config.cmake.in`, `cmake/{ci-clang,llvm,llvm-16,llvm-master,gcc}-toolchain.cmake`.
Fix the gcovr filter to `.*/src/beman/.*` and exclude `.*\.test\.cpp`.

---

## 2. CI: what runs, redundancy, gaps

Eight workflows plus `.yamllint` (`.github/workflows/`):

| workflow | triggers | what it does | state |
|---|---|---|---|
| `ci_tests.yml` | push main, PR, dispatch, cron Fri 08:55 | `beman-submodule-check`; `preset-test` over the 8 presets on `:latest` gcc/clang images + macos + windows; `build-and-test` matrix (gcc 16/15/14, clang 23/22/21/20/19, appleclang, msvc); `vcpkg-ci` (plain + `modules`); issue-on-fault for scheduled runs | live |
| `codeql.yml` | push/PR main, cron | CodeQL for `actions` only (`:43-44`), not C++ | live, narrow |
| `commit-authorship.yml` | push main, PR, dispatch | rejects Claude-authored/attributed commits, exempting imported history | live, **verified working**: the two `Claude <noreply@anthropic.com>` commits in history (`c4275c2`, `af05316`) are ancestors of exempt tip `7850388e2` |
| `doxygen-gh-pages.yml` | push main / `doxify` | runs Doxygen with `config_file: docs/Doxyfile` (`:30`), deploys `docs/html` to `gh-pages` | **fails every push**: `docs/Doxyfile` does not exist; known since July (`docs/tree_algorithms/notes/punch-list.md:58`) |
| `ossf-scorecard-analysis.yml` | push main, cron | Scorecard → SARIF | live |
| `pre-commit-check.yml` | `pull_request_target`, push main | reusable pre-commit | live |
| `pre-commit-update.yml` | cron Thu 09:29, dispatch | autoupdate PR | live (needs bot secrets) |
| `vcpkg-release.yml` | release published | registry release for `beman-typeclass` | live |

### 2.1 Redundancy (minor)

`preset-test` and `build-and-test` both build gcc/clang Debug+sanitizers and Release. The preset rows
run on unpinned `:latest` images (`ci_tests.yml:28-31`) at `CMAKE_CXX_STANDARD 26` for *every* preset
(`CMakePresets.json:10`), including `appleclang-*` and `msvc-*`. The matrix comment (`ci_tests.yml:43-50`)
says appleclang must stay on C++23 because of the libc++ `std::expected` ADL bug, and the matrix keeps
msvc at C++23 (`:188-190`). So either the appleclang/msvc *preset* rows fail at C++26 or the comment is
stale — **inferred**; worth a look at a recent run. If the presets are retained for the Beman tooling
contract, set `CMAKE_CXX_STANDARD` per preset.

### 2.2 Gaps (significant)

**Verified absent from every workflow:**

1. `make wording-check` (`Makefile:217-221`) — the only guard that the checked-in P3200 wording still
   matches the headers. It needs `specgen` + Clang 22, which CI does not have; so wording drift is
   detected only on the owner's machine. Either build a small container with specgen or at least run
   the check in the scheduled job with a clear skip message when specgen is absent.
2. Paper build (`make -C papers pdf`) and blog export — no workflow runs pandoc or emacs. The papers are
   documents the repo exists to produce; a broken transclusion (`transclude.py` `fail()`) or a
   `wg21.py` crash only shows up locally.
3. Install test (see 1.2). The reusable workflow installs to `/opt/beman.package` and lists it, nothing
   more.
4. The vendored `papers/wg21/tests/` (`make -C papers/wg21 check`, `papers/wg21/Makefile:11-13`) is never
   run. Since `papers/filters/bibliography-header.py` exists precisely because of an upstream
   defect, running upstream's own suite when the subtree is bumped would be cheap insurance.
5. Modules only at C++23 Debug (1.4); no `Werror` on gcc at C++23; `Debug.Coverage` only gcc 16.
6. Coverage row: the reusable workflow's gcovr excludes `tests/` (fetched workflow), but this repo's
   tests are `src/beman/<lib>/*.test.cpp`, so test TUs are counted as covered source. **Inferred.**

### 2.3 Vendored workflows: dead, not live (note)

`infra/.github/workflows/{pre-commit,reusable-beman-create-issue-when-fault}.yml` and
`papers/wg21/.github/workflows/static.yml` are not under the repository's `.github/workflows/`, so
GitHub never schedules them. Harmless. `static.yml` triggers on `master`, which this repo does not have,
even if it were moved. `.gitattributes` marks `infra/**` vendored and `papers/*` documentation; add
`papers/wg21/** linguist-vendored` for consistency.

### 2.4 Preset/toolchain coverage vs README claims

`README.md:58-61` claims GCC 14, Clang 19, current AppleClang, MSVC for C++23; GCC 16 for fingertree;
CMake 3.30. The matrix (`ci_tests.yml:106-112` gcc 14 Release only; `:168-176` clang 19/20 Release only;
`:179-185` appleclang c++23; `:186-201` msvc c++23) matches the floors, and `README.md:65` correctly
defers to the matrix. Two mismatches:

- `CONTRIBUTING.md:76` shows `-DCMAKE_CXX_STANDARD=17`, and `:40,68,92` say GoogleTest; the project is
  C++23/26 with Catch2 (`lockfile.json`, `vcpkg.json`). Exemplar text.
- `docs/fingertree/README.md:48-58` lists GCC 11-16 and Clang 17-22 at C++17-26 as "supported
  platforms" — the standalone repo's table, false for a C++26-only library.

### 2.5 Lint configuration (note)

`.pre-commit-config.yaml:52` excludes `infra/`, `port/`, `papers/wg21/`; codespell additionally excludes
all of `papers/` (`:44`), so the papers and blog posts — the most text-heavy, most public content — are
not spell-checked. markdownlint is commented out (`:30-34`). `pyproject.toml` pins `clang-format==22.1.5`
while pre-commit pins `v22.1.8` (`.pre-commit-config.yaml:17`): two formatters, one `.clang-format` …
actually four `.clang-format` files with four different checksums (root, `src/beman/transpose/`,
`examples/transpose/`, `installtest/transpose/`; `md5sum` verified). The transpose copies' header
comment says they were kept to avoid reformatting transcluded code (`installtest/transpose/.clang-format:2-5`),
which is a legitimate reason for *one* preserved style, not three divergent ones.

---

## 3. API documentation

### 3.1 Current state (blocking for the "coherent, integrated API docs" goal)

**Verified.**

- `docs/transpose/Doxyfile` (2943 lines) is the exemplar's generated file with a handful of edits:
  `PROJECT_NAME = "transpose"` (`:47`), `PROJECT_BRIEF = An Example of Everything` (`:59`, exemplar
  placeholder), `PROJECT_LOGO/ICON = docs/beman-logo.png` (`:66-67`, file does not exist),
  `OUTPUT_DIRECTORY = docs` (`:79` → `docs/html`, gitignored), `INPUT = src docs` (`:996`, relative to
  the working directory; run from the root it ingests **all three libraries** and every `docs/**/*.md`
  including the transactional coordination/review documents slated for deletion),
  `EXCLUDE = docs/debug-ci.md` (`:1054`, nonexistent), `EXAMPLE_PATH = src/examples/` (`:1084`,
  nonexistent; examples are `examples/<lib>/`), `HTML_EXTRA_STYLESHEET = docs/doxygen-awesome.css`
  (`:1173`; the file is `docs/transpose/doxygen-awesome.css`), `CLANG_ASSISTED_PARSING = YES` with
  `CLANG_DATABASE_PATH = ./` (`:1281, 1310`; needs a `compile_commands.json` symlink at the root, which
  only the Makefile creates), `EXTRACT_ALL = YES`, `WARN_IF_UNDOCUMENTED = YES`.
- `docs/fingertree/Doxyfile` (19 lines): `INPUT = include/beman README.md` (`:12`) — both stale after
  the move; `OUTPUT_DIRECTORY = docs` (`:7`) would collide with transpose's output.
- No Doxyfile for tree_algorithms.
- `docs/transpose/mrdocs.yml` is the exemplar's `beman::optional` configuration, untouched (`:8-22`
  reference `../include/beman/optional/detail`, `beman::optional::**`).
- `Makefile:322-329` `docs`/`mrdocs` targets run `doxygen docs/Doxyfile` and `cd docs && mrdocs mrdocs.yml`
  — neither file exists at that path. The gh-pages workflow uses the same nonexistent `docs/Doxyfile`.
- **Nothing is published.** The gh-pages workflow fails before deploying; there is no `gh-pages`
  content to protect.
- Doxygen structure: `grep` for `@defgroup|@addtogroup|@ingroup|@page|@mainpage|@anchor|@file` across
  `src/`, `examples/`, `docs/` returns **nothing**. So there are no group/anchor collisions — and no
  navigation beyond namespace/file lists.
- Doc-comment dialects diverge: transpose has 332 `///` lines, 721 `//!` specgen lines and 226 `/** */`
  blocks; tree_algorithms 114 `/** */` blocks and no `///`/`//!`; fingertree 41 `/** */` blocks
  (counts over `src/beman/<lib>/*.hpp`). `docs/transpose/wording-pipeline.md:83-85` states that `///`
  and `/** */` are "Doxygen" and `//!` is specgen — but Doxygen treats `//!` as a Qt-style doc comment
  too. A Doxygen run over transpose will therefore attach both the rationale (`///`) and the normative
  text with its specgen commands (`//! \seebelow`, `\omit`, `\expos`, `\rSec3[...]`, `\ref{...}`,
  e.g. `include/beman/transpose/apply.hpp:19-31, 44, 53`) to each entity, and warn on every unknown
  command. Doxygen has no idea these are two audiences.
- The only `doxygen-awesome` copy is `docs/transpose/doxygen-awesome.css` + `-darkmode-toggle.js`
  (3004 lines). Not duplicated, but mis-referenced (see above) and the toggle JS is not wired in
  (`HTML_EXTRA_FILES` empty, `HTML_HEADER` empty).

### 3.2 Target state (recommendation)

One Doxygen run over `src/beman`, one output, one deploy. Concretely:

1. **One `docs/api/Doxyfile`** (or `docs/Doxyfile`, so the Makefile and workflow become true as written),
   `PROJECT_NAME = beman.typeclass`, `PROJECT_BRIEF = Typeclass Based Libraries for C++`,
   `INPUT = src/beman README.md docs/transpose/README.md docs/tree_algorithms/README.md docs/fingertree/README.md`
   (decision docs can be added as `@page`s deliberately; never `INPUT = docs`), `EXCLUDE_PATTERNS =
   */*.test.cpp */*_probe.test.cpp *.cppm`, `USE_MDFILE_AS_MAINPAGE = README.md`, `OUTPUT_DIRECTORY =
   build/docs` or `docs/api/html` (gitignored), `GENERATE_TREEVIEW = YES`, `HTML_EXTRA_STYLESHEET =
   docs/api/doxygen-awesome.css`, `HTML_EXTRA_FILES = docs/api/doxygen-awesome-darkmode-toggle.js` with a
   matching `HTML_HEADER`. Drop `CLANG_ASSISTED_PARSING` (or point `CLANG_DATABASE_PATH` at the preset
   build dir): the headers are self-contained and the parse adds a build dependency to a docs job.
2. **Per-library groups, plus one cross-cutting group.** Add `@defgroup transpose`, `@defgroup
   tree_algorithms`, `@defgroup fingertree` in one `docs/api/groups.dox`, and a fourth `@defgroup
   typeclass` ("the typeclass-object mechanism") holding `functor.hpp`, `apply.hpp`, `traverse.hpp`,
   `fold.hpp`, `monoid.hpp`, `overloaded.hpp`, `detail/typeclass_base.hpp` — the thing being sold
   independent of any one paper. Each header gets a `@file … @ingroup <lib>` block. Group names should be
   the library slugs; since namespaces already differ, symbol pages cannot collide, and group/page ids
   only collide if two libraries pick the same `@page` id — adopt the rule "page ids are prefixed with
   the library slug" (`transpose-lookup-modes`, `fingertree-measures`) to match the slug discipline in
   `decisions.md`.
3. **Keep specgen text out of Doxygen.** Two options: `INPUT_FILTER = scripts/strip-specgen.sh` that
   deletes `//!` lines (and `/*! */` blocks) before Doxygen sees them; or `ALIASES` mapping
   `seebelow`, `omit`, `expos`, `rSec3`, `ref`, `mandates`, `remarks`→(already a Doxygen command) so the
   wording renders as a labelled "Specification" paragraph. The first is simpler and keeps the two
   audiences separate; the second is how to get one page that shows both design rationale and the
   normative text, which may be exactly the "coherent, integrated" product wanted. Either way, record the
   choice in `wording-pipeline.md` and fix the claim at `:84`.
4. **Normalise the dialect** across libraries: either `///` everywhere or `/** */` everywhere, and
   `@brief` on every public entity (currently 1 in transpose, 0 in tree_algorithms, 4 in fingertree).
   With `EXTRACT_ALL = YES` the pages exist regardless; without briefs the index is a list of signatures.
5. **One deploy path.** `make docs` → `doxygen docs/api/Doxyfile`; `doxygen-gh-pages.yml` uses the same
   file and `folder: build/docs/html` (or wherever `OUTPUT_DIRECTORY` says); delete
   `docs/fingertree/Doxyfile` and `docs/transpose/mrdocs.yml` (or rewrite the latter for
   `beman::transpose::**`, `beman::tree_algorithms::**`, `beman::fingertree::**` with
   `implementation-defined: ['beman::*::detail']` if MrDocs is still wanted — it supports multiple
   `include-symbols` patterns, so one `mrdocs.yml` covers all three).
6. Consider a `BEMAN_TYPECLASS_BUILD_DOCS` CMake option with `find_package(Doxygen)` +
   `doxygen_add_docs(docs …)` so the docs build reuses the header set lists and the Makefile `docs`
   target is `cmake --build --target docs`. Optional; the Doxyfile route is fine if it is one file.

---

## 4. Paper tooling

### 4.1 How the papers build (verified by reading; pandoc 3.9 not available here)

`papers/Makefile:14` includes the vendored `wg21/flat.mk`; `flat.mk:54` makes every `papers/*.md`
except `CHANGELOG/LICENSE/README` a paper, so `D3200R0.md` and `algorithms-for-trees.md` both build with
`make -C papers {html,pdf,latex}`. `base.mk` pins pandoc 3.9.0.2 (`:8`) and a Python venv under
`papers/wg21/deps/` (gitignored by `papers/wg21/.gitignore`). `papers/defaults.yaml` is picked up
automatically (`flat.mk:48`) and appends three filters — `transclude.py`, `bibliography-header.py`,
a second `wg21.py` pass — for **both** papers. `papers/Makefile:20-21` adds `wording/.stamp` and
`filters/transclude.py` as prerequisites of the D3200R0 outputs only. `D3200R0.md:611-709` contains 29
`::: {.include file="wording/transpose.*.md"}` divs; `algorithms-for-trees.md` has none.

Root `Makefile:228-230`: `papers: wording` then `$(MAKE) -C papers pdf`. `wording` runs
`scripts/gen-wording.sh`, which exits 1 if `specgen` is not on PATH (`gen-wording.sh:69-72`). So
`make papers` **cannot build the papers without specgen**, contradicting `docs/transpose/wording-pipeline.md:21-22`
("The generated fragments are checked in, so the paper builds without either"). Fix: make `papers`
depend on nothing, and have `wording-check` be the drift gate (ideally in CI, see 2.2).

**Contradiction (minor, docs).** `papers/defaults.yaml:9-13` says a pandoc defaults file *appends* to a
list-valued key ("verified against the pinned pandoc 3.9.0.2 … restating the whole chain ran citeproc a
second time"), and the file lists only three filters. `docs/transpose/wording-pipeline.md:182-183` says
the opposite ("*replaces* a list-valued key … restates the whole filter chain"). The YAML is the one that
was verified against behaviour; the prose is stale.

### 4.2 Transclusion and example anchors (verified)

- `papers/filters/transclude.py` resolves `file=` against `$TRANSCLUDE_BASE` or cwd (`:30`), which
  `flat.mk` leaves at `papers/`. Nested includes to depth 4; a missing file is a hard `fail()`. It parses
  the included file as *markdown*, so it is a wording splicer, not a code splicer.
- Code anchors are UUID pairs `// <uuid>` / `// <uuid> end` (75 pairs across `examples/` and
  `include/beman/tree_algorithms/*.hpp`, 2 in `include/beman/transpose/functor.hpp`). **No UUID is defined in
  more than one source file** (checked across `examples/` and `src/`). Consumers: the tree_algorithms
  article (68 references, `docs/tree_algorithms/recursive-tree-algorithms.org`), the blog posts, the
  essay, `docs/tree_algorithms/notes/anchor-inventory.md`, and `papers/algorithms-for-trees.md:200-224`
  (11 references, as a *comment* beside a hand-maintained synopsis).
- `examples/tree_algorithms/CMakeLists.txt:33-60` `paper_snippets_check` (in `ALL`) gates the anchored
  tree_algorithms files; transpose's anchored examples are built as examples and registered as tests
  (`examples/transpose/CMakeLists.txt:25-36`), so both libraries' anchored code is compiled on every
  build. Good.
- `papers/algorithms-for-trees.md:200-205` says "Markdown has no equivalent mechanism available in this
  build" and keeps the synopsis by hand, last diffed 2026-07-06 (`:224`); its anchor list cites
  `include/beman/tree_algorithms/...` paths that no longer exist and `fixpoint/main/papers/wg21`. Since
  the merge, `transclude.py` *does* exist in this build; a 30-line `{.include-code file= anchor=}` variant
  that slices between a UUID pair and wraps in a `cpp` fence would make the D4322 synopsis generated like
  D3200's wording, and the "Checked against: manually diffed" note could go.

### 4.3 Wording pipeline: transpose-only, and how a second paper's wording should live

**Verified.** `scripts/gen-wording.sh:40` `MANIFEST="transpose.manifest"`, `:45-54` an eight-entry
`HEADERS` array of `src/beman/transpose/*.hpp` with `transpose.*` stems, `:87` the hard-coded
`src/beman/transpose/` directory, output `papers/wording/` flat (`:26`). `make wording-check` diffs the
whole directory (`Makefile:217-221`). Everything in `papers/wording/` is `transpose.*` (29 fragments +
manifest). The paper's include order is reconciled against the single manifest.

Proposed layout for three papers:

```
papers/wording/
  transpose/          transpose.manifest, transpose.<clause>.<item>.md   (D3200)
  tree_algorithms/    tree_algorithms.manifest, tree.<clause>.<item>.md  (D4322)
  fingertree/         fingertree.manifest, fingertree.<clause>.<item>.md (D4325)
  <lib>.headers       one line per `header.hpp:stable.stem`, replacing the HEADERS array
```

- `scripts/gen-wording.sh <lib> [out-dir]`: reads `papers/wording/<lib>.headers`, parses
  `src/beman/<lib>/<header>`, renders once per library with `--split .` inside `papers/wording/<lib>/`,
  writes `<lib>.manifest` and `.stamp`. One render per paper keeps `--validate` scoped to that paper
  (the reason the script gives at `:10-16` for rendering a paper's headers together) while preventing
  `--split`'s "never deletes files left by an earlier run" (`:18-20`) from letting one library's stale
  fragments survive in another's directory.
- `papers/Makefile`: one stamp rule per paper — `$(OUTDIR)/D3200R0.%: wording/transpose/.stamp`,
  `$(OUTDIR)/algorithms-for-trees.%: wording/tree_algorithms/.stamp`, etc. Include divs become
  `::: {.include file="wording/tree_algorithms/tree.fold.syn.md"}`.
- Root `Makefile`: `wording` and `wording-check` loop over `transpose tree_algorithms fingertree`, or take
  `LIB=`; `wording-check` diffs per directory so a failure names the paper.
- Stable-name stems must stay globally distinct and must not shadow draft stable names: `transpose.*`
  is fine; for tree_algorithms prefer `tree.alg.*`/`tree.fold.*` over anything the draft already uses
  (`alg.*` is the draft's algorithms clause); fingertree `fingertree.*`.
- The `//!` markup rules in `wording-pipeline.md:81-166` are transpose's rules but library-agnostic;
  move the document to `docs/wording-pipeline.md` (or `papers/README.md`) so it is not filed under one
  library, and keep the per-header table (`:36-47`) per library.
- Note the second paper's wording must be generated from headers that today carry *no* `//!` markup
  (0 lines in tree_algorithms and fingertree). That is the real cost; the layout is the easy part.

### 4.4 Blog export (verified by reading; emacs not available here)

- `papers/blog/Makefile` exports `.org → .html/.md` via `.emacs.d/init.el` (installs packages from MELPA
  on first run) and `blog-export.el`. The "running code" posts execute binaries from
  `$(ROOT)/.build/build-gcc-16/examples/transpose/RelWithDebInfo` (`:26`, `:48`) — a hard-coded
  Makefile-workflow path; a preset build cannot feed them.
- **Owner-local absolute paths baked into content (significant).** Transclusion links use
  `orgit-file:~/src/typeclass/typeclass::<tag>::<path>::<uuid>` in `papers/blog/transpose-at-work.org:38,61,78`,
  `traverse-does-all-the-work.org:39,41,57`, `adapting-a-type-to-a-typeclass.org:49,64,122,192`,
  `graded-gadgets.org` (10 refs), and the essay `docs/coordination/essay/trees-and-transposition-for-29.org:107-161`.
  `orgit-file-transclusion.el:83,103` does `git -C (expand-file-name REPO)`, so the export works only on a
  machine whose checkout is at `~/src/typeclass/typeclass`. The commit that "repointed" these
  (`9d8fa1c`) changed the REV/PATH segments but kept the REPO segment. Fix in the adapter: treat a REPO
  of `.`/empty as "this repository" (`git rev-parse --show-toplevel` of the .org file's directory) and
  rewrite the links once.
- `pins.md` (the post → tag table) is cited by `.emacs.d/init.el:186`, `orgit-file-transclusion.el:17`,
  `papers/blog/Makefile:8`, `papers/blog/transpose-over-real-senders.org:30` and **does not exist**
  anywhere in the tree. The tags themselves exist on the remote (16 refs, e.g.
  `transpose/blog/graded-gadgets`, `fingertree/blog/trees-and-transposition-for-29`; `git ls-remote --tags`),
  consistent with `README.md:27`. This shallow clone has none, so a fresh `--depth` clone cannot export the
  blog; document `git fetch --tags`.
- **The two CSS files are orphaned (minor).** `modus-operandi-tinted.css` (5776 lines) and
  `modus-vivendi-tinted.css` (5769 lines) are referenced by no `.org`, `.el` or Makefile in the repo
  (`grep -rn 'modus-\|\.css' papers/blog` → nothing). Either they are the sdowney.org theme and belong
  with the publishing config outside this repo, or `blog-export.el` should set `org-html-head` to one of
  them so the local HTML looks like the published post. 11.5k lines of unreferenced CSS is the kind of
  thing the end-of-month cleanup should decide on.
- Generated exports are half-committed: nine `.md` exports are tracked (`papers/blog/*.md`) while
  `.html` is ignored (`.gitignore:38`); `transpose-over-real-senders.org` has no `.md`; the essay's
  `.html` *is* committed (`docs/coordination/essay/trees-and-transposition-for-29.html`) because the
  ignore pattern is `/papers/blog/*.html` only. Pick one rule.
- `.emacs.d/init.el:158` hard-codes `file:////home/sdowney/bld/reveal.js`; `:130-146` hard-code
  `/usr/share/plantuml`/`ditaa` jars. `use-package org :after (flycheck)` (`:49`) never fires in batch,
  which `blog-export.el:20-31` explicitly works around — the init file is a personal config pressed into
  service as a build tool.

### 4.5 Bibliographies (verified)

Three sources of references:

| file | keys |
|---|---|
| `papers/blog/references.bib` | 11: hinze2006fingertrees, mcbride2008applicative, gibbons2006iterator, katsumata2014parametric, mcnamara2000fcpp, wadler1989adhoc, downey2026treesresearch, milewski2013/2017falgebras, niebler2013falgebras, stroustrup2009concepts |
| `docs/tree_algorithms/references.bib` | 5: mcbride2008applicative, gibbons2006iterator, milewski2013/2017falgebras, niebler2013falgebras — all also in the blog file; `milewski2017falgebras` has a longer `note` here than there |
| `papers/D3200R0.md:17-…` | inline YAML `references:` (mcbride-paterson, ghc-applicative, …) with different ids (`mcbride-paterson` vs `mcbride2008applicative`) |

Recommendation: one `papers/references.bib` (CSL-JSON or BibTeX) with one id scheme; the blog and the
org article point `#+bibliography:` at it; the papers get it through `defaults.yaml`
(`bibliography: [references.bib]` appends to `doc.yaml`'s `csl.json`, per the append semantics
`defaults.yaml` already relies on). Keep WG21 paper citations on MPark's `csl.json`.

---

## 5. Leftovers from the merge

Grouped; each is verified unless marked.

### 5.1 Standalone-repo artefacts that now lie

- **Per-library READMEs carry the retired repos' badges and build text.** `docs/transpose/README.md:7-9`,
  `docs/tree_algorithms/README.md:9-11`, `docs/fingertree/README.md:9-11` link CI/lint/coveralls for
  `bemanproject/{transpose,tree_algorithms,fingertree}` (repos that were never there and are retired).
  `docs/transpose/README.md:59` "Headers live under `include/beman/transpose/`" (and the list names
  `dual_monoid.hpp`, which is a struct in `monoid.hpp:208`, not a header, while omitting `overloaded.hpp`,
  `induced_monoid.hpp`, `simd.hpp`); `:92` "selects `etc/gcc-16-toolchain.cmake`" (it is `cmake/`).
  `docs/fingertree/README.md` is mostly exemplar scaffolding: `:15` "(... TODO: description)", `:17`
  "Implements `std::todo` … PnnnnRr", `:30` "TODO", `:42` GoogleTest, `:44` `BEMAN_FINGERTREE_BUILD_TESTS`
  (no such option), `:48-58` a GCC 11+/C++17 platform table, `:107` `vcpkg install beman-fingertree` (the
  port is `beman-typeclass`). The root `README.md` is good and should be the one source of build truth;
  the per-library READMEs should shrink to "what this library is + design notes index" and link up.
- **`CONTRIBUTING.md`** is exemplar text: GoogleTest ×3 (`:40,68,92`), C++17 example (`:76`),
  nonexistent `BEMAN_TYPECLASS_INSTALL_CONFIG_FILE_PACKAGE` / `find_package(beman.typeclass)` (`:131-137`).
- **`.copier-answers.yml`** records `paper: P3200R0` and a single-library template; a `copier update`
  would try to regenerate the single-library layout. Fine to keep, but note it.
- **Makefile submodule remnants**: `Makefile:28-32,96` make every first configure run
  `git submodule update --init --recursive` and touch `.update-submodules` although there is no
  `.gitmodules` (infra is a subtree, `Makefile:344-362`). Harmless; dead.
- `vcpkg-configuration.json`, `port/vcpkg.json.in:5` (`homepage: bemanproject/typeclass`),
  `port/portfile.cmake.in:4` (`REPO bemanproject/typeclass`) and the README badges (`README.md:9-10`)
  name `bemanproject/typeclass`; the remote is `steve-downey/typeclass` (`git remote -v`). If a transfer
  is planned, say so in the README (the tree_algorithms punch list made the same point at
  `punch-list.md:25-27`).

### 5.2 Stale paths

- `include/` in code comments: `tests/beman/transpose/laws.hpp:18`, `examples/transpose/p2300_adapter.hpp:10,17`,
  `tests/beman/transpose/collect_hook.test.cpp:118`, `execution_probe.test.cpp:35`,
  `src/beman/transpose/CMakeLists.txt:107`; `.claude/skills/tree-proposals-coherence/SKILL.md:161`;
  `docs/fingertree/compile-time-design.md:13`, `measure-based-splitting.md:88`;
  `docs/transpose/typeclass-object-pattern.md:32,42,68,88,123`; `docs/tree_algorithms/DECISIONS.md:137`,
  `notes/*` (many; those are transactional and will go). `cmake/gcovr.cfg.in:10`.
- `~/src/...` sibling checkouts: `docs/coordination/README.md:10-11,34`, `docs/coordination/CLAUDE.md:21`,
  `SKILL.md:27-28`, `docs/transpose/provenance.md:13`. fixpoint and trees are genuinely external, so a
  reference is legitimate — but as a *name* ("the fixpoint repository"), not a path on one machine.
- `docs/transpose/wording-pipeline.md:206-215` tells you to `git subtree pull --prefix=papers/wg21`;
  `papers/filters/transclude.py:3` header still says `papers/wg21/filters/transclude.py`.

### 5.3 Duplicated LICENSE/CONTRIBUTING material

`LICENSE` and `infra/LICENSE` are byte-identical Apache-2.0 (vendored; acceptable, mark `infra/**`
vendored — already done in `.gitattributes:1`). `papers/wg21/LICENSE.md` is upstream's (Boost).
`docs/fingertree/LICENSE.CC-BY-SA-4.0` is deliberate and announced in `README.md:49-50`. No duplicated
CONTRIBUTING fragments beyond the exemplar text above. Three `README.md`s under `docs/<lib>/` each
restate the maturity badge/status/license paragraphs that the root README also has.

### 5.4 `.gitignore` gaps

Checked with `git check-ignore`: **not** ignored — `docs/adoc/` (mrdocs output, `Makefile:327`),
`coverage.json` (gcovr `coveralls = coverage.json` in `cmake/gcovr.cfg.in:11` is relative to the
working directory), `.emacs.d/custom.el` (`init.el:2` writes it), `docs/coordination/essay/*.html`
(while `papers/blog/*.html` is). Ignored correctly: `/build`, `/.build`, `/.install`, `docs/html`,
`papers/generated`, `papers/wording/.stamp`, `installtest/*/.build`, `.emacs.d/elpa*`, `*.deps`.

### 5.5 Anything that still thinks it is a standalone repo

In addition to the above: `docs/transpose/Doxyfile`, `docs/fingertree/Doxyfile`, `docs/transpose/mrdocs.yml`
(§3); `installtest/transpose/CMakeLists.txt` (§1.2); `cmake/` as a whole (§1.6); the `BEMAN_FINGERTREE_*`
/ `BEMAN_TREE_ALGORITHMS_*` variables that mirror the project option (`src/beman/fingertree/CMakeLists.txt:7`,
`src/beman/tree_algorithms/CMakeLists.txt:5`) — fine as internal aliases, but `examples/` and `benchmarks/`
use the project option directly, so the aliases buy nothing.

---

## Summary table of findings by severity

| # | finding | severity |
|---|---|---|
| 3.1 | No working API-docs configuration; gh-pages workflow fails every push; `//!` wording would leak into Doxygen | blocking (for the stated docs goal) |
| 1.2 | `installtest` covers transpose only and is broken by relocation (`Makefile:335`, `installtest/transpose/CMakeLists.txt:11-14`) | significant |
| 1.5 | fingertree probe hard-errors on compilers without a C++26 dialect; "only libstdc++ 16" premise doubtful | significant |
| 1.6 | Two toolchain trees; `cmake/use-fetch-content.cmake` is a stale fork; dead `cmake/Config.cmake.in`; gcovr filter never matches | significant |
| 1.4 | Modules: one library, C++23 Debug only, `fingertree.cppm` dead, mixed gating variables | significant |
| 2.2 | CI never runs `wording-check`, paper/blog builds, install test, or the vendored wg21 tests | significant |
| 4.1 | `make papers` requires specgen despite checked-in fragments; `defaults.yaml` vs `wording-pipeline.md` contradiction | significant (docs) / minor (build) |
| 4.3 | Wording pipeline hard-wired to transpose; layout for three papers proposed | significant (design) |
| 4.4 | `~/src/typeclass/typeclass` baked into transclusion links; `pins.md` missing; orphaned 11.5k-line CSS; half-committed exports | significant |
| 5.1 | Per-library READMEs/CONTRIBUTING are standalone/exemplar text with false claims | significant (docs) |
| 1.3 | Dead `USE_MODULES` config macros in transpose/fingertree | minor |
| 2.1 | preset-test duplicates matrix on `:latest` images at C++26 for all presets (appleclang/msvc comment conflict) | minor (inferred) |
| 2.5 | Four divergent `.clang-format`; codespell excludes `papers/`; formatter version skew | minor |
| 4.5 | Three bibliographies, overlapping keys, one divergent entry | minor |
| 5.2–5.4 | Stale `include/`, `~/src`, `etc/` paths; `.gitignore` gaps; submodule remnants | minor |
| 2.3 | Vendored `.github/workflows` under `infra/` and `papers/wg21/` are inert | note |
| 1.1 | Target/alias/package naming is consistent; `tree_algorithms` and `fingertree` genuinely depend on `beman::transpose`; explicit-tier isolation check works (compiled under clang 18) | note (positive) |
