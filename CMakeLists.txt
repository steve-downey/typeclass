# CMakeLists.txt                                                     -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.4)

include(infra/cmake/enable-experimental-import-std.cmake)

project(
    beman.typeclass
    DESCRIPTION "Typeclass Based Libraries for C++"
    LANGUAGES CXX
    VERSION 0.1.0
)

# One repository, three libraries that layer on shared machinery:
#   beman.transpose       -- Applicative/Traversable, traverse and transpose (P3200)
#   beman.tree_algorithms -- the fold family and direct verbs for trees
#   beman.fingertree      -- a persistent measured sequence
# Each is its own target, header set, and installed package; they are built
# together so that a change to shared design is checked against all of them.

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_TYPECLASS_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_TYPECLASS_BUILD_EXAMPLES
    "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# Benchmarks build with everything else so they can never rot silently.
# They are only *run* / *measured* on demand (make bench, bench-compile) in
# the Bench configuration; ordinary builds just keep them compiling.
option(
    BEMAN_TYPECLASS_BUILD_BENCHMARKS
    "Enable building benchmarks. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_TYPECLASS_USE_MODULES
    "Provide beman.tree_algorithms and beman.fingertree as C++ modules"
    OFF
)

# The P2300 evidence -- examples/transpose/p2300_adapter.hpp and its tests --
# is the only thing here with an external dependency, so it is opt-in and OFF
# by default: a plain build of this repository must not need beman.execution
# fetched. Turn it on to check the Applicative surface against genuine
# std::execution senders. See docs/transpose/decisions.md#p2300-front-door-shape.
option(
    BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE
    "Enable the non-normative P2300 sender adapter and its test. Requires beman.execution. Default: OFF. Values: { ON, OFF }."
    OFF
)

if(BEMAN_TYPECLASS_USE_MODULES)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

# for find of beman_install_library and configure_build_telemetry
include(infra/cmake/beman-install-library.cmake)
include(infra/cmake/BuildTelemetryConfig.cmake)

if(BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE)
    # beman.execution builds as C++20 modules by default, and its umbrella
    # header then resolves to `import` declarations. The adapter is written
    # against the header path, which is the one a reader can copy. Set,
    # without FORCE, so an explicit -DBEMAN_USE_MODULES=ON still wins.
    set(BEMAN_USE_MODULES OFF CACHE BOOL "Build beman.execution as headers.")
    find_package(beman.execution REQUIRED)
endif()

if(BEMAN_TYPECLASS_BUILD_TESTS OR BEMAN_TYPECLASS_BUILD_BENCHMARKS)
    find_package(Catch2 3 REQUIRED)
    # The fetch-content dependency provider makes Catch2 available inside a
    # function scope, so its extras/ directory never reaches
    # CMAKE_MODULE_PATH and catch2_SOURCE_DIR is not visible here; recover it
    # from FetchContent's global properties. (The vcpkg path needs neither:
    # Catch2's installed config puts Catch.cmake on the module path itself.)
    if(NOT COMMAND catch_discover_tests)
        include(FetchContent)
        FetchContent_GetProperties(catch2)
        if(catch2_SOURCE_DIR)
            list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
        endif()
    endif()
    include(Catch)
    enable_testing()
endif()

add_subdirectory(src/beman)

if(BEMAN_TYPECLASS_BUILD_TESTS)
    add_subdirectory(tests/beman)
endif()

configure_build_telemetry()

if(BEMAN_TYPECLASS_BUILD_EXAMPLES)
    if(NOT BEMAN_TYPECLASS_BUILD_TESTS)
        enable_testing()
    endif()
    add_subdirectory(examples)
endif()

if(BEMAN_TYPECLASS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Coverage
configure_file("cmake/gcovr.cfg.in" gcovr.cfg @ONLY)

add_custom_target(
    process_coverage
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running gcovr to process coverage results"
    COMMAND mkdir -p coverage
    COMMAND gcovr --config gcovr.cfg .
)
