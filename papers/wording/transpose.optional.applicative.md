::: add

::: wording

## Applicative instance for optional [transpose.optional.applicative]{- .sref} {-}

```cpp
template<class VALUE>
[[nodiscard]] constexpr auto pure(this auto&&, VALUE&& value) noexcept(
    is_nothrow_constructible_v<optional<remove_cvref_t<VALUE>>, VALUE>)
    -> optional<remove_cvref_t<VALUE>>;
```

[x]{.pnum} *Returns*: An engaged `optional` holding `value`.

```cpp
[[nodiscard]] constexpr auto zero(this auto&&) noexcept -> optional<VALUE_TYPE>;
```

[x+1]{.pnum} *Returns*: A disengaged `optional<VALUE_TYPE>`.

[x+2]{.pnum} *Remarks*: The failure state of `optional` carries no witness, so `optional` has a `zero`; it is the identity of the first-engaged disjunction that `first_monoid` already folds with.

```cpp
template<class FUNCTION, class FIRST, class... REST>
[[nodiscard]] constexpr auto invoke(
    this auto&&, FUNCTION&& function, FIRST&& first,
    REST&&... rest) noexcept($optional-invoke-nothrow$<FUNCTION, FIRST, REST...>)
    -> optional<remove_cvref_t<
        invoke_result_t<FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...>>>;
```

[x+3]{.pnum} *Constraints*:

- [x+3.#]{.pnum} `$is-optional-v$<FIRST>` is `true`,
- [x+3.#]{.pnum} `($is-optional-v$<REST> && ...)` is `true`,
- [x+3.#]{.pnum} `invocable<FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...>` is satisfied,
- [x+3.#]{.pnum} `invoke_result_t<FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...>` models `$optional-holdable-value$`.

[x+4]{.pnum} *Returns*: If every operand is engaged, an engaged `optional` holding the result of invoking `function` with the contained values, in the order written; otherwise a disengaged `optional`.

[x+5]{.pnum} *Remarks*: `function` is invoked at most once. Each operand's held value is passed on with that operand's own value category, so a caller that hands over an rvalue operand has its value moved from rather than copied. This is what lets a traversal carry its accumulated result forward without duplicating it at every step.

:::

:::
