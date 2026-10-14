# src/beman/CMakeLists.txt                                           -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# beman.fingertree requires std::inplace_vector at C++26, with no fallback;
# today only libstdc++ 16 ships it. The other libraries build on much older
# toolchains, so rather than hold them to fingertree's floor, fingertree is
# built only where the probe below succeeds. Force it either way with
# -DBEMAN_TYPECLASS_BUILD_FINGERTREE=ON|OFF.
#
# The probe asks for C++26 with CMAKE_CXX_STANDARD_REQUIRED, which is a hard
# configure error on a compiler CMake has no C++26 flag for (MSVC today), so
# the dialect is checked first and the probe runs only where it can.
include(CheckCXXSourceCompiles)
if(cxx_std_26 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    block()
        set(CMAKE_CXX_STANDARD 26)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        check_cxx_source_compiles(
            "#include <inplace_vector>
            int main() { std::inplace_vector<int, 1> v; return int(v.size()); }"
            BEMAN_TYPECLASS_HAS_INPLACE_VECTOR
        )
    endblock()
else()
    set(BEMAN_TYPECLASS_HAS_INPLACE_VECTOR OFF)
endif()
option(
    BEMAN_TYPECLASS_BUILD_FINGERTREE
    "Build beman.fingertree. Default: whether std::inplace_vector is available at C++26. Values: { ON, OFF }."
    ${BEMAN_TYPECLASS_HAS_INPLACE_VECTOR}
)

# Order is layering order: later libraries may build on earlier ones.
add_subdirectory(transpose)
add_subdirectory(tree_algorithms)
if(BEMAN_TYPECLASS_BUILD_FINGERTREE)
    add_subdirectory(fingertree)
else()
    message(
        STATUS
        "beman.fingertree: skipped, std::inplace_vector is not available at C++26"
    )
endif()
