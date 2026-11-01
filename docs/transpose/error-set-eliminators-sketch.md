# Sketch: public eliminators for a multi-witness `error_set`

Status: a sketch for the "fence moves" outcome of
[multi-witness-elimination](decisions.md#multi-witness-elimination), written
2026-11-01 so that the ruling in stage
[set-elimination](transpose-surface-plan.md#set-elimination) is taken against
concrete forms rather than a description. The code here was compiled with GCC
15 in C++23 mode against the current `error_set.hpp` and run; the output is
reproduced below. Nothing here is shipped, and the names are provisional.

## The two verbs

```cpp
fold_witnesses(errors, init, handler)           -> ACC
collapse<TARGET>(errors, handler[, combine])    -> TARGET
collapse<TARGET>(expected, handler[, combine])  -> std::expected<VALUE, TARGET>
```

`fold_witnesses` is the report. For each alternative the value witnesses, in
canonical order, the accumulator becomes `handler(std::move(acc), witness)`.
The accumulator is where two witnesses meet, so multiplicity is the caller's
stated business and nothing is dropped unless an arm drops it.

`collapse` is the boundary. Every arm maps its alternative to one error type,
the results are folded with `combine`, and one `TARGET` leaves. `TARGET` may
be omitted when every arm returns the same type. `combine` defaults to keeping
the first result in canonical order, which is the rule `combined_with` and
`recover` already use; pass `std::plus<>{}` or the like when a second witness
must survive. Applied to the `std::expected`, the result is the ungraded
`std::expected<VALUE, TARGET>`: the graded region ends here.

Both are built from the public API only, `holds<E>()` and `witness<E>()`, the
way `recover`'s internals are, so neither is privileged.

## In use

```cpp
auto both = accumulating.invoke(add, bad_parse(12), bad_range(99));
// both.error() witnesses parse_error{12} and range_error{99}

// The report: every witness into a vector, arms by overload set.
auto messages = bt::fold_witnesses(
    both.error(), std::vector<std::string>{},
    bt::overloaded{
        [](std::vector<std::string> acc, const parse_error& e) {
            acc.push_back(describe(e)); return acc; },
        [](std::vector<std::string> acc, const range_error& e) {
            acc.push_back(describe(e)); return acc; },
    });

// A generic arm is enough when the alternatives share an interface.
auto count = bt::fold_witnesses(both.error(), 0,
                                [](int n, const auto&) { return n + 1; });

// The boundary: one error type, deduced from the arms, first witness kept.
auto first = bt::collapse(both, bt::overloaded{
    [](const parse_error& e) { return describe(e); },
    [](const range_error& e) { return describe(e); },
});
static_assert(std::is_same_v<decltype(first), std::expected<int, std::string>>);

// With a combine, the second witness survives.
auto joined = bt::collapse(both,
                           [](const auto& e) { return describe(e) + ";"; },
                           std::plus<>{});

// TARGET named: the arms return different things and each converts.
auto code = bt::collapse<long>(both.error(), bt::overloaded{
    [](const parse_error& e) { return e.position; },     // int
    [](const range_error& e) { return short(e.value); }, // short
});
```

Output of the test program:

```text
fold_witnesses: 2 messages: parse@12 range=99
fold_witnesses generic arm: count = 2
collapse (keep first): parse@12
collapse (std::plus): parse@12;range=99;
collapse<long>: 12
collapse single: parse@12
collapse success: 7
```

Both verbs are `constexpr`; the test folds a constant `error_set` and
`static_assert`s on the count.

## Diagnostics

A missing arm is a compile error naming the alternative. `overloaded`'s
consteval catch-all is invocable and returns `void`, so "an arm accepts E" is
checked as "invocable and not void", and the catch-all is reported as the
missing arm it stands for:

```text
In instantiation of 'struct collapse_arm_check<overloaded<lambda(const parse_error&)>, range_error>':
error: static assertion failed: collapse: no arm accepts this alternative -- add a case
```

Arms that disagree without a named target:

```text
error: static assertion failed: collapse: the arms return different types.
Name the target, collapse<TARGET>(...), and every arm's result converts to it.
```

## Points for the ruling

- **Naming.** `fold.hpp` already holds the Foldable family (`fold_map`,
  `fold_left`, `fold_right`), so a bare `fold` on `error_set` would sit in
  their namespace meaning something else. `fold_witnesses` says what it folds.
  `collapse` has no neighbour. `recover`, `collapse`, `fold_witnesses` would
  be the whole elimination vocabulary, with `visit` staying partial.
- **The default combine.** Keeping the first matches `combined_with`, but it
  is also the silent drop the `visit` precondition refuses. The alternative is
  no default: `collapse` always takes a combine, and the one-witness callers
  pay a `keep_first` argument. The sketch defaults; the ruling may not.
- **Where the grade ends.** `collapse` on the `std::expected` returns
  `std::expected<VALUE, TARGET>`, not `std::expected<VALUE, error_set<TARGET>>`.
  That is the boundary reading. A caller who wants to stay graded names
  `collapse<error_set<TARGET>>`, and the singleton set converts from `TARGET`
  through the injecting constructor.
- **`recover` through `fold_witnesses`.** `recover`'s body is this fold with
  accumulator `(std::optional<VALUE>, std::optional<REMAINDER>)`. Re-expressing
  it is a refactor with no observable change if the tie-break is kept; whether
  to do it is sub-question 4 under the slug.
- **House style.** The header below is a sketch and has not had the
  house-style pass; stage set-elimination does that.

## The header as compiled

```cpp
// Sketch for docs/transpose/decisions.md#multi-witness-elimination, outcome
// "fence moves". Two public eliminators for a multi-witness error_set, built
// from the public API only (holds, witness), the way recover's internals are.
//
//   fold_witnesses(errors, init, handler)       -> ACC
//   collapse<TARGET>(errors, handler[, combine]) -> TARGET
//   collapse<TARGET>(expected, handler[, combine]) -> std::expected<VALUE, TARGET>
//
// TARGET may be omitted when every arm returns the same type.
#ifndef SKETCH_ERROR_SET_ELIMINATORS_HPP
#define SKETCH_ERROR_SET_ELIMINATORS_HPP

#include <beman/transpose/error_set.hpp>

#include <concepts>
#include <expected>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace beman::transpose {
namespace detail {

// One instantiation per alternative, so a failing static_assert names the
// alternative in its instantiation context, as overloaded's catch-all does.
// `overloaded`'s consteval catch-all is invocable and returns void, so "an
// arm accepts E" is "invocable and not void": the catch-all is then reported
// here as the missing arm it stands for, before its own diagnostic fires.
template <class HANDLER, class... ARGS>
concept arm_for = std::invocable<HANDLER &, ARGS...> &&
                  !std::is_void_v<std::invoke_result_t<HANDLER &, ARGS...>>;

template <class HANDLER, class ACC, class E>
struct fold_arm_check {
    static_assert(arm_for<HANDLER, ACC &&, const E &>,
                  "fold_witnesses: no arm accepts (ACC, const E&) for this "
                  "alternative -- add a case");
    static_assert(std::is_invocable_r_v<ACC, HANDLER &, ACC &&, const E &>,
                  "fold_witnesses: this arm's result does not convert to ACC");
    static constexpr bool value = true;
};

template <class HANDLER, class E>
struct collapse_arm_check {
    static_assert(arm_for<HANDLER, const E &>,
                  "collapse: no arm accepts this alternative -- add a case");
    using type = std::remove_cvref_t<std::invoke_result_t<HANDLER &, const E &>>;
};

/** The common result of every arm. Ill-formed, with a message, when the arms
 * disagree and no TARGET was named. */
template <class HANDLER, class... ERRORS>
struct common_arm_result;

template <class HANDLER, class FIRST, class... REST>
struct common_arm_result<HANDLER, FIRST, REST...> {
    using type = typename collapse_arm_check<HANDLER, FIRST>::type;
    static_assert(
        (std::is_same_v<type, typename collapse_arm_check<HANDLER, REST>::type> &&
         ...),
        "collapse: the arms return different types. Name the target, "
        "collapse<TARGET>(...), and every arm's result converts to it.");
};

/** TARGET as written, or the arms' common result when TARGET is void. A
 * specialization rather than conditional_t, so that naming TARGET never
 * instantiates the agreement check. */
template <class TARGET, class HANDLER, class... ERRORS>
struct collapse_target {
    using type = TARGET;
};

template <class HANDLER, class... ERRORS>
struct collapse_target<void, HANDLER, ERRORS...>
    : common_arm_result<HANDLER, ERRORS...> {};

template <class TARGET, class HANDLER, class... ERRORS>
using collapse_target_t =
    typename collapse_target<TARGET, HANDLER, ERRORS...>::type;

/** The default multiplicity rule: the first result in canonical order wins,
 * matching error_set_of::combined_with and recover. */
struct keep_first {
    template <class T>
    constexpr auto operator()(T first, const T &) const -> T {
        return first;
    }
};

} // namespace detail

// -- fold_witnesses ----------------------------------------------------------

//! \mandates For every alternative `E` of the set, `handler` is invocable
//! with `(ACC&&, const E&)` and the result converts to `ACC`.
//! \effects For each alternative the set witnesses, in canonical order,
//! replaces the accumulator with `handler(std::move(acc), witness)`.
//! \returns The final accumulator.
//! \remarks Multiplicity is the caller's stated business: the accumulator is
//! where two witnesses meet, and nothing is dropped unless the arm drops it.
template <class ACC, class HANDLER, class... ERRORS>
constexpr auto fold_witnesses(const error_set_of<ERRORS...> &errors, ACC init,
                              HANDLER &&handler) -> ACC {
    static_assert((detail::fold_arm_check<HANDLER, ACC, ERRORS>::value && ...));
    ACC acc = std::move(init);
    auto step = [&]<class E>(std::type_identity<E>) {
        if (errors.template holds<E>()) {
            acc = std::invoke(handler, std::move(acc),
                              *errors.template witness<E>());
        }
    };
    (step(std::type_identity<ERRORS>{}), ...);
    return acc;
}

// -- collapse ----------------------------------------------------------------

//! \mandates For every alternative `E` of the set, `handler` is invocable
//! with `const E&`. If `TARGET` is not named, every arm returns one type.
//! \effects Invokes `handler` on each present witness in canonical order and
//! folds the results with `combine`, which defaults to keeping the first.
//! \returns One `TARGET`.
//! \remarks This is the boundary verb: a graded region ends here and one error
//! type leaves. Pass a combine (`std::plus<>{}` on a string, say) when a
//! second witness must not be dropped.
template <class TARGET = void, class HANDLER, class COMBINE = detail::keep_first,
          class... ERRORS>
constexpr auto collapse(const error_set_of<ERRORS...> &errors, HANDLER &&handler,
                        COMBINE combine = {})
    -> detail::collapse_target_t<TARGET, HANDLER, ERRORS...> {
    using Result = detail::collapse_target_t<TARGET, HANDLER, ERRORS...>;
    std::optional<Result> acc;
    auto step = [&]<class E>(std::type_identity<E>) {
        if (errors.template holds<E>()) {
            Result r = std::invoke(handler, *errors.template witness<E>());
            acc = acc.has_value() ? std::invoke(combine, std::move(*acc), r)
                                  : std::move(r);
        }
    };
    (step(std::type_identity<ERRORS>{}), ...);
    return *acc; // the set is non-empty by invariant
}

//! \returns The computation's value, or its error collapsed to `TARGET`. The
//! result is the ungraded `std::expected<VALUE, TARGET>`: the grade is gone.
template <class TARGET = void, class VALUE, class... ERRORS, class HANDLER,
          class COMBINE = detail::keep_first>
constexpr auto
collapse(const std::expected<VALUE, error_set_of<ERRORS...>> &computation,
         HANDLER &&handler, COMBINE combine = {})
    -> std::expected<VALUE, detail::collapse_target_t<TARGET, HANDLER, ERRORS...>> {
    using Result = detail::collapse_target_t<TARGET, HANDLER, ERRORS...>;
    if (computation.has_value()) {
        return std::expected<VALUE, Result>(std::in_place, *computation);
    }
    return std::unexpected(collapse<TARGET>(computation.error(),
                                            std::forward<HANDLER>(handler),
                                            combine));
}

} // namespace beman::transpose

#endif
```

## The test as run

```cpp
#include "error_set_eliminators.hpp"

#include <beman/transpose.hpp>
#include <beman/transpose/overloaded.hpp>

#include <expected>
#include <iostream>
#include <string>
#include <vector>

namespace bt = beman::transpose;

struct parse_error { int position{}; };
struct range_error { int value{}; };
struct io_error    { int code{}; };

using exp_mixed = std::expected<int, bt::error_set<parse_error, range_error>>;
auto bad_parse(int at) -> exp_mixed { return exp_mixed{std::unexpect, parse_error{at}}; }
auto bad_range(int v)  -> exp_mixed { return exp_mixed{std::unexpect, range_error{v}}; }

auto describe(const parse_error& e) -> std::string { return "parse@" + std::to_string(e.position); }
auto describe(const range_error& e) -> std::string { return "range=" + std::to_string(e.value); }

int main() {
    auto add = [](int a, int b) { return a + b; };
    const auto& accumulating = bt::accumulating_applicative<exp_mixed>;
    auto both = accumulating.invoke(add, bad_parse(12), bad_range(99));
    auto one  = bt::applicative<exp_mixed>.invoke(add, bad_parse(12), bad_range(99));

    // 1. The report: fold every witness into a vector, arms by overload set.
    auto messages = bt::fold_witnesses(
        both.error(), std::vector<std::string>{},
        bt::overloaded{
            [](std::vector<std::string> acc, const parse_error& e) { acc.push_back(describe(e)); return acc; },
            [](std::vector<std::string> acc, const range_error& e) { acc.push_back(describe(e)); return acc; },
        });
    std::cout << "fold_witnesses: " << messages.size() << " messages:";
    for (auto& m : messages) std::cout << ' ' << m;
    std::cout << '\n';

    // 1b. A generic arm is enough when the alternatives share an interface.
    auto count = bt::fold_witnesses(both.error(), 0, [](int n, const auto&) { return n + 1; });
    std::cout << "fold_witnesses generic arm: count = " << count << '\n';

    // 2. The boundary: collapse to one error type, TARGET deduced from the arms.
    auto first = bt::collapse(both, bt::overloaded{
        [](const parse_error& e) { return describe(e); },
        [](const range_error& e) { return describe(e); },
    });
    static_assert(std::is_same_v<decltype(first), std::expected<int, std::string>>);
    std::cout << "collapse (keep first): " << first.error() << '\n';

    // 2b. With a combine, the second witness survives.
    auto joined = bt::collapse(both, [](const auto& e) { return describe(e) + ";"; }, std::plus<>{});
    std::cout << "collapse (std::plus): " << joined.error() << '\n';

    // 2c. TARGET named: arms return different things, each converts.
    auto code = bt::collapse<long>(both.error(), bt::overloaded{
        [](const parse_error& e) { return e.position; },   // int
        [](const range_error& e) { return short(e.value); }, // short
    });
    std::cout << "collapse<long>: " << code << '\n';

    // 2d. A single witness and a success pass through.
    std::cout << "collapse single: " << bt::collapse(one, [](const auto& e) { return describe(e); }).error() << '\n';
    auto ok = bt::collapse(exp_mixed{7}, [](const auto& e) { return describe(e); });
    std::cout << "collapse success: " << *ok << '\n';

    // 3. The verbs are constexpr.
    constexpr auto n = [] {
        bt::error_set<parse_error, range_error> e{parse_error{3}};
        return bt::fold_witnesses(e, 0, [](int acc, const auto&) { return acc + 1; });
    }();
    static_assert(n == 1);
    return 0;
}
```
