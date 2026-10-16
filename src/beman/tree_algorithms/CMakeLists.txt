# src/beman/tree_algorithms/CMakeLists.txt                           -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(BEMAN_TREE_ALGORITHMS_USE_MODULES ${BEMAN_TYPECLASS_USE_MODULES})

configure_file(
    "${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/config_generated.hpp.in"
    "${PROJECT_BINARY_DIR}/include/beman/tree_algorithms/config_generated.hpp"
    @ONLY
)

if(BEMAN_TREE_ALGORITHMS_USE_MODULES)
    add_library(beman.tree_algorithms STATIC)
    target_sources(
        beman.tree_algorithms
        PUBLIC FILE_SET CXX_MODULES FILES tree_algorithms.cppm
    )
    set_target_properties(beman.tree_algorithms PROPERTIES CXX_MODULE_STD ON)
    target_compile_features(beman.tree_algorithms PUBLIC cxx_std_23)
    set(_scope PUBLIC)
else()
    add_library(beman.tree_algorithms INTERFACE)
    target_compile_features(beman.tree_algorithms INTERFACE cxx_std_23)
    set_target_properties(
        beman.tree_algorithms
        PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
    )
    set(_scope INTERFACE)
endif()
add_library(beman::tree_algorithms ALIAS beman.tree_algorithms)

target_sources(
    beman.tree_algorithms
    ${_scope}
    FILE_SET HEADERS
    BASE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/include"
    FILES
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/binary_tree.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/box.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/child_slot.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/config.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/expression.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/fix.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/fold_map.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/fold_map_lookup.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/fringe_tree.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/functors.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/recursion_schemes.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/recursion_schemes_lookup.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/rose_tree.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/tree_algorithms.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/tree_algorithms/todo.hpp
    "${PROJECT_BINARY_DIR}/include/beman/tree_algorithms/config_generated.hpp"
)

# The lookup tier (functor.hpp, recursion_schemes_lookup.hpp, the example
# trees) consumes beman.transpose; the explicit tier does not.
target_link_libraries(beman.tree_algorithms ${_scope} beman::transpose)

beman_install_library(
    beman.tree_algorithms
    TARGETS beman.tree_algorithms
    DEPENDENCIES "beman.transpose"
)
