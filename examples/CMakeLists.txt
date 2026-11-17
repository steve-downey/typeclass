# examples/CMakeLists.txt                                            -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_subdirectory(transpose)
add_subdirectory(tree_algorithms)
if(BEMAN_TYPECLASS_BUILD_FINGERTREE)
    add_subdirectory(fingertree)
endif()
if(TARGET beman.typeclass_tree_adapters)
    add_subdirectory(typeclass_tree_adapters)
endif()
