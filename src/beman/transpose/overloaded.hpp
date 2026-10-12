// src/beman/transpose/overloaded.hpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_OVERLOADED_HPP
#define BEMAN_TRANSPOSE_OVERLOADED_HPP

namespace beman::transpose {

/** An overload set built from callables, for `std::visit` and for writing
 * algebras over a variant-shaped layer:
 *
 *     std::visit(overloaded{
 *         [](int x)         { ... },
 *         [](std::string s) { ... },
 *     }, v);
 *
 * Selection is overload resolution, so the best match wins, not the first
 * case written. The consteval catch-all is a diagnostic, not a fallback: it
 * is chosen only when no case accepts the argument, and turns that into a
 * compile-time error naming the problem. Adding an alternative to a variant
 * without handling it everywhere is caught where the visit is written. A
 * generic case of the caller's own, `[](const auto&)`, is ambiguous with it;
 * see docs/transpose/decisions.md#overloaded-home.
 *
 * No deduction guide is needed: aggregate CTAD deduces `overloaded<F...>`
 * from the callables.
 */
template <class... FUNCTIONS>
struct overloaded : FUNCTIONS... {
    using FUNCTIONS::operator()...;

    consteval void operator()(auto) const {
        static_assert(
            false, "overloaded: unhandled variant alternative -- add a case");
    }
};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_OVERLOADED_HPP
