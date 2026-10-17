# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

message(NOTICE "USE_VCPKG_TOOLCHAIN: $ENV{PROJECT_VCPKG_TOOLCHAIN}")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{PROJECT_VCPKG_TOOLCHAIN}")

# vcpkg hashes the chainloaded toolchain file, but not the files it includes.
# The compiler flags live in those, so name them explicitly; otherwise a flags
# change would restore a stale cached port build.
file(GLOB _flags_files "${CMAKE_CURRENT_LIST_DIR}/*-flags.cmake")
set(VCPKG_HASH_ADDITIONAL_FILES ${_flags_files})
unset(_flags_files)
