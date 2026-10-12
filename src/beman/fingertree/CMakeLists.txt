# src/beman/fingertree/CMakeLists.txt                                -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# The finger-tree headers include many standard headers, which conflict with
# `import std` in the module interface unit, so the C++ module cannot be built
# yet; fingertree stays a header library even when the others are modules.
set(BEMAN_FINGERTREE_USE_MODULES OFF)

configure_file(
    config_generated.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/config_generated.hpp"
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
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_BINARY_DIR}/src"
    FILES
    config.hpp
    fingertree.hpp
    tree/finger_tree.hpp
    tree/finger_tree_iterator.hpp
    tree/finger_tree_pmr.hpp
    tree/finger_tree_foldable.hpp
    tree/finger_tree_traversable.hpp
    tree/finger_tree_random_access.hpp
    tree/finger_tree_rope.hpp
    tree/finger_tree_priority_queue.hpp
    tree/finger_tree_interval_index.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/config_generated.hpp"
)

target_link_libraries(beman.fingertree ${_scope} beman::transpose)

beman_install_library(
    beman.fingertree
    TARGETS beman.fingertree
    DEPENDENCIES "beman.transpose"
)

if(BEMAN_TYPECLASS_BUILD_TESTS)
    # Catch2-based unit tests: one executable per component.
    foreach(
        test
        IN
        ITEMS
            finger_tree
            finger_tree_iterator
            finger_tree_pmr
            finger_tree_foldable
            finger_tree_traversable
            finger_tree_random_access
            finger_tree_rope
            finger_tree_priority_queue
            finger_tree_interval_index
    )
        add_executable(beman.fingertree.tests.${test})
        target_sources(beman.fingertree.tests.${test} PRIVATE ${test}.test.cpp)
        target_link_libraries(
            beman.fingertree.tests.${test}
            PRIVATE beman::fingertree Catch2::Catch2WithMain
        )
        if(BEMAN_FINGERTREE_USE_MODULES)
            set_target_properties(
                beman.fingertree.tests.${test}
                PROPERTIES CXX_MODULE_STD ON
            )
        endif()
        catch_discover_tests(beman.fingertree.tests.${test})
    endforeach()

    # Self-enforcing proof: zero global heap allocations under a PMR resource.
    # Owns its own main(); returns non-zero on any violation.
    add_executable(beman.fingertree.tests.pmr_probe)
    target_sources(
        beman.fingertree.tests.pmr_probe
        PRIVATE finger_tree_pmr_probe.test.cpp
    )
    target_link_libraries(
        beman.fingertree.tests.pmr_probe
        PRIVATE beman::fingertree
    )
    if(BEMAN_FINGERTREE_USE_MODULES)
        set_target_properties(
            beman.fingertree.tests.pmr_probe
            PROPERTIES CXX_MODULE_STD ON
        )
    endif()
    add_test(
        NAME beman.fingertree.tests.pmr_probe
        COMMAND beman.fingertree.tests.pmr_probe
    )

    # Compile-only regression probe (no main()); building it is the test.
    add_library(
        beman.fingertree.tests.compile_probe
        OBJECT
        finger_tree_compile_probe.test.cpp
    )
    target_link_libraries(
        beman.fingertree.tests.compile_probe
        PRIVATE beman::fingertree
    )
    if(BEMAN_FINGERTREE_USE_MODULES)
        set_target_properties(
            beman.fingertree.tests.compile_probe
            PROPERTIES CXX_MODULE_STD ON
        )
    endif()
endif()
