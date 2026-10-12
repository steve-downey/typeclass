# src/beman/CMakeLists.txt                                           -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Order is layering order: later libraries may build on earlier ones.
add_subdirectory(transpose)
add_subdirectory(tree_algorithms)
add_subdirectory(fingertree)
