# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES
    todo
    fixpoint_tree_example
    expression_algorithms
    binary_tree_adapt
    nonce_tree_direct
    search_tree_on_fix
    sequence_algorithms
)
message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.tree_algorithms.examples.${example})
    target_sources(
        beman.tree_algorithms.examples.${example}
        PRIVATE ${example}.cpp
    )
    target_link_libraries(
        beman.tree_algorithms.examples.${example}
        PRIVATE beman::tree_algorithms
    )
    if(BEMAN_TYPECLASS_USE_MODULES)
        set_target_properties(
            beman.tree_algorithms.examples.${example}
            PROPERTIES CXX_MODULE_STD ON
        )
    endif()
endforeach()

# Anchored-snippet gate. The article
# (docs/tree_algorithms/recursive-tree-algorithms.org) and the paper transclude
# UUID-anchored regions from the targets below. paper_snippets_check is part of
# the default build (ALL), so any anchored file that stops compiling fails the
# build; it can also be built alone:
#   cmake --build <build-dir> --config <cfg> --target paper_snippets_check
add_custom_target(paper_snippets_check ALL)
foreach(
    _snippet_target
    IN
    ITEMS
        # examples with anchored regions
        beman.tree_algorithms.examples.fixpoint_tree_example
        beman.tree_algorithms.examples.expression_algorithms
        beman.tree_algorithms.examples.binary_tree_adapt
        # test targets whose sources (and included headers) carry anchors
        beman.tree_algorithms.tests.box
        beman.tree_algorithms.tests.fix
        beman.tree_algorithms.tests.expression
        beman.tree_algorithms.tests.functors
        beman.tree_algorithms.tests.binary_tree
)
    if(TARGET ${_snippet_target})
        add_dependencies(paper_snippets_check ${_snippet_target})
    endif()
endforeach()
