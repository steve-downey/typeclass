#!/usr/bin/env bash
# scripts/gen-wording.sh                                              -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Generates the P3200 wording fragments from the marked-up headers.
#
# Two phases. Each header is PARSED on its own -- beman.specgen reads only the
# main file's declaration/comment interleave -- and emits its document as IR;
# then one `render` takes all eight IR documents together. `--root` names each
# document's synopsis subclause, which is what keeps the root fragments
# distinct in the shared output directory.
#
# Rendering them together is what makes `--validate` see the paper rather than
# a header. The unit that has to be internally consistent is the paper: the
# applicative clause specifies `subsume` in terms of `grade_subsume`, which is
# specified over in `grade.hpp`, and a validator scoped to one header calls
# that a foreign name (steve-downey/specgen#109). Rendered together, each
# document validates against the union of all their documented names.
#
# `--split` never deletes files left by an earlier run, so the ordered
# manifest is written beside the fragments and is the record the paper's
# include order is reconciled against. One render writes one manifest, for
# the whole paper, in paper order.
#
# Usage: scripts/gen-wording.sh [output-directory]

set -o nounset
set -o errexit
set -o pipefail
IFS=$'\n\t'

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

SPECGEN="${SPECGEN:-specgen}"
GCC_TOOLCHAIN="${GCC_TOOLCHAIN:-}"
OUT_DIR="${1:-$REPO_ROOT/papers/wording}"

# One render, one manifest. Each header had its own while each was rendered
# on its own; the paper's include order is one order.
readonly MANIFEST="transpose.manifest"

# Headers to generate wording from, paired with the stable-name stem their
# subclauses live under. Add a header here once it carries specgen markup and
# validates clean.
readonly HEADERS=(
    "apply.hpp:transpose.applicative"
    "traverse.hpp:transpose.traversable"
    "transpose.hpp:transpose.alg"
    "sequence.hpp:transpose.range"
    "array.hpp:transpose.array"
    "grade.hpp:transpose.grade"
    "error_set.hpp:transpose.errset"
    "expected.hpp:transpose.expected"
)

# The specgen-supported front end is Clang 22 against a C++26 standard
# library. Flags are passed explicitly rather than through a compilation
# database, because headers are not translation units in one.
CLANG_ARGS=(
    -std=c++2c
    "-I$REPO_ROOT/src"
)
if [[ -n "$GCC_TOOLCHAIN" ]]; then
    CLANG_ARGS+=("--gcc-toolchain=$GCC_TOOLCHAIN")
fi
readonly -a CLANG_ARGS

if ! command -v "$SPECGEN" > /dev/null 2>&1; then
    echo "gen-wording: error: '$SPECGEN' not found; set SPECGEN to its path" >&2
    exit 1
fi

# The IR is an intermediate, not an artifact: it exists only to carry each
# header's document from its own parse into the one render that validates
# them against each other.
IR_DIR="$(mktemp -d)"
trap 'rm -rf "$IR_DIR"' EXIT

render_args=()
for entry in "${HEADERS[@]}"; do
    header="${entry%%:*}"
    stem="${entry#*:}"

    echo "gen-wording: $header -> $stem.*"
    "$SPECGEN" generate "$REPO_ROOT/src/beman/transpose/$header" \
        --emit-ir \
        --no-compile-commands \
        --output "$IR_DIR/$stem.ir.json" \
        -- "${CLANG_ARGS[@]}"

    render_args+=(--from-ir "$IR_DIR/$stem.ir.json" --root "$stem.syn")
done

# Render into a fresh directory, then replace what the previous render left.
# `--split` never deletes files, so rendering in place would keep a fragment
# whose subclause has since gone; replacing the whole set is what makes the
# output a function of the inputs. Run from the render directory with a
# relative `--split` so the manifest records `<stable name>.md` rather than a
# path particular to this machine.
RENDER_DIR="$IR_DIR/render"
mkdir -p "$RENDER_DIR"
echo "gen-wording: rendering ${#HEADERS[@]} documents as one paper"
(
    cd "$RENDER_DIR"
    "$SPECGEN" render "${render_args[@]}" \
        --backend mpark \
        --validate \
        --paper \
        --split . \
        > "$MANIFEST"
)

# The generation record. The paper build cannot depend on the headers through
# make -- a fresh checkout gives every file an arbitrary mtime -- so the
# dependency is recorded by content: the SHA-256 of each input, by path from
# the repository root, and of each output, by name. scripts/wording-verify.sh
# recomputes them without specgen; `make papers` runs it first.
(
    cd "$REPO_ROOT"
    inputs=(scripts/gen-wording.sh)
    for entry in "${HEADERS[@]}"; do
        inputs+=("src/beman/transpose/${entry%%:*}")
    done
    sha256sum "${inputs[@]}" > "$RENDER_DIR/transpose.inputs.sha256"
)
(
    cd "$RENDER_DIR"
    sha256sum transpose.*.md "$MANIFEST" > transpose.outputs.sha256
    {
        "$SPECGEN" --version 2> /dev/null || echo "specgen (version not reported)"
        echo "clang args: ${CLANG_ARGS[*]}"
    } > transpose.generator
)

mkdir -p "$OUT_DIR"
find "$OUT_DIR" -maxdepth 1 -type f -name 'transpose.*' -delete
cp "$RENDER_DIR"/transpose.* "$OUT_DIR/"

echo "gen-wording: wrote $OUT_DIR"
