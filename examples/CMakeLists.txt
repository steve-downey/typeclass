# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES todo)
message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.typeclass.examples.${example})
    target_sources(beman.typeclass.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(
        beman.typeclass.examples.${example}
        PRIVATE beman::typeclass
    )
    if(BEMAN_TYPECLASS_USE_MODULES)
        set_target_properties(
            beman.typeclass.examples.${example}
            PROPERTIES CXX_MODULE_STD ON
        )
    endif()
endforeach()
