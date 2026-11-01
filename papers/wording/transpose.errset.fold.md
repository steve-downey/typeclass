::: add

::: wording

## fold_witnesses [transpose.errset.fold]{- .sref} {-}

```cpp
template<class ACC, class HANDLER, class... ERRORS>
constexpr auto fold_witnesses(const error_set_of<ERRORS...>& errors, ACC init,
                              HANDLER&& handler) -> ACC;
```

[x]{.pnum} *Mandates*: For every alternative `E` of the set, `handler` is invocable with `(ACC&&, const E&)` and the result converts to `ACC`.

[x+1]{.pnum} *Effects*: For each alternative the set witnesses, in canonical order, replaces the accumulator with `handler(std::move(acc), witness)`, starting from `init`.

[x+2]{.pnum} *Returns*: The final accumulator.

[x+3]{.pnum} *Remarks*: Multiplicity is the caller's stated business: the accumulator is where two witnesses meet, and nothing is dropped unless an arm drops it. An alternative no arm accepts is ill-formed; the diagnostic names it.

:::

:::
