# src/beman/transpose/CMakeLists.txt                                 -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

configure_file(
    "${PROJECT_SOURCE_DIR}/include/beman/transpose/config_generated.hpp.in"
    "${PROJECT_BINARY_DIR}/include/beman/transpose/config_generated.hpp"
    @ONLY
)

# beman.transpose is a header-only library.
add_library(beman.transpose INTERFACE)
add_library(beman::transpose ALIAS beman.transpose)

target_compile_features(beman.transpose INTERFACE cxx_std_23)

target_sources(
    beman.transpose
    INTERFACE
        FILE_SET HEADERS
            BASE_DIRS
                "${PROJECT_SOURCE_DIR}/include"
                "${PROJECT_BINARY_DIR}/include"
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/config.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/apply.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/array.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/error_set.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/expected.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/fold.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/functor.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/grade.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/induced_monoid.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/monad.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/monoid.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/optional.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/overloaded.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/sender.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/sequence.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/string.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/traverse.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/zip_list.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/simd.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/simd_lanes.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/detail/applicative_derivation.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/detail/make_array.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/transpose/detail/typeclass_base.hpp
                "${PROJECT_BINARY_DIR}/include/beman/transpose/config_generated.hpp"
)

set_target_properties(
    beman.transpose
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
)

beman_install_library(beman.transpose TARGETS beman.transpose)
