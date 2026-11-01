::: add

::: wording

## collapse [transpose.errset.collapse]{- .sref} {-}

```cpp
namespace std {
  struct keep_first {
    // @[transpose.errset.collapse]{- .sref}@, the default combine
    template<class T>
    [[nodiscard]] constexpr auto operator()(T first, const T&) const
        noexcept(is_nothrow_move_constructible_v<T>) -> T;
  };
}
```

[x]{.pnum} *Remarks*: `keep_first` is the default multiplicity rule of `collapse`: of two results, the first in canonical order is kept, matching `error_set_of::combined_with` and `recover`.

```cpp
template<class T>
[[nodiscard]] constexpr auto operator()(T first, const T&) const
    noexcept(is_nothrow_move_constructible_v<T>) -> T;
```

[x+1]{.pnum} *Returns*: `first`.

```cpp
template<class TARGET = void, class HANDLER, class COMBINE = keep_first,
         class... ERRORS>
[[nodiscard]] constexpr auto collapse(
    const error_set_of<ERRORS...>& errors, HANDLER&& handler,
    COMBINE combine =
        {}) noexcept($collapse-nothrow$<TARGET, HANDLER, COMBINE, ERRORS...>)
    -> $collapse-target-t$<TARGET, HANDLER, ERRORS...>;
```

[x+2]{.pnum} *Constraints*: `sizeof...(ERRORS) > 0` is `true`.

[x+3]{.pnum} *Mandates*: For every alternative `E` of the set, `handler` is invocable with `const E&`. If `TARGET` is `void`, every arm's decayed result is one type; otherwise each arm's result converts to `TARGET`.

[x+4]{.pnum} *Effects*: Invokes `handler` on each present witness in canonical order and folds the results with `combine`, which by default keeps the first.

[x+5]{.pnum} *Returns*: One `TARGET`, or when `TARGET` is `void` one value of the arms' common result type.

[x+6]{.pnum} *Remarks*: This is the boundary verb: a graded region ends here and one error type leaves. The set is non-empty by invariant, so there is always a result. Pass a combine when a second witness must not be dropped; the default is the left-biased pick of `combined_with`, which is the silent drop `visit`'s precondition refuses, made explicit by the name. The exception specification is `$collapse-nothrow$<TARGET, HANDLER, COMBINE, ERRORS...>`.

```cpp
template<class TARGET = void, class VALUE, class... ERRORS, class HANDLER,
         class COMBINE = keep_first>
[[nodiscard]] constexpr auto collapse(
    const expected<VALUE, error_set_of<ERRORS...>>& computation, HANDLER&& handler,
    COMBINE combine =
        {}) noexcept($collapse-nothrow$<TARGET, HANDLER, COMBINE, ERRORS...> &&
                     (is_void_v<VALUE> || is_nothrow_copy_constructible_v<VALUE>))
    -> expected<VALUE, $collapse-target-t$<TARGET, HANDLER, ERRORS...>>;
```

[x+7]{.pnum} *Constraints*: `sizeof...(ERRORS) > 0` is `true`.

[x+8]{.pnum} *Effects*: If `computation` holds a value, nothing; otherwise as for `collapse` on `computation.error()`.

[x+9]{.pnum} *Returns*: `expected<VALUE, T>`, where `T` is the type `collapse` on the error yields: holding `computation`'s value, or the collapsed error.

[x+10]{.pnum} *Remarks*: The result is the ungraded `expected<VALUE, T>`: the grade is gone. A caller who wants to stay graded names `collapse<error_set<T>>`, and the singleton set converts from `T` through the injecting constructor. The exception specification is that of `collapse` on the error, and of copying `computation`'s value into the result.

:::

:::
