# tests/beman/tree_algorithms/CMakeLists.txt                      -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# The library decides whether it is built as a module (src/beman/tree_algorithms/CMakeLists.txt);
# its tests follow it.
get_target_property(
    BEMAN_TREE_ALGORITHMS_USE_MODULES
    beman.tree_algorithms
    CXX_MODULE_STD
)

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
    target_sources(beman.tree_algorithms.tests.${test} PRIVATE ${test}.test.cpp)
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
            PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/explicit_tier_${header}.cpp"
        )
    endforeach()
    target_link_libraries(
        beman.tree_algorithms.tests.explicit_tier_isolation
        PRIVATE beman::tree_algorithms
    )
endif()
