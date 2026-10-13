# installtest/tree_algorithms/CMakeLists.txt                        -*-CMake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# The package's config finds beman.transpose for itself; that dependency is
# part of what this test checks.
find_package(beman.tree_algorithms REQUIRED CONFIG)

add_executable(test_installed_tree_algorithms test.cpp)
target_compile_features(test_installed_tree_algorithms PRIVATE cxx_std_23)
target_link_libraries(
    test_installed_tree_algorithms
    PRIVATE beman::tree_algorithms
)

add_test(NAME installed.tree_algorithms COMMAND test_installed_tree_algorithms)
