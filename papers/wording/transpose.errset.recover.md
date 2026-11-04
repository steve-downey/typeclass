::: add

::: wording

## recover [transpose.errset.recover]{- .sref} {-}

```cpp
template<class... HANDLED, class VALUE, class... ERRORS, class HANDLER>
[[nodiscard]] constexpr auto recover(
    const expected<VALUE, error_set_of<ERRORS...>>& computation,
    HANDLER&& handler) noexcept($recover-nothrow-v$<HANDLER, VALUE,
                                                  error_set_of<ERRORS...>, HANDLED...>)
    -> $see below$;
```

[x]{.pnum} *Constraints*: `sizeof...(ERRORS) > 0` is `true`.

[x+1]{.pnum} *Mandates*: `HANDLED` is not empty; every type in `HANDLED` is one of the computation's alternatives; and for each such type `handler` returns either `VALUE` or a `std::expected<VALUE, error_set<...>>`.

[x+2]{.pnum} *Effects*: For each type in `HANDLED` that the computation's error witnesses, invokes `handler` with that witness. A handler either recovers a value or raises an error set of its own. Witnesses for types outside `HANDLED`, and any a handler raises, survive into the result.

[x+3]{.pnum} *Returns*: The computation at the narrowed grade: the alternatives it may raise, less those handled, plus any a handler raises. The result holds a value when every witness present was recovered, and otherwise the surviving witnesses.

[x+4]{.pnum} *Remarks*: The handled set is annotated rather than inferred: it is given as explicit template arguments and checked against the computation's grade. Recovering one alternative from a value that raised two leaves the other, which is still an error. If more than one handled type is witnessed and each independently recovers a value, the first in canonical order is the result, matching the tie-break of `combined_with`. The exception specification is `$recover-nothrow-v$<HANDLER, VALUE, error_set_of<ERRORS...>, HANDLED...>`.

```cpp
template<class... HANDLED, class VALUE, class HANDLER>
[[nodiscard]] constexpr auto recover(
    const expected<VALUE, error_set_of<>>& computation,
    HANDLER&&) noexcept(is_nothrow_copy_constructible_v<expected<VALUE,
                                                                 error_set_of<>>>)
    -> expected<VALUE, error_set_of<>>;
```

[x+5]{.pnum} *Constraints*: The computation's grade is the empty error set.

[x+6]{.pnum} *Mandates*: `sizeof...(HANDLED) > 0` is `true`.

[x+7]{.pnum} *Effects*: None. `handler` is not invoked, and no requirement is placed on it.

[x+8]{.pnum} *Returns*: `computation`.

[x+9]{.pnum} *Remarks*: At the empty grade no alternative can be witnessed, so there is nothing to recover from and nothing to check `HANDLED` against. This is what keeps a generic recovery step well-formed when an earlier stage has statically eliminated every failure. The exception specification is that of copying `computation`.

```cpp
namespace std {
  template<class T> inline constexpr bool $is-expected-v$ = false; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR>
  inline constexpr bool $is-expected-v$<expected<VALUE, ERROR>> = true; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class... ARGS>
  concept $witness-arm-for$ =
      invocable<HANDLER&, ARGS...> &&
      !is_void_v<invoke_result_t<HANDLER&, ARGS...>>; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class ACC, class E>
  struct $fold-arm-check$ {
    static constexpr bool value = true;
  }; // exposition only
}
```

[x+10]{.pnum} A program that instantiates `fold_arm_check<HANDLER, ACC, E>` is ill-formed unless `$witness-arm-for$<HANDLER, ACC&&, const E&>` is satisfied and `is_invocable_r_v<ACC, HANDLER&, ACC&&, const E&>` is `true`.

```cpp
namespace std {
  template<class HANDLER, class E>
  struct $collapse-arm-check$ {
    using type = remove_cvref_t<invoke_result_t<HANDLER&, const E&>>;
  }; // exposition only
}
```

[x+11]{.pnum} A program that instantiates `collapse_arm_check<HANDLER, E>` is ill-formed unless `$witness-arm-for$<HANDLER, const E&>` is satisfied.

```cpp
namespace std {
  template<class HANDLER, class... ERRORS>
  struct $common-arm-result$; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class FIRST, class... REST>
  struct $common-arm-result$<HANDLER, FIRST, REST...> {
    using type = typename $collapse-arm-check$<HANDLER, FIRST>::type;
  }; // exposition only
}
```

[x+12]{.pnum} A program that instantiates `common_arm_result` is ill-formed unless `(is_same_v<type, typename $collapse-arm-check$<HANDLER, REST>::type> && ...)` is `true`.

```cpp
namespace std {
  template<class TARGET, class HANDLER, class... ERRORS>
  struct $collapse-target$ {
    using type = TARGET;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class... ERRORS>
  struct $collapse-target$<void, HANDLER, ERRORS...>
      : $common-arm-result$<HANDLER, ERRORS...> {}; // exposition only
}
```

```cpp
namespace std {
  template<class TARGET, class HANDLER, class... ERRORS>
  using $collapse-target-t$ =
      typename $collapse-target$<TARGET, HANDLER, ERRORS...>::type; // exposition only
}
```

```cpp
namespace std {
  template<class TARGET, class HANDLER, class COMBINE, class... ERRORS>
  inline constexpr bool $collapse-nothrow$ =
      (is_nothrow_invocable_r_v<$collapse-target-t$<TARGET, HANDLER, ERRORS...>, HANDLER&,
                                const ERRORS&> &&
       ...) &&
      is_nothrow_invocable_r_v<$collapse-target-t$<TARGET, HANDLER, ERRORS...>, COMBINE&,
                               $collapse-target-t$<TARGET, HANDLER, ERRORS...>,
                               $collapse-target-t$<TARGET, HANDLER, ERRORS...>> &&
      is_nothrow_move_constructible_v<$collapse-target-t$<TARGET, HANDLER, ERRORS...>> &&
      is_nothrow_move_assignable_v<
          $collapse-target-t$<TARGET, HANDLER, ERRORS...>>; // exposition only
}
```

[x+13]{.pnum} *Remarks*: `$collapse-nothrow$<TARGET, HANDLER, COMBINE, ERRORS...>` is `true` when invoking `HANDLER` on each alternative and converting its result to the target, combining two results with `COMBINE`, and moving a result into place are all non-throwing.

:::

:::
