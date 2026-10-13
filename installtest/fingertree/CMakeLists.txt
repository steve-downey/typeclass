# installtest/fingertree/CMakeLists.txt                             -*-CMake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Added by the parent only when the package is installed; see ../CMakeLists.txt.
find_package(beman.fingertree REQUIRED CONFIG)

add_executable(test_installed_fingertree test.cpp)
target_compile_features(test_installed_fingertree PRIVATE cxx_std_26)
target_link_libraries(test_installed_fingertree PRIVATE beman::fingertree)

add_test(NAME installed.fingertree COMMAND test_installed_fingertree)
