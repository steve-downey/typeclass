::: add

::: wording

## Accumulating instance [transpose.expected.accumulating]{- .sref} {-}

```cpp
template<class VALUE>
  [[nodiscard]] auto pure(this auto&&, VALUE&& value) noexcept($expected-pure-nothrow$<VALUE_TYPE, VALUE, ERROR_TYPE>)
      -> $expected-pure-t$<VALUE_TYPE, VALUE, ERROR_TYPE>;
```

[x]{.pnum} *Returns*: An `expected` holding `value`.

```cpp
template<class ERROR>
  [[nodiscard]] auto raise(this auto&&, ERROR&& error) noexcept($raise-nothrow$<VALUE_TYPE, ERROR_TYPE, ERROR>)
      -> $raise-result-t$<VALUE_TYPE, ERROR_TYPE, ERROR>;
```

[x+1]{.pnum} *Constraints*: As for the short-circuit object's `raise`.

[x+2]{.pnum} *Returns*: As for the short-circuit object's `raise`: the two objects share the carrier, so they share the entry into its failure state.

```cpp
template<class FUNCTION, class... CARRIERS>
  [[nodiscard]] auto invoke(this auto&&, FUNCTION&& function, CARRIERS&&... operands) noexcept(
      $invokes-nothrow-into$<$expected-result-t$<invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>,
                           FUNCTION&, $expected-contained-t$<CARRIERS>...> &&
      $expected-accumulate-nothrow$<
          $expected-result-t$<invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>, CARRIERS...>)
      -> $expected-result-t$<invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>;
```

[x+3]{.pnum} *Constraints*: `sizeof...(CARRIERS) > 0` is `true` and `$all-declare-v$<ERROR_TYPE, CARRIERS...>` is `true`.

[x+4]{.pnum} *Returns*: If every operand holds a value, an `expected` holding the result of invoking `function` with those values, in the order written; otherwise an `expected` holding the combination of every operand's error, combined in that same order.

[x+5]{.pnum} *Remarks*: `function` is invoked at most once. Composition accumulates: every error is observed, not only the first. Each operand's held value is passed on with that operand's own value category, so a caller that hands over an rvalue operand has its value moved from rather than copied.

```cpp
template<class FUNCTION, class... CARRIERS>
  [[nodiscard]] auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
      $invokes-nothrow-into$<
          $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>, FUNCTION&,
          const $carrier-value-t$<CARRIERS>&...> &&
      $expected-accumulate-nothrow$<
          $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>, CARRIERS...>)
      -> $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>;
```

[x+6]{.pnum} *Constraints*: At least one operand is an `expected`; the operands do not all declare `ERROR_TYPE`; and every operand either declares `ERROR_TYPE` or is a bare value.

[x+7]{.pnum} *Returns*: As for the preceding overload, treating a bare operand as holding itself.

```cpp
template<class FUNCTION, class... CARRIERS>
  [[nodiscard]] auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
      $invokes-nothrow-into$<
          $mixed-result-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                         $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>,
                         CARRIERS...>,
          FUNCTION&, const $carrier-value-t$<CARRIERS>&...> &&
      $expected-accumulate-nothrow$<
          $mixed-result-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                         $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>,
                         CARRIERS...>,
          CARRIERS...>)
      -> $mixed-result-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                        $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>,
                        CARRIERS...>;
```

[x+8]{.pnum} *Constraints*: At least one operand is an `expected`; the operands do not all declare `ERROR_TYPE` or a bare value; and every operand's grade belongs to this instance's grade model.

[x+9]{.pnum} *Returns*: As for the preceding overloads, at the grade that is the join of the operands' grades, holding the combination of every failing operand's evidence.

[x+10]{.pnum} *Remarks*: Errors are combined in the order the operands are written.

:::

:::
