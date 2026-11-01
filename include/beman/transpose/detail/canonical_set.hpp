// include/beman/transpose/detail/canonical_set.hpp                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_DETAIL_CANONICAL_SET_HPP
#define BEMAN_TRANSPOSE_DETAIL_CANONICAL_SET_HPP

// The canonical set of types: a pack deduplicated by identity and sorted by
// a strict total order on types, so that two spellings of the same set are
// one type. This is framework-neutral machinery -- it knows nothing about
// what the set is FOR -- and it lives here, beside grade.hpp, so that any
// model or any other canonical sum can build on it without acquiring the
// vocabulary of the first client. See
// docs/transpose/decisions.md#canonical-set-home.
//
// DEDUPLICATION is by `std::is_same`, never by name: the name is an ordering
// key, not an equality.
//
// TYPE ORDERING is P2830's job. Until `std::type_order` is available this
// header uses an interim ordering read out of the compiler's own function
// signature, valid only for the documented restrictions: named types,
// external linkage, stable spelling. Two distinct types that render to the
// same name would be silently conflated, so a client checks
// `names_are_distinct` and makes that case ill-formed rather than a wrong
// answer.
//
// THE ORDER IS PART OF THE ABI. Any type whose layout follows the canonical
// order -- a tuple of slots in sorted position, say -- has a different layout
// under a different order, and two translation units that disagreed about
// the order would violate the ODR silently. The order therefore carries an
// inline namespace keyed on which order is in use, and an order-dependent
// type carries the same key. Today there is one key,
// `ordered_by_type_name`; a P2830 order arrives as a second key beside it,
// so that mixing the two fails to link instead of corrupting a value. See
// docs/transpose/decisions.md#type-order-abi.

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace beman::transpose::detail {

// -- Interim type ordering (P2830 fallback) -------------------------------
// The key names the order: everything in this namespace is specific to
// ordering types by their rendered name, and nothing outside it is.

inline namespace ordered_by_type_name {

/** The compiler's spelling of T, extracted from the enclosing signature.
 * Valid as an ordering key only under the documented restrictions: named
 * types with external linkage and a stable spelling.
 */
template <class T>
consteval auto type_name() -> std::string_view {
#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC has no __PRETTY_FUNCTION__. __FUNCSIG__ spells the specialization
    // as `... type_name<T>(void)`, so the name is what sits between the
    // template argument list and the last `>`. Should that spelling ever
    // change, fall back to the whole signature, which still names T and so
    // is still an injective key -- a duller order, not a broken one.
    const std::string_view signature{__FUNCSIG__};
    constexpr std::string_view marker{"type_name<"};

    const auto found = signature.find(marker);
    const auto stop = signature.rfind('>');
    if (found == std::string_view::npos || stop == std::string_view::npos ||
        stop <= found + marker.size()) {
        return signature;
    }
    const auto start = found + marker.size();
    return signature.substr(start, stop - start);
#else
    const std::string_view signature{__PRETTY_FUNCTION__};
    constexpr std::string_view marker{"T = "};

    const auto start = signature.find(marker) + marker.size();
    auto stop = signature.find(';', start);
    if (stop == std::string_view::npos) {
        stop = signature.rfind(']');
    }
    return signature.substr(start, stop - start);
#endif
}

/** Strict total order on types, by name. Interim stand-in for P2830. */
template <class LEFT, class RIGHT>
inline constexpr bool type_precedes_v = type_name<LEFT>() < type_name<RIGHT>();

/** False when two DISTINCT types render to the same name, which is exactly
 * the case the interim ordering cannot handle. */
template <class... TYPES>
consteval auto names_are_distinct() -> bool {
    constexpr std::size_t count = sizeof...(TYPES);
    const std::array<std::string_view, count> names{type_name<TYPES>()...};
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1; j < count; ++j) {
            if (names[i] == names[j]) {
                return false;
            }
        }
    }
    return true;
}

/// The same check as a trait, which is the form a client names in a
/// declaration.
//! \expos
template <class... TYPES>
inline constexpr bool names_are_distinct_v = names_are_distinct<TYPES...>();

} // namespace ordered_by_type_name

// -- Type list machinery --------------------------------------------------

//! \expos
template <class... TYPES>
struct type_list {};

template <class T, class LIST>
inline constexpr bool list_contains_v = false;

