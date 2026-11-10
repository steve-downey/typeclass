::: add

::: wording

## Lattice operations [transpose.errset.lattice]{- .sref} {-}

```cpp
template<class LEFT, class RIGHT>
  struct error_set_join;
```

[x]{.pnum} *Remarks*: `error_set_join<LEFT, RIGHT>::type` is the union of two error sets. Canonicalization reduces the union to pack concatenation.

```cpp
namespace std {
  template<class... LEFT, class... RIGHT>
  struct error_set_join<error_set_of<LEFT...>, error_set_of<RIGHT...>> {
    using type = error_set<LEFT..., RIGHT...>;
  };
}
```

```cpp
using error_set_bottom = error_set<>;
```

[x+1]{.pnum} *Remarks*: `error_set_bottom` is the identity of join: the empty set.

```cpp
template<class LEFT, class RIGHT>
  inline constexpr bool error_set_subsumes_v = false;
```

[x+2]{.pnum} *Remarks*: `error_set_subsumes_v<LEFT, RIGHT>` is `true` when every alternative of `LEFT` is an alternative of `RIGHT`. By order-from-join this holds exactly when the join of `LEFT` and `RIGHT` is `RIGHT`, so the only implicit widenings are subset inclusions: an error set can widen to a superset and never the other way.

```cpp
template<class... ERRORS>
  [[nodiscard]] constexpr auto error_set_combine(
      const error_set_of<ERRORS...>& lhs, const error_set_of<ERRORS...>& rhs) noexcept(noexcept(lhs.combined_with(rhs)))
      -> error_set_of<ERRORS...>;
template<class... ERRORS>
  constexpr auto combine_grade_evidence(const error_set_of<ERRORS...>& lhs,
                                        const error_set_of<ERRORS...>& rhs) noexcept(noexcept(error_set_combine(lhs,
                                                                                                                rhs)))
      -> error_set_of<ERRORS...>;
```

[x+3]{.pnum} *Effects*: Equivalent to:

```cpp
return lhs.combined_with(rhs);
```

[x+4]{.pnum} *Returns*: The combination of `lhs` and `rhs` at their common grade.

[x+5]{.pnum} *Remarks*: Combination is left-biased per alternative: where both operands witness the same error type, `lhs`'s witness is kept. The type-level join above is a commutative union and needs no semigroup; this value-level combination is what the accumulating applicative object uses.

```cpp
namespace std {
  template<class RESULT, class VALUE>
  struct raised_by_result {
    using type = $type-list$<>;
  };
}
```

```cpp
namespace std {
  template<class VALUE, class... RS>
  struct raised_by_result<expected<VALUE, error_set_of<RS...>>, VALUE> {
    using type = $type-list$<RS...>;
  };
}
```

```cpp
namespace std {
  template<class HANDLER, class VALUE, class H>
  struct handler_result_of {
    // `HANDLER &`: apply_handler takes the handler by lvalue reference and
    // invokes it in that category.
    using type = remove_cvref_t<invoke_result_t<HANDLER&, const H&>>;
  };
}
```

```cpp
namespace std {
  template<class HANDLER, class VALUE, class... ERRORS, class... HANDLED>
  struct recover_final_grade<HANDLER, VALUE, error_set_of<ERRORS...>, HANDLED...> {
    using remainder = $list-subtract-t$<$type-list$<HANDLED...>, $type-list$<ERRORS...>>;
    using raised = $list-concat-all-t$<typename raised_by_result<
        typename handler_result_of<HANDLER, VALUE, HANDLED>::type, VALUE>::type...>;
    using type = error_set_of_list_t<$list-concat-all-t$<remainder, raised>>;
  };
}
```

```cpp
namespace std {
  template<class HANDLER, class VALUE, class GRADE, class... HANDLED>
  using $recover-return-t$ =
      $rebind-grade-t$<VALUE, recover_final_grade_t<HANDLER, VALUE, GRADE,
                                                  HANDLED...>>; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class VALUE, class GRADE, class... HANDLED>
  struct $recover-nothrow$; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class VALUE, class... ERRORS, class... HANDLED>
  struct $recover-nothrow$<HANDLER, VALUE, error_set_of<ERRORS...>, HANDLED...> {
    using final_grade =
        recover_final_grade_t<HANDLER, VALUE, error_set_of<ERRORS...>, HANDLED...>;
    using returned =
        $recover-return-t$<HANDLER, VALUE, error_set_of<ERRORS...>, HANDLED...>;
    static constexpr bool value =
        (is_nothrow_invocable_v<HANDLER&, const HANDLED&> && ...) &&
        (is_nothrow_copy_constructible_v<ERRORS> && ...) &&
        is_nothrow_copy_constructible_v<final_grade> &&
        is_nothrow_move_constructible_v<final_grade> &&
        is_nothrow_copy_constructible_v<VALUE> &&
        is_nothrow_move_constructible_v<VALUE> &&
        is_nothrow_constructible_v<returned, const VALUE&> &&
        (is_same_v<returned, VALUE> ||
         is_nothrow_constructible_v<returned, unexpect_t, const final_grade&>);
  }; // exposition only
}
```

```cpp
namespace std {
  template<class HANDLER, class VALUE, class GRADE, class... HANDLED>
  inline constexpr bool $recover-nothrow-v$ =
      $recover-nothrow$<HANDLER, VALUE, GRADE, HANDLED...>::value; // exposition only
}
```

[x+6]{.pnum} *Remarks*: `$recover-nothrow-v$<HANDLER, VALUE, GRADE, HANDLED...>` is `true` when invoking `HANDLER` on each handled alternative, copying the value and every witness that survives, and constructing the result are all non-throwing.

```cpp
namespace std {
  template<class HANDLER, class VALUE, class GRADE, class... HANDLED>
  struct $recover-return$ {
    using type = $recover-return-t$<HANDLER, VALUE, GRADE, HANDLED...>;
  }; // exposition only
}
```

:::

:::
