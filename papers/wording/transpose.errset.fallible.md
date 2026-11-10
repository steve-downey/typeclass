::: add

::: wording

## The uniform spelling [transpose.errset.fallible]{- .sref} {-}

```cpp
template<class VALUE>
  [[nodiscard]] constexpr auto fallible(VALUE&& value) noexcept(
      is_nothrow_constructible_v<remove_cvref_t<VALUE>, VALUE>) -> expected<remove_cvref_t<VALUE>, error_set<>>;
```

[x]{.pnum} *Constraints*: `remove_cvref_t<VALUE>` is not a specialization of `expected`.

[x+1]{.pnum} *Returns*: `expected<remove_cvref_t<VALUE>, error_set<>>` holding `value`.

[x+2]{.pnum} *Remarks*: This is the explicit spelling of a value at the empty grade: a computation that cannot fail, written in the fallible carrier so that the error-side operations apply to it, vacuously. The bare value remains the spelling of the empty grade everywhere the user does not write this. The exception specification is that of constructing the value from `value`.

```cpp
template<class VALUE, class ERROR>
  [[nodiscard]] constexpr auto fallible(expected<VALUE, ERROR> computation) noexcept(
      is_nothrow_move_constructible_v<expected<VALUE, ERROR>>) -> expected<VALUE, ERROR>;
```

[x+3]{.pnum} *Returns*: `computation`.

[x+4]{.pnum} *Remarks*: An `expected` is already in the carrier, at whatever grade it carries; `fallible` is the identity on it, so generic code can apply it to an operand that may or may not already be fallible. The exception specification is that of moving `computation`.

:::

:::
