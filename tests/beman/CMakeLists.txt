# tests/beman/CMakeLists.txt                                       -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Each library's tests, in the libraries' layering order. fingertree's are
# built only where the library itself is; src/beman/CMakeLists.txt decides.
add_subdirectory(transpose)
add_subdirectory(tree_algorithms)
if(TARGET beman.fingertree)
    add_subdirectory(fingertree)
endif()
