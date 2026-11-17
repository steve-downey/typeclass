#!/bin/bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Compile and run the layer-coherence review probes. Run from anywhere inside
# the worktree, after a GCC 16 build has generated the config headers.
#
#   CXX            GCC driver                     (default g++-16)
#   CLANGXX        Clang driver                   (default clang++-22)
#   BUILD_INCLUDE  directory of generated headers (default .build/build-gcc-16/include)

set -u
cd "$(git rev-parse --show-toplevel)" || exit 1

here=docs/coordination/review/2026-11-16-layer-coherence/probes
build_include=${BUILD_INCLUDE:-.build/build-gcc-16/include}
gcc=("${CXX:-g++-16}" -std=gnu++26 -Iinclude "-I$build_include" -Wall -Wextra)
clang=("${CLANGXX:-clang++-22}" -std=c++26 -Iinclude "-I$build_include")
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

# First few diagnostics of a compile that is expected to fail.
errors() { grep -E "error|required from here" | head -4 | cut -c1-320; }

# Compile with GCC and run; print the diagnostics instead if it fails.
run() {
    local source=$1
    shift
    if "${gcc[@]}" "$@" "$here/$source" -o "$out/probe" 2>"$out/log"; then
        "$out/probe"
    else
        errors <"$out/log"
    fi
    rm -f "$out/probe"
}

echo "== name-clash: both namespaces in scope"
"${gcc[@]}" -fsyntax-only "$here/name-clash.cpp" 2>&1 | errors

for which in 0 1; do
    echo "== instance-placement WHICH=$which: type header only (rc 0 means no instance visible)"
    "${gcc[@]}" "-DWHICH=$which" -fsyntax-only "$here/instance-placement.cpp"
    echo "   rc=$?"
    echo "== instance-placement WHICH=$which LATE: instance header after first use, gcc"
    "${gcc[@]}" "-DWHICH=$which" -DLATE -fsyntax-only "$here/instance-placement.cpp" 2>&1 | errors
    echo "== instance-placement WHICH=$which LATE: instance header after first use, clang"
    if "${clang[@]}" "-DWHICH=$which" -DLATE "$here/instance-placement.cpp" -o "$out/probe" 2>"$out/log"; then
        "$out/probe"
    else
        errors <"$out/log"
    fi
    rm -f "$out/probe"
done

for probe in 0 1 2; do
    echo "== lookup-hard-error PROBE=$probe"
    "${gcc[@]}" "-DPROBE=$probe" -fsyntax-only "$here/lookup-hard-error.cpp" 2>&1 | errors
    echo "   rc=${PIPESTATUS[0]}"
done

for structure in 0 1 2; do
    echo "== collect-carrier WHICH_S=$structure"
    run collect-carrier.cpp "-DWHICH_S=$structure"
done

echo "== fold-copies"
run fold-copies.cpp

echo "== fold-cost"
run fold-cost.cpp -O2

echo "== endo-map"
run endo-map.cpp

echo "== fingertree-with-tree-algorithms"
run fingertree-with-tree-algorithms.cpp
