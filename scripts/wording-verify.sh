#!/usr/bin/env bash
# scripts/wording-verify.sh                                           -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Checks, without specgen, that the checked-in wording fragments are the ones
# the current headers generate.
#
# gen-wording.sh records what it ran on and what it wrote: the SHA-256 of
# every input (the headers and the generator script, as paths from the
# repository root) in <stem>.inputs.sha256, and of every output (the fragments
# and the manifest, as bare names) in <stem>.outputs.sha256. This script
# recomputes both. A changed input means the fragments are stale: a header was
# edited after the wording was generated. A changed output means a fragment
# was edited by hand, or left over from an earlier render. Either way the
# answer is `make wording` on a machine with specgen, then commit the result.
#
# Usage: scripts/wording-verify.sh [wording-directory]

set -o nounset
set -o errexit
set -o pipefail

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
WORDING_DIR="${1:-$REPO_ROOT/papers/wording}"
readonly STEM=transpose

readonly INPUTS="$WORDING_DIR/$STEM.inputs.sha256"
readonly OUTPUTS="$WORDING_DIR/$STEM.outputs.sha256"

fail() {
    echo "wording-verify: $*" >&2
    exit 1
}

[[ -f "$INPUTS" && -f "$OUTPUTS" ]] \
    || fail "no generation record in $WORDING_DIR; run 'make wording' where specgen is installed and commit papers/wording"

(cd "$REPO_ROOT" && sha256sum --check --quiet --strict "$INPUTS") \
    || fail "a header or the generator changed after the wording was generated; run 'make wording' and commit the result"

(cd "$WORDING_DIR" && sha256sum --check --quiet --strict "$OUTPUTS") \
    || fail "a fragment differs from what the generator wrote; run 'make wording' and commit the result"

# Fragments present but unrecorded are leftovers from an earlier render, which
# `--split` never deletes; gen-wording.sh clears them, a stray copy would not.
unrecorded=$(
    cd "$WORDING_DIR"
    for f in "$STEM".*.md "$STEM".manifest; do
        [[ -e "$f" ]] || continue
        grep -q -- " $f\$" "$OUTPUTS" || echo "$f"
    done
)
[[ -z "$unrecorded" ]] \
    || fail "fragments not written by the recorded render:"$'\n'"$unrecorded"

echo "wording-verify: $WORDING_DIR matches the headers it was generated from"
