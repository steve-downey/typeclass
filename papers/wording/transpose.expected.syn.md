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
  struct $expected-contained$ {
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
  template<class ERROR>
  struct applicative_value<expected<void, ERROR>> {
    using type = monostate;
  };
}
```

::: wording

[x]{.pnum} *Remarks*: The element type of `std::expected<void, ERROR>` is `std::monostate`.

:::

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $expected-applicative-impl$ {
    // @[transpose.expected.applicative]{- .sref}@, applicative instance for expected
    template<class VALUE>
    auto pure(this auto&&, VALUE&& value)
        -> $expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) && $all-declare-v$<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto&&, FUNCTION&& function, CARRIERS&&... operands)
        -> $expected-result-t$<
            invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) &&
              ($is-expected-v$<remove_cvref_t<CARRIERS>> || ...) &&
              (!$all-declare-v$<ERROR_TYPE, CARRIERS...>) &&
              $all-declare-or-bare-v$<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
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
    auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
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
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE, class ERROR_TYPE>
  struct $accumulating-expected-applicative-impl$ {
    // @[transpose.expected.accumulating]{- .sref}@, accumulating instance
    template<class VALUE>
    auto pure(this auto&&, VALUE&& value)
        -> $expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) && $all-declare-v$<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto&&, FUNCTION&& function, CARRIERS&&... operands)
        -> $expected-result-t$<
            invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>;

    template<class FUNCTION, class... CARRIERS>
      requires(sizeof...(CARRIERS) > 0) &&
              ($is-expected-v$<remove_cvref_t<CARRIERS>> || ...) &&
              (!$all-declare-v$<ERROR_TYPE, CARRIERS...>) &&
              $all-declare-or-bare-v$<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
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
    auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
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
  }; // exposition only
}
```

:::
