# benchmarks/tree_algorithms/CMakeLists.txt                         -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Benchmark targets are ordinary targets in the default build: they always
# compile, so a change that breaks them fails loudly and immediately rather
# than rotting until someone next runs a benchmark. They are only *measured*
# on demand, in the Bench configuration (make bench / bench-compile).
# Anything that shapes the measurement (optimization, sanitizers) lives in
# the Bench configuration and the toolchain files, so every object in a
# build directory is compiled the same way. The one flag set here is the
# constexpr evaluation budget of the compile-time matrix: that is a property
# of those translation units, not of a build configuration -- the largest
# tree is built and folded at compile time and exceeds every compiler's
# default limit -- and it changes no generated code.

# ---------------------------------------------------------------------
# Runtime microbenchmarks (Catch2 Benchmark).
# ---------------------------------------------------------------------

find_package(Catch2 3 REQUIRED)

set(_runtime_benches fold build)

foreach(_bench IN LISTS _runtime_benches)
    add_executable(beman.tree_algorithms.benchmarks.${_bench})
    target_sources(
        beman.tree_algorithms.benchmarks.${_bench}
        PRIVATE ${_bench}.bench.cpp
    )
    target_link_libraries(
        beman.tree_algorithms.benchmarks.${_bench}
        PRIVATE beman::tree_algorithms Catch2::Catch2WithMain
    )
    if(BEMAN_TYPECLASS_USE_MODULES)
        set_target_properties(
            beman.tree_algorithms.benchmarks.${_bench}
            PROPERTIES CXX_MODULE_STD ON
        )
    endif()
endforeach()

add_custom_target(bench_runtime_all)
add_dependencies(
    bench_runtime_all
    beman.tree_algorithms.benchmarks.fold
    beman.tree_algorithms.benchmarks.build
)

# ---------------------------------------------------------------------
# Compile-time benchmarks: a matrix of generated translation units.
# ---------------------------------------------------------------------
#
# For every size in BEMAN_TREE_ALGORITHMS_BENCH_CT_SIZES and every kind in
# {fix, plain}, configure_file stamps out one .cpp from the matching
# template with @BENCH_N@ substituted. Each is its own executable, compiled
# on its own, so the build telemetry (.build/$toolchain/.trace) records one
# `compile` event per (kind, size). compile_trace_report.py reads those
# events back into a fix-vs-plain-vs-size table.

set(BEMAN_TREE_ALGORITHMS_BENCH_CT_SIZES
    "1024;4096;16384"
    CACHE STRING
    "Node counts for the compile-time benchmark matrix (semicolon-separated)."
)

add_custom_target(bench_ct_all)

foreach(_size IN LISTS BEMAN_TREE_ALGORITHMS_BENCH_CT_SIZES)
    foreach(_kind fix plain)
        set(BENCH_N ${_size})
        set(_generated "${CMAKE_CURRENT_BINARY_DIR}/ct_${_kind}_${_size}.cpp")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/ct_${_kind}.cpp.in"
            "${_generated}"
            @ONLY
        )

        set(_target beman.tree_algorithms.benchmarks.ct_${_kind}_${_size})
        add_executable(${_target})
        target_sources(${_target} PRIVATE "${_generated}")
        target_link_libraries(${_target} PRIVATE beman::tree_algorithms)
        target_compile_options(
            ${_target}
            PRIVATE
                $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=1000000000>
                $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fconstexpr-steps=100000000>
                $<$<CXX_COMPILER_ID:MSVC>:/constexpr:steps100000000>
        )

        add_dependencies(bench_ct_all ${_target})
    endforeach()
endforeach()
