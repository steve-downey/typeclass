::: add

```cpp
namespace std {
  template<class EXPECTED, class ERROR_TYPE>
  inline constexpr bool $is-expected-with-error-v$ = false; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  inline constexpr bool
      $is-expected-with-error-v$<expected<VALUE_TYPE, ERROR_TYPE>, ERROR_TYPE> =
          true; // exposition only
}
```

```cpp
namespace std {
  template<class EXPECTED>
  inline constexpr bool $is-void-expected-v$ = false; // exposition only
}
```

```cpp
namespace std {
  template<class ERROR_TYPE>
  inline constexpr bool $is-void-expected-v$<expected<void, ERROR_TYPE>> =
      true; // exposition only
}
```

```cpp
namespace std {
  template<class CARRIER>
  struct $carrier-value$ {
    using type = CARRIER;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR>
  struct $carrier-value$<expected<VALUE, ERROR>> {
    using type = VALUE;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class ERROR>
  struct $carrier-value$<expected<void, ERROR>> {
    using type = monostate;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class CARRIER>
  using $carrier-value-t$ =
      typename $carrier-value$<remove_cvref_t<CARRIER>>::type; // exposition only
}
```

```cpp
namespace std {
  template<class OPERAND>
  struct $expected-contained$ {}; // exposition only
}
```

```cpp
namespace std {
  template<class OPERAND>
    requires $is-expected-v$<remove_cvref_t<OPERAND>> &&
             (!$is-void-expected-v$<remove_cvref_t<OPERAND>>)
  struct $expected-contained$<OPERAND> {
    using type = $contained-ref-t$<OPERAND>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class OPERAND>
    requires $is-void-expected-v$<remove_cvref_t<OPERAND>>
  struct $expected-contained$<OPERAND> {
    using type = monostate;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class OPERAND>
  using $expected-contained-t$ =
      typename $expected-contained$<OPERAND>::type; // exposition only
}
```

```cpp
namespace std {
  template<class RESULT, class ERROR_TYPE>
  struct $expected-result$ {
    using type = expected<remove_cvref_t<RESULT>, ERROR_TYPE>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class ERROR_TYPE>
  struct $expected-result$<void, ERROR_TYPE> {
    using type = expected<void, ERROR_TYPE>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class RESULT, class ERROR_TYPE>
  using $expected-result-t$ =
      typename $expected-result$<RESULT, ERROR_TYPE>::type; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class VALUE, class ERROR_TYPE>
  struct $expected-pure$ {
    using type = expected<remove_cvref_t<VALUE>, ERROR_TYPE>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR_TYPE>
    requires is_same_v<remove_cvref_t<VALUE>, monostate>
  struct $expected-pure$<void, VALUE, ERROR_TYPE> {
    using type = expected<void, ERROR_TYPE>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class VALUE, class ERROR_TYPE>
  using $expected-pure-t$ =
      typename $expected-pure$<VALUE_TYPE, VALUE, ERROR_TYPE>::type; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class VALUE, class ERROR_TYPE>
  inline constexpr bool $expected-pure-nothrow$ =
      is_void_v<typename $expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>::value_type> ||
      is_nothrow_constructible_v<$expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>,
                                 VALUE>; // exposition only
}
```

::: wording

[x]{.pnum} *Remarks*: `$expected-pure-nothrow$<VALUE_TYPE, VALUE, ERROR_TYPE>` is `true` when `$expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>` can be constructed from `VALUE` without throwing; a `void` expected is constructed from nothing.

:::

```cpp
namespace std {
  template<class A>
  struct $bind-argument$ {
    using type = const A&;
  }; // exposition only
}
```

```cpp
namespace std {
  template<>
  struct $bind-argument$<void> {
    using type = monostate;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class A>
  using $bind-argument-t$ = typename $bind-argument$<A>::type; // exposition only
}
```

```cpp
namespace std {
  template<class F, class A>
  using $bind-result-t$ =
      remove_cvref_t<invoke_result_t<F, $bind-argument-t$<A>>>; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE, class ERROR>
  struct $raise-result$ {
    using type =
        $rebind-grade-t$<expected<VALUE_TYPE, ERROR_TYPE>,
                       $grade-join-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                                    $grade-of-t$<expected<VALUE_TYPE, ERROR>>>>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $raise-result$<VALUE_TYPE, ERROR_TYPE, ERROR_TYPE> {
    using type = expected<VALUE_TYPE, ERROR_TYPE>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE, class ERROR>
  using $raise-result-t$ =
      typename $raise-result$<VALUE_TYPE, ERROR_TYPE,
                            remove_cvref_t<ERROR>>::type; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE, class ERROR>
  concept $raisable$ = constructible_from<
      typename $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>::error_type,
      ERROR>; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE, class ERROR>
  inline constexpr bool $raise-nothrow$ =
      is_nothrow_constructible_v<
          typename $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>::error_type, ERROR> &&
      is_nothrow_constructible_v<
          $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>, unexpect_t,
          typename $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>::error_type>; // exposition
                                                                               // only
}
```

