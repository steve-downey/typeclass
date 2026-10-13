# installtest/transpose/CMakeLists.txt                              -*-CMake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_package(beman.transpose REQUIRED CONFIG)

add_executable(test_installed_transpose test.cpp)
target_compile_features(test_installed_transpose PRIVATE cxx_std_23)
target_link_libraries(test_installed_transpose PRIVATE beman::transpose)

add_test(NAME installed.transpose COMMAND test_installed_transpose)