template <class T, class... TYPES>
inline constexpr bool list_contains_v<T, type_list<TYPES...>> =
    (std::is_same_v<T, TYPES> || ...);

template <class T, class LIST>
struct list_prepend;

//! \omit
template <class T, class... TYPES>
struct list_prepend<T, type_list<TYPES...>> {
    using type = type_list<T, TYPES...>;
};

template <class T, class LIST>
using list_prepend_t = typename list_prepend<T, LIST>::type;

template <class T, class LIST>
struct list_remove;

//! \omit
template <class T>
struct list_remove<T, type_list<>> {
    using type = type_list<>;
};

//! \omit
template <class T, class HEAD, class... TAIL>
struct list_remove<T, type_list<HEAD, TAIL...>> {
  private:
    using rest = typename list_remove<T, type_list<TAIL...>>::type;

  public:
    using type = std::conditional_t<std::is_same_v<T, HEAD>, rest,
                                    list_prepend_t<HEAD, rest>>;
};

/** Deduplicate by type identity, keeping the first occurrence. Identity is
 * std::is_same, never the name: names are an ordering key, not an equality. */
template <class LIST>
struct list_dedupe;

//! \omit
template <>
struct list_dedupe<type_list<>> {
    using type = type_list<>;
};

//! \omit
template <class HEAD, class... TAIL>
struct list_dedupe<type_list<HEAD, TAIL...>> {
    using type =
        list_prepend_t<HEAD, typename list_dedupe<typename list_remove<
                                 HEAD, type_list<TAIL...>>::type>::type>;
};

template <class T, class LIST>
struct list_insert_sorted;

//! \omit
template <class T>
struct list_insert_sorted<T, type_list<>> {
    using type = type_list<T>;
};

//! \omit
template <class T, class HEAD, class... TAIL>
struct list_insert_sorted<T, type_list<HEAD, TAIL...>> {
    using type = std::conditional_t<
        type_precedes_v<T, HEAD>, type_list<T, HEAD, TAIL...>,
        list_prepend_t<
            HEAD, typename list_insert_sorted<T, type_list<TAIL...>>::type>>;
};

template <class LIST>
struct list_sort;

//! \omit
template <>
struct list_sort<type_list<>> {
    using type = type_list<>;
};

//! \omit
template <class HEAD, class... TAIL>
struct list_sort<type_list<HEAD, TAIL...>> {
    using type = typename list_insert_sorted<
        HEAD, typename list_sort<type_list<TAIL...>>::type>::type;
};

/** The canonical form of a pack: deduplicated, then sorted. */
//! \expos
template <class... TYPES>
using canonical_t =
    typename list_sort<typename list_dedupe<type_list<TYPES...>>::type>::type;

// -- Set difference and list concatenation --------------------------------

/** FROM_LIST with every member of REMOVE_LIST taken out. */
template <class REMOVE_LIST, class FROM_LIST>
struct list_subtract;

template <class FROM_LIST>
struct list_subtract<type_list<>, FROM_LIST> {
    using type = FROM_LIST;
};

template <class HEAD, class... TAIL, class FROM_LIST>
struct list_subtract<type_list<HEAD, TAIL...>, FROM_LIST> {
    using type = typename list_subtract<
        type_list<TAIL...>, typename list_remove<HEAD, FROM_LIST>::type>::type;
};

//! \expos
template <class REMOVE_LIST, class FROM_LIST>
using list_subtract_t = typename list_subtract<REMOVE_LIST, FROM_LIST>::type;

/** Concatenates any number of type_lists, left to right. */
template <class... LISTS>
struct list_concat_all;

template <>
struct list_concat_all<> {
    using type = type_list<>;
};

template <class... TYPES>
struct list_concat_all<type_list<TYPES...>> {
    using type = type_list<TYPES...>;
};

template <class... LEFT, class... RIGHT, class... REST>
struct list_concat_all<type_list<LEFT...>, type_list<RIGHT...>, REST...>
    : list_concat_all<type_list<LEFT..., RIGHT...>, REST...> {};

//! \expos
template <class... LISTS>
using list_concat_all_t = typename list_concat_all<LISTS...>::type;

} // namespace beman::transpose::detail

#endif // BEMAN_TRANSPOSE_DETAIL_CANONICAL_SET_HPP