::: wording

[x+1]{.pnum} *Remarks*: `$raise-nothrow$<VALUE_TYPE, ERROR_TYPE, ERROR>` is `true` when the result's error alternative can be constructed from `ERROR`, and the result from that alternative, without throwing.

:::

```cpp
namespace std {
  template<class ERROR_TYPE, class... CARRIERS>
  inline constexpr bool $all-declare-v$ =
      ($is-expected-with-error-v$<remove_cvref_t<CARRIERS>, ERROR_TYPE> &&
       ...); // exposition only
}
```

```cpp
namespace std {
  template<class ERROR_TYPE, class CARRIER>
  inline constexpr bool $declares-or-bare-v$ =
      (!graded_context<remove_cvref_t<CARRIER>>) ||
      $is-expected-with-error-v$<remove_cvref_t<CARRIER>, ERROR_TYPE>; // exposition only
}
```

```cpp
namespace std {
  template<class ERROR_TYPE, class... CARRIERS>
  inline constexpr bool $all-declare-or-bare-v$ =
      ($declares-or-bare-v$<ERROR_TYPE, CARRIERS> && ...); // exposition only
}
```

```cpp
namespace std {
  template<class ERROR, class CARRIER>
  struct $error-widens-nothrow$ : true_type {}; // exposition only
}
```

```cpp
namespace std {
  template<class ERROR, class VALUE, class OPERAND_ERROR>
  struct $error-widens-nothrow$<ERROR, expected<VALUE, OPERAND_ERROR>>
      : bool_constant<is_nothrow_constructible_v<ERROR, const OPERAND_ERROR&>> {
  }; // exposition only
}
```

```cpp
namespace std {
  template<class RETURNED, class... CARRIERS>
  inline constexpr bool $expected-failure-nothrow$ =
      ($error-widens-nothrow$<typename RETURNED::error_type,
                            remove_cvref_t<CARRIERS>>::value &&
       ...) &&
      is_nothrow_move_constructible_v<typename RETURNED::error_type> &&
      is_nothrow_constructible_v<RETURNED, unexpect_t,
                                 typename RETURNED::error_type>; // exposition only
}
```

::: wording

[x+2]{.pnum} *Remarks*: `$expected-failure-nothrow$<RETURNED, CARRIERS...>` is `true` when the failure path of a short-circuiting composition yielding `RETURNED` is non-throwing: recording the first failing operand's error as `RETURNED`'s error alternative, moving it, and constructing `RETURNED` from it.

:::

```cpp
namespace std {
  template<class RETURNED, class... CARRIERS>
  inline constexpr bool $expected-accumulate-nothrow$ =
      $expected-failure-nothrow$<RETURNED, CARRIERS...> &&
      is_nothrow_move_assignable_v<typename RETURNED::error_type> &&
      noexcept(combine_grade_evidence(
          declval<const typename RETURNED::error_type&>(),
          declval<const typename RETURNED::error_type&>())); // exposition only
}
```

::: wording

[x+3]{.pnum} *Remarks*: `$expected-accumulate-nothrow$<RETURNED, CARRIERS...>` is `true` when the failure path of an accumulating composition yielding `RETURNED` is non-throwing: as for `$expected-failure-nothrow$`, and additionally combining two witnesses at `RETURNED`'s error alternative and assigning the combination.

:::

```cpp
namespace std {
  template<class ERROR>
  struct applicative_value<expected<void, ERROR>> {
    using type = monostate;
  };
}
```

::: wording

[x+4]{.pnum} *Remarks*: The element type of `std::expected<void, ERROR>` is `std::monostate`.

