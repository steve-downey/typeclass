// tests/beman/transpose/detail/canonical_set.test.cpp                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/detail/canonical_set.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <type_traits>

namespace bt = beman::transpose;
namespace dt = beman::transpose::detail;

// Named types with external linkage and a stable spelling -- the documented
// restrictions of the interim name order. NOT in an anonymous namespace,
// deliberately: these probes must satisfy the restriction they are testing.
namespace beman_transpose_set_probes {

struct probe_alpha {};
struct probe_beta {};
struct probe_gamma {};

} // namespace beman_transpose_set_probes

using beman_transpose_set_probes::probe_alpha;
using beman_transpose_set_probes::probe_beta;
using beman_transpose_set_probes::probe_gamma;

TEST_CASE("canonical_set: bootstrap") {
    STATIC_REQUIRE(std::is_same_v<dt::canonical_t<probe_alpha>,
                                  dt::type_list<probe_alpha>>);
}

// =========================================================================
// The order. A strict total order on types: irreflexive, asymmetric, and
// total on distinct names. Which way round it falls is the compiler's
// spelling and is not pinned; that it is an order is.
// =========================================================================

TEST_CASE("canonical_set: the name order is a strict total order") {
    STATIC_REQUIRE(!dt::type_precedes_v<probe_alpha, probe_alpha>);
    STATIC_REQUIRE(dt::type_precedes_v<probe_alpha, probe_beta> !=
                   dt::type_precedes_v<probe_beta, probe_alpha>);
    STATIC_REQUIRE(dt::type_precedes_v<probe_alpha, probe_gamma> !=
                   dt::type_precedes_v<probe_gamma, probe_alpha>);

    // The key is the name the compiler renders, so the probe's own name is
    // in it.
    STATIC_REQUIRE(dt::type_name<probe_alpha>().find("probe_alpha") !=
                   std::string_view::npos);
}

TEST_CASE("canonical_set: the order carries its key") {
    // The explicitly keyed spelling names the same entity as the unkeyed
    // one: the inline namespace is part of the identity, not of the
    // spelling anyone has to write. See
    // docs/transpose/decisions.md#type-order-abi.
    STATIC_REQUIRE(dt::ordered_by_type_name::type_name<probe_alpha>() ==
                   dt::type_name<probe_alpha>());
    STATIC_REQUIRE(
        dt::ordered_by_type_name::type_precedes_v<probe_alpha, probe_beta> ==
        dt::type_precedes_v<probe_alpha, probe_beta>);
    STATIC_REQUIRE(dt::ordered_by_type_name::names_are_distinct<probe_alpha,
                                                                probe_beta>() ==
                   dt::names_are_distinct<probe_alpha, probe_beta>());
}

// =========================================================================
// The collision check. Two distinct types rendering to the same name is the
// one case the name order cannot separate; the check is what turns it into a
// hard error at the client. The same type twice is a collision by
// construction, which is the case that can be written here.
// =========================================================================

TEST_CASE("canonical_set: names_are_distinct") {
    STATIC_REQUIRE(dt::names_are_distinct<>());
    STATIC_REQUIRE(dt::names_are_distinct<probe_alpha>());
    STATIC_REQUIRE(
        dt::names_are_distinct<probe_alpha, probe_beta, probe_gamma>());
    STATIC_REQUIRE(!dt::names_are_distinct<probe_alpha, probe_alpha>());
    STATIC_REQUIRE(
        !dt::names_are_distinct<probe_alpha, probe_beta, probe_alpha>());
}

// =========================================================================
// Deduplication is by type identity, keeping the first occurrence.
// =========================================================================

TEST_CASE("canonical_set: dedupe") {
    STATIC_REQUIRE(std::is_same_v<dt::list_dedupe<dt::type_list<>>::type,
                                  dt::type_list<>>);
    STATIC_REQUIRE(
        std::is_same_v<
            dt::list_dedupe<dt::type_list<probe_alpha, probe_alpha>>::type,
            dt::type_list<probe_alpha>>);
    STATIC_REQUIRE(
        std::is_same_v<
            dt::list_dedupe<dt::type_list<probe_alpha, probe_beta, probe_alpha,
                                          probe_gamma, probe_beta>>::type,
            dt::type_list<probe_alpha, probe_beta, probe_gamma>>);
}

