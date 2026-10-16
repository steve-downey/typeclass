# src/beman/fingertree/CMakeLists.txt                                -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# The finger-tree headers include many standard headers, which conflict with
# `import std` in the module interface unit, so the C++ module cannot be built
# yet; fingertree stays a header library even when the others are modules.
set(BEMAN_FINGERTREE_USE_MODULES OFF)

configure_file(
    "${PROJECT_SOURCE_DIR}/include/beman/fingertree/config_generated.hpp.in"
    "${PROJECT_BINARY_DIR}/include/beman/fingertree/config_generated.hpp"
    @ONLY
)

if(BEMAN_FINGERTREE_USE_MODULES)
    add_library(beman.fingertree STATIC)
    target_sources(
        beman.fingertree
        PUBLIC FILE_SET CXX_MODULES FILES fingertree.cppm
    )
    set_target_properties(beman.fingertree PROPERTIES CXX_MODULE_STD ON)
    target_compile_features(beman.fingertree PUBLIC cxx_std_26)
    set(_scope PUBLIC)
else()
    add_library(beman.fingertree INTERFACE)
    target_compile_features(beman.fingertree INTERFACE cxx_std_26)
    set_target_properties(
        beman.fingertree
        PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
    )
    set(_scope INTERFACE)
endif()
add_library(beman::fingertree ALIAS beman.fingertree)

# finger_tree requires std::inplace_vector (C++26); no fallback exists.
#
# Clang-only warning suppressions applied in our build tree (not exported to
# consumers). Both are spurious for this code base:
#  * -Wdeprecated-declarations: libstdc++ 16's <inplace_vector> internally uses
#    std::is_trivial_v, deprecated in C++26; Clang surfaces it from our
#    inplace_vector instantiation. GCC does not warn.
#  * -Wmissing-braces: a low-value, false-positive-prone diagnostic that fires
#    (instantiation-dependent) on the aggregate Elem initialization.
target_compile_options(
    beman.fingertree
    ${_scope}
    $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:Clang>:-Wno-deprecated-declarations;-Wno-missing-braces>>
)

target_sources(
    beman.fingertree
    ${_scope}
    FILE_SET HEADERS
    BASE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/include"
    FILES
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/config.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/fingertree.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_iterator.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_pmr.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_foldable.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_traversable.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_random_access.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_rope.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_priority_queue.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/fingertree/tree/finger_tree_interval_index.hpp
    "${PROJECT_BINARY_DIR}/include/beman/fingertree/config_generated.hpp"
)

target_link_libraries(beman.fingertree ${_scope} beman::transpose)

beman_install_library(
    beman.fingertree
    TARGETS beman.fingertree
    DEPENDENCIES "beman.transpose"
)
