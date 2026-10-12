module;

// beman.transpose is not a module. Its headers are included here, in the
// global module fragment, so that their declarations stay attached to the
// global module; the later includes inside the export block below are then
// skipped by the include guards rather than redeclaring them in this
// module's purview.
#include <beman/transpose/overloaded.hpp>

// <cassert> is macro-only, so `import std` does not provide it. Included
// here it brings its declarations in attached to the global module, as
// they must be; included first inside the purview below, through
// binary_tree.hpp and fringe_tree.hpp, it redeclared the library's
// configuration types in this module.
#include <cassert>

export module beman.tree_algorithms;

import std;

#define BEMAN_TREE_ALGORITHMS_INCLUDED_FROM_INTERFACE_UNIT
export {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"
#include <beman/tree_algorithms/tree_algorithms.hpp>
#pragma clang diagnostic pop
}