// =========================================================================
// Canonical form. The load-bearing property: every spelling of the same set
// is one type, whatever the order and multiplicity it was written in.
// =========================================================================

TEST_CASE("canonical_set: canonical_t is stable under permutation and "
          "repetition") {
    STATIC_REQUIRE(std::is_same_v<dt::canonical_t<>, dt::type_list<>>);
    STATIC_REQUIRE(std::is_same_v<dt::canonical_t<probe_alpha, probe_alpha>,
                                  dt::type_list<probe_alpha>>);
    STATIC_REQUIRE(std::is_same_v<dt::canonical_t<probe_alpha, probe_beta>,
                                  dt::canonical_t<probe_beta, probe_alpha>>);
    STATIC_REQUIRE(
        std::is_same_v<dt::canonical_t<probe_alpha, probe_beta>,
                       dt::canonical_t<probe_beta, probe_alpha, probe_alpha>>);
    STATIC_REQUIRE(
        std::is_same_v<dt::canonical_t<probe_alpha, probe_beta, probe_gamma>,
                       dt::canonical_t<probe_gamma, probe_beta, probe_alpha,
                                       probe_gamma, probe_beta>>);

    // Sorting a sorted list is the identity.
    using sorted = dt::canonical_t<probe_alpha, probe_beta, probe_gamma>;
    STATIC_REQUIRE(std::is_same_v<dt::list_sort<sorted>::type, sorted>);
}

// =========================================================================
// Membership, difference, concatenation.
// =========================================================================

TEST_CASE("canonical_set: list_contains_v") {
    using abc = dt::type_list<probe_alpha, probe_beta, probe_gamma>;
    STATIC_REQUIRE(dt::list_contains_v<probe_alpha, abc>);
    STATIC_REQUIRE(dt::list_contains_v<probe_gamma, abc>);
    STATIC_REQUIRE(!dt::list_contains_v<probe_alpha, dt::type_list<>>);
    STATIC_REQUIRE(
        !dt::list_contains_v<probe_alpha, dt::type_list<probe_beta>>);
}

TEST_CASE("canonical_set: list_subtract") {
    using abc = dt::type_list<probe_alpha, probe_beta, probe_gamma>;
    STATIC_REQUIRE(
        std::is_same_v<dt::list_subtract_t<dt::type_list<>, abc>, abc>);
    STATIC_REQUIRE(
        std::is_same_v<dt::list_subtract_t<dt::type_list<probe_beta>, abc>,
                       dt::type_list<probe_alpha, probe_gamma>>);
    STATIC_REQUIRE(
        std::is_same_v<dt::list_subtract_t<abc, abc>, dt::type_list<>>);
    // Subtracting what is not there changes nothing.
    STATIC_REQUIRE(
        std::is_same_v<dt::list_subtract_t<dt::type_list<probe_gamma>,
                                           dt::type_list<probe_alpha>>,
                       dt::type_list<probe_alpha>>);
}

TEST_CASE("canonical_set: list_concat_all") {
    STATIC_REQUIRE(std::is_same_v<dt::list_concat_all_t<>, dt::type_list<>>);
    STATIC_REQUIRE(
        std::is_same_v<dt::list_concat_all_t<dt::type_list<probe_alpha>>,
                       dt::type_list<probe_alpha>>);
    STATIC_REQUIRE(
        std::is_same_v<
            dt::list_concat_all_t<dt::type_list<probe_alpha>, dt::type_list<>,
                                  dt::type_list<probe_beta, probe_gamma>>,
            dt::type_list<probe_alpha, probe_beta, probe_gamma>>);
    // Concatenation keeps order and multiplicity; canonical form is a
    // separate step.
    STATIC_REQUIRE(
        std::is_same_v<dt::list_concat_all_t<dt::type_list<probe_beta>,
                                             dt::type_list<probe_beta>>,
                       dt::type_list<probe_beta, probe_beta>>);
}

// The header speaks of types only. This file includes it and nothing else
// from the library, so a dependency on the grade framework or on any model
// would be a compile error here.
static_assert(std::is_same_v<bt::detail::canonical_t<probe_beta, probe_alpha>,
                             dt::canonical_t<probe_alpha, probe_beta>>);
