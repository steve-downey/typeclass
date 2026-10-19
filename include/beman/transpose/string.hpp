// include/beman/transpose/string.hpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_STRING_HPP
#define BEMAN_TRANSPOSE_STRING_HPP

// The registered Monoid for std::string.
//
// This header stands in for <string>, which would declare the instance itself
// (docs/transpose/decisions.md#instance-placement); the paper proposes no
// wording for it. Include it, not only <string>, before the first use of
// Monoid<std::string>, in every translation unit that makes one: a use that
// precedes the instance is ill-formed, no diagnostic required. transpose.hpp
// includes it.

#include <beman/transpose/monoid.hpp>

#include <string>

namespace beman::transpose {

/** Monoid<std::string>: concatenation monoid with identity "". */
template <>
struct Monoid<std::string> {
    using value_type = std::string;

    constexpr auto identity() const -> std::string { return {}; }

    constexpr auto combine(const std::string &lhs, const std::string &rhs) const
        -> std::string {
        return lhs + rhs;
    }
};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_STRING_HPP
