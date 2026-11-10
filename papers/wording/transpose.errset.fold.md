::: add

::: wording

## fold_witnesses [transpose.errset.fold]{- .sref} {-}

```cpp
template<class ACC, class HANDLER, class... ERRORS>
  [[nodiscard]] constexpr auto fold_witnesses(
      const error_set_of<ERRORS...>& errors, ACC init,
      HANDLER&& handler) noexcept((is_nothrow_invocable_r_v<ACC, HANDLER&, ACC&&, const ERRORS&> && ...) &&
                                  is_nothrow_move_constructible_v<ACC> && is_nothrow_move_assignable_v<ACC>) -> ACC;
```

[x]{.pnum} *Mandates*: For every alternative `E` of the set, `handler` is invocable with `(ACC&&, const E&)` and the result converts to `ACC`.

[x+1]{.pnum} *Effects*: For each alternative the set witnesses, in canonical order, replaces the accumulator with `handler(std::move(acc), witness)`, starting from `init`.

[x+2]{.pnum} *Returns*: The final accumulator.

[x+3]{.pnum} *Remarks*: Multiplicity is the caller's stated business: the accumulator is where two witnesses meet, and nothing is dropped unless an arm drops it. An alternative no arm accepts is ill-formed; the diagnostic names it. The exception specification is that of each arm's invocation and conversion to `ACC`, and of moving the accumulator.

:::

:::
