# installtest/typeclass_tree_adapters/CMakeLists.txt                -*-CMake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Added by the parent only when the package is installed; see ../CMakeLists.txt.
# The package's config finds beman.tree_algorithms and beman.fingertree for
# itself, and each of those finds beman.transpose; that chain is part of
# what this test checks.
find_package(beman.typeclass_tree_adapters REQUIRED CONFIG)

add_executable(test_installed_typeclass_tree_adapters test.cpp)
target_compile_features(
    test_installed_typeclass_tree_adapters
    PRIVATE cxx_std_26
)
target_link_libraries(
    test_installed_typeclass_tree_adapters
    PRIVATE beman::typeclass_tree_adapters
)

add_test(
    NAME installed.typeclass_tree_adapters
    COMMAND test_installed_typeclass_tree_adapters
)
