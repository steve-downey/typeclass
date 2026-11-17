# tests/beman/typeclass_tree_adapters/CMakeLists.txt              -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Catch2-based unit tests: one executable per component.
foreach(test IN ITEMS finger_tree conformance)
    add_executable(beman.typeclass_tree_adapters.tests.${test})
    target_sources(
        beman.typeclass_tree_adapters.tests.${test}
        PRIVATE ${test}.test.cpp
    )
    target_link_libraries(
        beman.typeclass_tree_adapters.tests.${test}
        PRIVATE beman::typeclass_tree_adapters Catch2::Catch2WithMain
    )
    catch_discover_tests(beman.typeclass_tree_adapters.tests.${test})
endforeach()

# Compile-only probe (no main()); building it is the test. One structural
# fold over trees of unrelated depths must compile from one finite set of
# instantiations: a layer or handle type that changed with spine depth would
# not terminate.
add_library(
    beman.typeclass_tree_adapters.tests.compile_probe
    OBJECT
    finger_tree_compile_probe.test.cpp
)
target_link_libraries(
    beman.typeclass_tree_adapters.tests.compile_probe
    PRIVATE beman::typeclass_tree_adapters
)

# Neither library may reach the other, or the adapter, through its own
# headers: the integration is opt-in. One translation unit per library
# includes that library's umbrella alone and fails with #error if an include
# guard from the other side is defined. Building them is the test.
add_library(beman.typeclass_tree_adapters.tests.independence OBJECT)
file(
    CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/fingertree_alone.cpp"
    CONTENT
        "#include <beman/fingertree.hpp>
#ifdef BEMAN_TREE_ALGORITHMS_HPP
#error \"beman/fingertree.hpp includes beman.tree_algorithms\"
#endif
#ifdef BEMAN_TREE_ALGORITHMS_FIX_HPP
#error \"beman/fingertree.hpp includes beman.tree_algorithms\"
#endif
#ifdef BEMAN_TYPECLASS_TREE_ADAPTERS_FINGER_TREE_HPP
#error \"beman/fingertree.hpp includes the optional adapter\"
#endif
"
)
file(
    CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/tree_algorithms_alone.cpp"
    CONTENT
        "#include <beman/tree_algorithms.hpp>
#ifdef BEMAN_FINGERTREE_HPP
#error \"beman/tree_algorithms.hpp includes beman.fingertree\"
#endif
#ifdef BEMAN_FINGERTREE_TREE_FINGER_TREE_HPP
#error \"beman/tree_algorithms.hpp includes beman.fingertree\"
#endif
#ifdef BEMAN_TYPECLASS_TREE_ADAPTERS_FINGER_TREE_HPP
#error \"beman/tree_algorithms.hpp includes the optional adapter\"
#endif
"
)
target_sources(
    beman.typeclass_tree_adapters.tests.independence
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/fingertree_alone.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/tree_algorithms_alone.cpp"
)
target_link_libraries(
    beman.typeclass_tree_adapters.tests.independence
    PRIVATE beman::typeclass_tree_adapters
)
