::: add

```cpp
namespace std {
  template<class T>
  struct $is-optional$ : false_type {}; // exposition only
}
```

```cpp
namespace std {
  template<class U>
  struct $is-optional$<optional<U>> : true_type {}; // exposition only
}
```

```cpp
namespace std {
  template<class T>
  inline constexpr bool $is-optional-v$ =
      $is-optional$<remove_cvref_t<T>>::value; // exposition only
}
```

```cpp
namespace std {
  template<class RESULT>
  concept $optional-holdable-value$ =
      $holdable-value$<RESULT> && !same_as<remove_cvref_t<RESULT>, nullopt_t> &&
      !same_as<remove_cvref_t<RESULT>, in_place_t>; // exposition only
}
```

```cpp
namespace std {
  template<class FUNCTION, class FIRST, class... REST>
  inline constexpr bool $optional-invoke-nothrow$ = $invokes-nothrow-into$<
      optional<remove_cvref_t<invoke_result_t<FUNCTION&, $contained-ref-t$<FIRST>,
                                              $contained-ref-t$<REST>...>>>,
      FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...>; // exposition only
}
```

::: wording

[x]{.pnum} *Remarks*: `$optional-invoke-nothrow$<FUNCTION, FIRST, REST...>` is `true` when invoking `FUNCTION` on the values the operands hold, each in its operand's value category, and constructing an `optional` from the result are both non-throwing.

:::

```cpp
namespace std {
  template<class VALUE_TYPE>
  struct $optional-applicative-impl$ {
    // @[transpose.optional.applicative]{- .sref}@, applicative instance for optional
    template<class VALUE>
    [[nodiscard]] constexpr auto pure(this auto&&, VALUE&& value) noexcept(
        is_nothrow_constructible_v<optional<remove_cvref_t<VALUE>>, VALUE>)
        -> optional<remove_cvref_t<VALUE>>;

    [[nodiscard]] constexpr auto zero(this auto&&) noexcept -> optional<VALUE_TYPE>;

    template<class FUNCTION, class FIRST, class... REST>
      requires $is-optional-v$<FIRST> && ($is-optional-v$<REST> && ...) &&
               invocable<FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...> &&
               $optional-holdable-value$<invoke_result_t<
                   FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...>>
    [[nodiscard]] constexpr auto invoke(
        this auto&&, FUNCTION&& function, FIRST&& first,
        REST&&... rest) noexcept($optional-invoke-nothrow$<FUNCTION, FIRST, REST...>)
        -> optional<remove_cvref_t<invoke_result_t<FUNCTION&, $contained-ref-t$<FIRST>,
                                                   $contained-ref-t$<REST>...>>>;
  }; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE_TYPE>
  struct $optional-applicative-map$
      : derive_applicative<$optional-applicative-impl$<VALUE_TYPE>> {
    using $optional-applicative-impl$<VALUE_TYPE>::invoke;
    using $optional-applicative-impl$<VALUE_TYPE>::pure;
    using $optional-applicative-impl$<VALUE_TYPE>::zero;
  }; // exposition only
}
```

:::