:::

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $expected-applicative-impl$ {
    // @[transpose.expected.applicative]{- .sref}@, applicative instance for expected
    template<class VALUE>
    [[nodiscard]] auto pure(this auto&&, VALUE&& value) noexcept(
        $expected-pure-nothrow$<VALUE_TYPE, VALUE, ERROR_TYPE>)
        -> $expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>;

    template<class ERROR>
      requires $raisable$<VALUE_TYPE, ERROR_TYPE, ERROR>
    [[nodiscard]] auto raise(this auto&&, ERROR&& error) noexcept(
        $raise-nothrow$<VALUE_TYPE, ERROR_TYPE, ERROR>)
        -> $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) && $all-declare-v$<ERROR_TYPE, CARRIERS...>
    [[nodiscard]] auto
    invoke(this auto&&, FUNCTION&& function, CARRIERS&&... operands) noexcept(
        $invokes-nothrow-into$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>,
                ERROR_TYPE>,
            FUNCTION&, $expected-contained-t$<CARRIERS>...> &&
        $expected-failure-nothrow$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>,
                ERROR_TYPE>,
            CARRIERS...>)
        -> $expected-result-t$<
            invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) &&
              ($is-expected-v$<remove_cvref_t<CARRIERS>> || ...) &&
              (!$all-declare-v$<ERROR_TYPE, CARRIERS...>) &&
              $all-declare-or-bare-v$<ERROR_TYPE, CARRIERS...>
    [[nodiscard]] auto
    invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
        $invokes-nothrow-into$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                ERROR_TYPE>,
            FUNCTION&, const $carrier-value-t$<CARRIERS>&...> &&
        $expected-failure-nothrow$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                ERROR_TYPE>,
            CARRIERS...>)
        -> $expected-result-t$<
            invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
            ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) &&
              ($is-expected-v$<remove_cvref_t<CARRIERS>> || ...) &&
              (!$all-declare-or-bare-v$<ERROR_TYPE, CARRIERS...>) &&
              ($mixes-with-model$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                                CARRIERS> &&
               ...)
    [[nodiscard]] auto
    invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
        $invokes-nothrow-into$<
            $mixed-result-t$<
                $grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                $expected-result-t$<
                    invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                    ERROR_TYPE>,
                CARRIERS...>,
            FUNCTION&, const $carrier-value-t$<CARRIERS>&...> &&
        $expected-failure-nothrow$<
            $mixed-result-t$<
                $grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                $expected-result-t$<
                    invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                    ERROR_TYPE>,
                CARRIERS...>,
            CARRIERS...>)
        -> $mixed-result-t$<
            $grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
            $expected-result-t$<
                invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                ERROR_TYPE>,
            CARRIERS...>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $expected-applicative-map$
      : derive_applicative<$expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>> {
    using $expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>::invoke;
    using $expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>::pure;
    using $expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>::raise;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $accumulating-expected-applicative-impl$ {
    // @[transpose.expected.accumulating]{- .sref}@, accumulating instance
    template<class VALUE>
    [[nodiscard]] auto pure(this auto&&, VALUE&& value) noexcept(
        $expected-pure-nothrow$<VALUE_TYPE, VALUE, ERROR_TYPE>)
        -> $expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>;

    template<class ERROR>
      requires $raisable$<VALUE_TYPE, ERROR_TYPE, ERROR>
    [[nodiscard]] auto raise(this auto&&, ERROR&& error) noexcept(
        $raise-nothrow$<VALUE_TYPE, ERROR_TYPE, ERROR>)
        -> $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) && $all-declare-v$<ERROR_TYPE, CARRIERS...>
    [[nodiscard]] auto
    invoke(this auto&&, FUNCTION&& function, CARRIERS&&... operands) noexcept(
        $invokes-nothrow-into$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>,
                ERROR_TYPE>,
            FUNCTION&, $expected-contained-t$<CARRIERS>...> &&
        $expected-accumulate-nothrow$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>,
                ERROR_TYPE>,
            CARRIERS...>)
        -> $expected-result-t$<
            invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) &&
              ($is-expected-v$<remove_cvref_t<CARRIERS>> || ...) &&
              (!$all-declare-v$<ERROR_TYPE, CARRIERS...>) &&
              $all-declare-or-bare-v$<ERROR_TYPE, CARRIERS...>
    [[nodiscard]] auto
    invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
        $invokes-nothrow-into$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                ERROR_TYPE>,
            FUNCTION&, const $carrier-value-t$<CARRIERS>&...> &&
        $expected-accumulate-nothrow$<
            $expected-result-t$<
                invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                ERROR_TYPE>,
            CARRIERS...>)
        -> $expected-result-t$<
            invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
            ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) &&
              ($is-expected-v$<remove_cvref_t<CARRIERS>> || ...) &&
              (!$all-declare-or-bare-v$<ERROR_TYPE, CARRIERS...>) &&
              ($mixes-with-model$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                                CARRIERS> &&
               ...)
    [[nodiscard]] auto
    invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
        $invokes-nothrow-into$<
            $mixed-result-t$<
                $grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                $expected-result-t$<
                    invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                    ERROR_TYPE>,
                CARRIERS...>,
            FUNCTION&, const $carrier-value-t$<CARRIERS>&...> &&
        $expected-accumulate-nothrow$<
            $mixed-result-t$<
                $grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                $expected-result-t$<
                    invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                    ERROR_TYPE>,
                CARRIERS...>,
            CARRIERS...>)
        -> $mixed-result-t$<
            $grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
            $expected-result-t$<
                invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>,
                ERROR_TYPE>,
            CARRIERS...>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $accumulating-expected-applicative-map$
      : derive_applicative<
            $accumulating-expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>> {
    using $accumulating-expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>::invoke;
    using $accumulating-expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>::pure;
    using $accumulating-expected-applicative-impl$<VALUE_TYPE, ERROR_TYPE>::raise;
  }; // exposition only
}
```

:::
