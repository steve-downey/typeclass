::: add

::: wording

## Operation objects [transpose.applicative.objects]{- .sref} {-}

```cpp
inline constexpr $unspecified$ invoke;
```

[x]{.pnum} *Constraints*: For a call `invoke(function, first_argument, rest_arguments...)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.invoke(function, first_argument, rest_arguments...)` is a valid expression.

[x+1]{.pnum} *Effects*: Equivalent to `applicative<C>.invoke(function, first_argument, rest_arguments...)`, with each argument forwarded in its own value category.

[x+2]{.pnum} *Remarks*: The name `invoke` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The context is deduced from the first in-context argument; every operand shares it. The exception specification is equivalent to `noexcept(applicative<C>.invoke(function, first_argument, rest_arguments...))`.

```cpp
inline constexpr $unspecified$ ap;
```

[x+3]{.pnum} *Constraints*: For a call `ap(function, argument)`, let `C` be `remove_cvref_t<decltype((argument))>`. `applicative<C>.ap(function, argument)` is a valid expression.

[x+4]{.pnum} *Effects*: Equivalent to `applicative<C>.ap(function, argument)`, with each argument forwarded in its own value category.

[x+5]{.pnum} *Remarks*: The name `ap` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The context is deduced from the argument in context, not from the callable in context. The exception specification is equivalent to `noexcept(applicative<C>.ap(function, argument))`.

```cpp
template<class CONTEXT>
  inline constexpr $unspecified$ pure;
```

[x+6]{.pnum} *Constraints*: For a call `pure<CONTEXT>(value)`, let `C` be `remove_cvref_t<CONTEXT>`. `applicative<C>.pure(value)` is a valid expression.

[x+7]{.pnum} *Effects*: Equivalent to `applicative<C>.pure(value)`, with `value` forwarded in its own value category.

[x+8]{.pnum} *Remarks*: The name `pure` denotes a family of operation objects, one per context. None is found by argument-dependent lookup, and `CONTEXT` is the only template argument a call gives. The context is named rather than deduced: `pure` builds a context instead of consuming one, so no argument carries it. The exception specification is equivalent to `noexcept(applicative<C>.pure(value))`.

```cpp
template<class CONTEXT>
  inline constexpr $unspecified$ raise;
```

[x+9]{.pnum} *Constraints*: For a call `raise<CONTEXT>(error)`, let `C` be `remove_cvref_t<CONTEXT>`. `applicative<C>.raise(error)` is a valid expression.

[x+10]{.pnum} *Effects*: Equivalent to `applicative<C>.raise(error)`, with `error` forwarded in its own value category.

[x+11]{.pnum} *Remarks*: The name `raise` denotes a family of operation objects, one per context, the dual of `pure`: where `pure` enters the context's success state with a value, `raise` enters its failure state with a witness. The context is named for the reason `pure`'s is: the operation builds a context rather than consuming one. The grade of the result is the object's business; for the error-set model it is the join of the context's grade with the grade `error`'s type alone would carry, spelled lazily. The exception specification is equivalent to `noexcept(applicative<C>.raise(error))`.

```cpp
template<class CONTEXT>
  inline constexpr $unspecified$ zero;
```

[x+12]{.pnum} *Constraints*: For a call `zero<CONTEXT>()`, let `C` be `remove_cvref_t<CONTEXT>`. `applicative<C>.zero()` is a valid expression.

[x+13]{.pnum} *Effects*: Equivalent to `applicative<C>.zero()`.

[x+14]{.pnum} *Remarks*: The name `zero` denotes a family of operation objects, one per context: the failure state that carries no witness, which only a context whose failure state needs nothing has. It is the identity of the disjunction an Alternative object would supply, and is spelled now so that `raise` and `zero` are decided under one rule. `std::expected` has no `zero`: its failure state needs a witness, which is what `raise` supplies. The exception specification is equivalent to `noexcept(applicative<C>.zero())`.

```cpp
inline constexpr $unspecified$ zip_with;
```

[x+15]{.pnum} *Constraints*: For a call `zip_with(function, first_argument, second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.zip_with(function, first_argument, second_argument)` is a valid expression.

[x+16]{.pnum} *Effects*: Equivalent to `applicative<C>.zip_with(function, first_argument, second_argument)`, with each argument forwarded in its own value category.

[x+17]{.pnum} *Remarks*: The name `zip_with` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The exception specification is equivalent to `noexcept(applicative<C>.zip_with(function, first_argument, second_argument))`.

```cpp
inline constexpr $unspecified$ discard_first;
```

[x+18]{.pnum} *Constraints*: For a call `discard_first(first_argument, second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.discard_first(first_argument, second_argument)` is a valid expression.

[x+19]{.pnum} *Effects*: Equivalent to `applicative<C>.discard_first(first_argument, second_argument)`, with each argument forwarded in its own value category.

[x+20]{.pnum} *Remarks*: The name `discard_first` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The exception specification is equivalent to `noexcept(applicative<C>.discard_first(first_argument, second_argument))`.

```cpp
inline constexpr $unspecified$ discard_second;
```

[x+21]{.pnum} *Constraints*: For a call `discard_second(first_argument, second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.discard_second(first_argument, second_argument)` is a valid expression.

[x+22]{.pnum} *Effects*: Equivalent to `applicative<C>.discard_second(first_argument, second_argument)`, with each argument forwarded in its own value category.

[x+23]{.pnum} *Remarks*: The name `discard_second` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The exception specification is equivalent to `noexcept(applicative<C>.discard_second(first_argument, second_argument))`.

:::

:::
