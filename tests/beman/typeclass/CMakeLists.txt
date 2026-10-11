# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_package(Catch2 3 REQUIRED)

add_executable(beman.typeclass.tests.todo)
target_sources(beman.typeclass.tests.todo PRIVATE todo.test.cpp)
target_link_libraries(
    beman.typeclass.tests.todo
    PRIVATE beman::typeclass Catch2::Catch2WithMain
)
if(BEMAN_TYPECLASS_USE_MODULES)
    set_target_properties(
        beman.typeclass.tests.todo
        PROPERTIES CXX_MODULE_STD ON
    )
endif()

include(Catch)
catch_discover_tests(beman.typeclass.tests.todo)
