# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if(BEMAN_TYPECLASS_USE_MODULES)
    target_sources(
        beman.typeclass
        PUBLIC
            FILE_SET CXX_MODULES FILES typeclass.cppm
            FILE_SET HEADERS
                FILES
                    config.hpp
                    typeclass.hpp
                    todo.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/typeclass/config_generated.hpp"
    )
else()
    target_sources(
        beman.typeclass
        PUBLIC
            FILE_SET HEADERS
                FILES
                    config.hpp
                    typeclass.hpp
                    todo.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/typeclass/config_generated.hpp"
    )
endif()
