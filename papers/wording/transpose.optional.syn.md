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
  template<class VALUE_TYPE>
  struct $optional-applicative-impl$ {
    // @[transpose.optional.applicative]{- .sref}@, applicative instance for optional
    template<class VALUE>
    constexpr auto pure(this auto&&, VALUE&& value) -> optional<remove_cvref_t<VALUE>>;

    template<class FUNCTION, class FIRST, class... REST>
      requires $is-optional-v$<FIRST> && ($is-optional-v$<REST> && ...) &&
               invocable<FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...> &&
               $optional-holdable-value$<invoke_result_t<FUNCTION&, $contained-ref-t$<FIRST>,
                                                       $contained-ref-t$<REST>...>>
    constexpr auto invoke(this auto&&, FUNCTION&& function, FIRST&& first, REST&&... rest)
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
  }; // exposition only
}
```

:::
