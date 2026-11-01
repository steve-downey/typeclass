::: add

::: wording

## Registration as a grade model [transpose.errset.model]{- .sref} {-}

```cpp
namespace std {
  struct error_set_model {};
}
```

[x]{.pnum} *Remarks*: `error_set_model` names the model that the error-set grades belong to. It is what makes two grades comparable: grades of different models do not mix.

```cpp
namespace std {
  template<class... ERRORS>
  struct grade_model<error_set_of<ERRORS...>> {
    using type = error_set_model;
  };
}
```

```cpp
namespace std {
  template<class... LEFT, class... RIGHT>
  struct grade_join<error_set_of<LEFT...>, error_set_of<RIGHT...>> {
    using type = error_set_join_t<error_set_of<LEFT...>, error_set_of<RIGHT...>>;
  };
}
```

```cpp
namespace std {
  template<class... ERRORS>
  struct grade_bottom<error_set_of<ERRORS...>> {
    using type = error_set_bottom;
  };
}
```

```cpp
namespace std {
  template<class... LEFT, class... RIGHT>
  struct grade_subsumes<error_set_of<LEFT...>, error_set_of<RIGHT...>>
      : bool_constant<
            error_set_subsumes_v<error_set_of<LEFT...>, error_set_of<RIGHT...>>> {};
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR>
  struct grade_of<expected<VALUE, ERROR>> {
    using type = error_set<ERROR>;
  };
}
```

```cpp
namespace std {
  template<class VALUE, class... ERRORS>
  struct grade_of<expected<VALUE, error_set_of<ERRORS...>>> {
    using type = error_set_of<ERRORS...>;
  };
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR, class HEAD, class... TAIL>
  struct rebind_grade<expected<VALUE, ERROR>, error_set_of<HEAD, TAIL...>> {
    using type = expected<VALUE, error_set_of<HEAD, TAIL...>>;
  };
}
```

```cpp
namespace std {
  template<class VALUE, class HEAD, class... TAIL>
    requires(!$is-expected-v$<VALUE>)
  struct rebind_grade<VALUE, error_set_of<HEAD, TAIL...>> {
    using type = expected<VALUE, error_set_of<HEAD, TAIL...>>;
  };
}
```

```cpp
namespace std {
  template<class VALUE>
    requires(!$is-expected-v$<VALUE>)
  struct rebind_grade<VALUE, error_set_of<>> {
    using type = VALUE;
  };
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR>
  struct rebind_grade<expected<VALUE, ERROR>, error_set_of<>> {
    using type = VALUE;
  };
}
```

:::

:::
