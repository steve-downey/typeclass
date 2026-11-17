# src/beman/typeclass_tree_adapters/CMakeLists.txt                   -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# The optional integration component: adapters that need two of the
# libraries at once. It depends on beman.tree_algorithms and
# beman.fingertree; neither of them links it or includes its headers, and
# neither depends on the other. A consumer opts in by linking this target.

add_library(beman.typeclass_tree_adapters INTERFACE)
add_library(beman::typeclass_tree_adapters ALIAS beman.typeclass_tree_adapters)

# The adapter is as new as the newer of the two libraries it joins.
target_compile_features(beman.typeclass_tree_adapters INTERFACE cxx_std_26)
set_target_properties(
    beman.typeclass_tree_adapters
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
)

target_sources(
    beman.typeclass_tree_adapters
    INTERFACE
        FILE_SET HEADERS
            BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/typeclass_tree_adapters/finger_tree.hpp
)

target_link_libraries(
    beman.typeclass_tree_adapters
    INTERFACE beman::tree_algorithms beman::fingertree
)

beman_install_library(
    beman.typeclass_tree_adapters
    TARGETS beman.typeclass_tree_adapters
    DEPENDENCIES "beman.tree_algorithms" "beman.fingertree"
)
