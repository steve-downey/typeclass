// include/beman/transpose/detail/make_array.hpp                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_DETAIL_MAKE_ARRAY_HPP
#define BEMAN_TRANSPOSE_DETAIL_MAKE_ARRAY_HPP

#include <array>
#include <cstddef>
#include <utility>

namespace beman::transpose::detail {

/** Builds a `std::array<U, N>` by calling `generator` with each index in turn.
 *
 * Default-constructing the array and then assigning its elements is the
 * obvious way to fill a fixed-extent result, and it silently requires the
 * element type to be default-constructible AND assignable -- neither of which
 * the operations that build such results actually need. Constructing the
 * array directly from the generated elements requires only what the operation
 * documents: that each element is constructible from its own result.
 *
 * The initializer-clauses of a braced-init-list are sequenced left to right
 * ([dcl.init.list]/4), so lane 0's element is produced before lane 1's.
 */
template <class U, std::size_t N, class GENERATOR, std::size_t... INDICES>
constexpr auto make_array(GENERATOR &&generator,
                          std::index_sequence<INDICES...>) -> std::array<U, N> {
    return std::array<U, N>{generator(INDICES)...};
}

} // namespace beman::transpose::detail

#endif // BEMAN_TRANSPOSE_DETAIL_MAKE_ARRAY_HPP
