# src/beman/tree_algorithms/CMakeLists.txt                           -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(BEMAN_TREE_ALGORITHMS_USE_MODULES ${BEMAN_TYPECLASS_USE_MODULES})

configure_file(
    config_generated.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/config_generated.hpp"
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
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_BINARY_DIR}/src"
    FILES
    binary_tree.hpp
    box.hpp
    child_slot.hpp
    config.hpp
    expression.hpp
    fix.hpp
    fold_map.hpp
    fold_map_lookup.hpp
    fringe_tree.hpp
    functors.hpp
    recursion_schemes.hpp
    recursion_schemes_lookup.hpp
    rose_tree.hpp
    tree_algorithms.hpp
    todo.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/config_generated.hpp"
)

# The lookup tier (functor.hpp, recursion_schemes_lookup.hpp, the example
# trees) consumes beman.transpose; the explicit tier does not.
target_link_libraries(beman.tree_algorithms ${_scope} beman::transpose)

beman_install_library(
    beman.tree_algorithms
    TARGETS beman.tree_algorithms
    DEPENDENCIES "beman.transpose"
)

if(BEMAN_TYPECLASS_BUILD_TESTS)
    # Catch2-based unit tests: one executable per component.
    foreach(
        test
        IN
        ITEMS
            todo
            box
            child_slot
            fix
            recursion_schemes
            recursion_schemes_lookup
            expression
            functors
            binary_tree
            fold_map
            fringe_tree
            rose_tree
    )
        add_executable(beman.tree_algorithms.tests.${test})
        target_sources(
            beman.tree_algorithms.tests.${test}
            PRIVATE ${test}.test.cpp
        )
        target_link_libraries(
            beman.tree_algorithms.tests.${test}
            PRIVATE beman::tree_algorithms Catch2::Catch2WithMain
        )
        if(BEMAN_TREE_ALGORITHMS_USE_MODULES)
            set_target_properties(
                beman.tree_algorithms.tests.${test}
                PROPERTIES CXX_MODULE_STD ON
            )
        endif()
        catch_discover_tests(beman.tree_algorithms.tests.${test})
    endforeach()

    # The explicit tier is the surface that stays valid if P3200 does not
    # progress, so it must include nothing from beman.transpose (DECISIONS.md,
    # amendment 2026-10-12). One translation unit per explicit-tier header
    # includes that header alone, then fails with #error if the include guard
    # of any beman.transpose header is defined. Building them is the test.
    # A module build imports everything, so there is nothing to check there.
    if(NOT BEMAN_TREE_ALGORITHMS_USE_MODULES)
        get_target_property(_transpose_dir beman.transpose SOURCE_DIR)
        get_target_property(_transpose_headers beman.transpose HEADER_SET)
        set(_guard_checks "")
        foreach(_header IN LISTS _transpose_headers)
            cmake_path(ABSOLUTE_PATH _header BASE_DIRECTORY "${_transpose_dir}")
            file(
                STRINGS "${_header}"
                _guard
                REGEX "^#ifndef BEMAN_TRANSPOSE_"
                LIMIT_COUNT 1
            )
            if(_guard)
                string(REPLACE "#ifndef " "" _guard "${_guard}")
                string(
                    APPEND _guard_checks
                    "#ifdef ${_guard}\n#error \"includes ${_header}\"\n#endif\n"
                )
            endif()
        endforeach()
        add_library(beman.tree_algorithms.tests.explicit_tier_isolation OBJECT)
        foreach(
            header
            IN
            ITEMS fix box child_slot recursion_schemes fold_map
        )
            file(
                CONFIGURE
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/explicit_tier_${header}.cpp"
                CONTENT
                    "#include <beman/tree_algorithms/${header}.hpp>\n${_guard_checks}"
            )
            target_sources(
                beman.tree_algorithms.tests.explicit_tier_isolation
                PRIVATE
                    "${CMAKE_CURRENT_BINARY_DIR}/explicit_tier_${header}.cpp"
            )
        endforeach()
        target_link_libraries(
            beman.tree_algorithms.tests.explicit_tier_isolation
            PRIVATE beman::tree_algorithms
        )
    endif()
endif()
