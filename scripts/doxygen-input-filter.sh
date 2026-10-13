#!/usr/bin/env bash
# scripts/doxygen-input-filter.sh                                     -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Doxygen input filter for the headers (docs/Doxyfile, FILTER_PATTERNS).
#
# `//!` lines are specgen markup: the normative wording the paper is generated
# from (docs/transpose/wording-pipeline.md). Doxygen reads `//!` as a doc
# comment too, so without this filter every documented entity would also carry
# its wording, with specgen's commands reported as unknown. Dropping the lines
# leaves the `///` and `/** */` comments, which are the API documentation.
#
# Doxygen invokes the filter with the file name as its one argument and reads
# the result from standard output.

set -o nounset
set -o errexit
set -o pipefail

sed -e '/^[[:space:]]*\/\/!/d' -- "$1"
