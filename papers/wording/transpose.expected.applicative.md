::: add

::: wording

## Applicative instance for expected [transpose.expected.applicative]{- .sref} {-}

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

[x+1]{.pnum} *Constraints*: The error alternative of the result is constructible from `error`.

[x+2]{.pnum} *Returns*: An `expected` holding `error` as its error, at the grade that is the join of this object's grade with the grade `error`'s type alone would carry. When that type is `ERROR_TYPE` itself the result is `expected<VALUE_TYPE, ERROR_TYPE>`, carried verbatim.

[x+3]{.pnum} *Remarks*: This is the dual of `pure`: the generic entry into the failure state, so that generic graded code can fail without naming the carrier. Carrier spelling stays lazy, as for the cores: raising the declared alternative leaves the spelling alone, and only a genuinely different alternative widens it into an error set. `error` may itself be an error set, whose members join.

```cpp
template<class FUNCTION, class... CARRIERS>
  [[nodiscard]] auto invoke(this auto&&, FUNCTION&& function, CARRIERS&&... operands) noexcept(
      $invokes-nothrow-into$<$expected-result-t$<invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>,
                           FUNCTION&, $expected-contained-t$<CARRIERS>...> &&
      $expected-failure-nothrow$<
          $expected-result-t$<invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>, CARRIERS...>)
      -> $expected-result-t$<invoke_result_t<FUNCTION&, $expected-contained-t$<CARRIERS>...>, ERROR_TYPE>;
```

[x+4]{.pnum} *Constraints*: `sizeof...(CARRIERS) > 0` is `true` and `$all-declare-v$<ERROR_TYPE, CARRIERS...>` is `true`.

[x+5]{.pnum} *Returns*: If every operand holds a value, an `expected` holding the result of invoking `function` with those values, in the order written; otherwise an `expected` holding the error of the first operand, in that same order, that does not hold a value.

[x+6]{.pnum} *Remarks*: `function` is invoked at most once. Composition short-circuits: only the first error is observed. Each operand's held value is passed on with that operand's own value category, so a caller that hands over an rvalue operand has its value moved from rather than copied.

```cpp
template<class FUNCTION, class... CARRIERS>
  [[nodiscard]] auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
      $invokes-nothrow-into$<
          $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>, FUNCTION&,
          const $carrier-value-t$<CARRIERS>&...> &&
      $expected-failure-nothrow$<
          $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>, CARRIERS...>)
      -> $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>;
```

[x+7]{.pnum} *Constraints*: At least one operand is an `expected`; the operands do not all declare `ERROR_TYPE`; and every operand either declares `ERROR_TYPE` or is a bare value.

[x+8]{.pnum} *Returns*: As for the preceding overload, treating a bare operand as holding itself.

[x+9]{.pnum} *Remarks*: This is not a mixing point. A bare operand is ungraded, lifts to the model bottom, and leaves no trace in the deduced type.

```cpp
template<class FUNCTION, class... CARRIERS>
  [[nodiscard]] auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands) noexcept(
      $invokes-nothrow-into$<
          $mixed-result-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                         $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>,
                         CARRIERS...>,
          FUNCTION&, const $carrier-value-t$<CARRIERS>&...> &&
      $expected-failure-nothrow$<
          $mixed-result-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                         $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>,
                         CARRIERS...>,
          CARRIERS...>)
      -> $mixed-result-t$<$grade-of-t$<expected<VALUE_TYPE, ERROR_TYPE>>,
                        $expected-result-t$<invoke_result_t<FUNCTION&, const $carrier-value-t$<CARRIERS>&...>, ERROR_TYPE>,
                        CARRIERS...>;
```

[x+10]{.pnum} *Constraints*: At least one operand is an `expected`; the operands do not all declare `ERROR_TYPE` or a bare value; and every operand's grade belongs to this instance's grade model.

[x+11]{.pnum} *Returns*: As for the preceding overloads, at the grade that is the join of the operands' grades.

[x+12]{.pnum} *Remarks*: This is the mixing point, reached exactly when the preceding overload's constraint does not hold; the two are complementary, so the choice is never a matter of overload ranking. Carrier spelling stays lazy: operands that declare the same error alternative keep that spelling, only genuinely different ones join into an error set, and a bare operand lifts to the model bottom without trace. Combinations that were previously ill-formed become well-formed; nothing that deduced a type before deduces a different one now.

:::

:::
