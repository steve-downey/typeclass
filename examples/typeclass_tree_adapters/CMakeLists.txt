# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES finger_tree_structure)
message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.typeclass_tree_adapters.examples.${example})
    target_sources(
        beman.typeclass_tree_adapters.examples.${example}
        PRIVATE ${example}.cpp
    )
    target_link_libraries(
        beman.typeclass_tree_adapters.examples.${example}
        PRIVATE beman::typeclass_tree_adapters
    )
    # The example checks its own results, and the guide quotes it, so it
    # runs with the tests.
    add_test(
        NAME beman.typeclass_tree_adapters.examples.${example}
        COMMAND beman.typeclass_tree_adapters.examples.${example}
    )
endforeach()
