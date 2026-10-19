// include/beman/transpose.hpp                                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_HPP
#define BEMAN_TRANSPOSE_HPP

// Umbrella header for beman.transpose: every typeclass, every instance, and
// the operation objects, including `traverse` and `transpose`. A program that
// includes this header everywhere it uses the library sees every instance
// before any use (docs/transpose/decisions.md#instance-placement).
//
// It sits beside the package directory rather than in it: a Beman package
// owns every extension of its name under beman/, so beman/transpose.hpp
// cannot collide with another package.

#include <beman/transpose/config.hpp>

#include <beman/transpose/apply.hpp>
#include <beman/transpose/array.hpp>
#include <beman/transpose/error_set.hpp>
#include <beman/transpose/expected.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/grade.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/optional.hpp>
#include <beman/transpose/sender.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/string.hpp>
#include <beman/transpose/traverse.hpp>
#include <beman/transpose/zip_list.hpp>

#if __has_include(<simd>)
    #include <beman/transpose/simd.hpp> // self-gates further on C++26
    #include <beman/transpose/simd_lanes.hpp>
#endif

#endif // BEMAN_TRANSPOSE_HPP
