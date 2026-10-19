::: add

::: wording

## Applicative instance for array [transpose.array.applicative]{- .sref} {-}

```cpp
template<class VALUE> auto pure(this auto&&, VALUE&& value);
```

[x]{.pnum} *Returns*: An `array<remove_cvref_t<VALUE>, N>` each of whose `N` elements is a copy of `value`.

```cpp
template<class FUNCTION, class FIRST, class... REST>
auto invoke(this auto&&, FUNCTION&& function, FIRST&& first, REST&&... rest);
```

[x+1]{.pnum} *Constraints*: `$is-std-array-of-extent-v$<FIRST, N>` is `true`, `($is-std-array-of-extent-v$<REST, N> && ...)` is `true`, and `invocable<FUNCTION&, $element-ref-t$<FIRST>, $element-ref-t$<REST>...>` is satisfied.

[x+2]{.pnum} *Returns*: An `array` of `N` elements whose element at position `i` is the result of invoking `function` with the element at position `i` of each operand.

[x+3]{.pnum} *Complexity*: Exactly `N` applications of `function`.

[x+4]{.pnum} *Remarks*: Application is positional: operands are combined lane by lane. Every operand is constrained to have the same fixed extent `N`, and `function` to be invocable with one element of each, in the value category the operand was passed in. Each position of each operand is read exactly once, so an operand passed as an rvalue has its elements handed to `function` rather than copied.

:::

:::
