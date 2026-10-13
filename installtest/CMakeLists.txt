# installtest/CMakeLists.txt                                        -*-CMake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Consumes the three libraries as a downstream project would: find_package on
# the installed packages, nothing from this source tree. Point
# CMAKE_PREFIX_PATH at the install prefix; the default is the Makefile's
# .install/ beside the repository root. `make testinstall` configures, builds
# and runs this.

cmake_minimum_required(VERSION 3.27)
project(TestInstalledTypeclass LANGUAGES CXX)

enable_testing()

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../.install")
endif()
message(STATUS "installtest: looking for packages under ${CMAKE_PREFIX_PATH}")

add_subdirectory(transpose)
add_subdirectory(tree_algorithms)

# fingertree needs std::inplace_vector at C++26, so the install may not carry
# it; the build probes and skips (src/beman/CMakeLists.txt), and so does this.
find_package(beman.fingertree QUIET CONFIG)
if(beman.fingertree_FOUND)
    add_subdirectory(fingertree)
else()
    message(STATUS "installtest: beman.fingertree not installed, skipped")
endif()
