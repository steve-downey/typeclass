::: add

::: wording

## Delegated traversal [transpose.traversable.delegate]{- .sref} {-}

```cpp
template<class TRAVERSABLE_MAP, class T, class F>
  [[nodiscard]] constexpr auto traverse_with(
      this auto&&, const TRAVERSABLE_MAP& traversable_map, F&& function,
      T&& value) noexcept(noexcept(traversable_map.traverse(applicative<$traversal-context-t$<TRAVERSABLE_MAP, T, F>>,
                                                            forward<F>(function), forward<T>(value))));
```

[x]{.pnum} *Constraints*: Let `C` be the context `function` returns for an argument of type `$traversal-argument-t$<TRAVERSABLE_MAP, T>`. `traversable_map.traverse` accepts the object `applicative<C>` names, `function`, and `value`.

[x+1]{.pnum} *Effects*: Traverses `value` using the traversable object `traversable_map` rather than `*this`. The element is presented through `$traversal-argument-t$<TRAVERSABLE_MAP, T>`. The applicative object is the one `applicative` names for the context `function` returns for that argument type.

[x+2]{.pnum} *Returns*: The result of that traversal.

[x+3]{.pnum} *Remarks*: The exception specification is that of the `traverse` call the operation makes.

```cpp
template<class TRAVERSABLE_MAP, class APPLICATIVE_MAP, class T, class F>
  [[nodiscard]] constexpr auto traverse_with(
      this auto&&, const TRAVERSABLE_MAP& traversable_map, const APPLICATIVE_MAP& applicative_map, F&& function,
      T&& value) noexcept(noexcept(traversable_map.traverse(applicative_map, forward<F>(function), forward<T>(value))));
```

[x+4]{.pnum} *Constraints*: `traversable_map.traverse(applicative_map, function, value)`, with `function` and `value` forwarded, is a valid expression.

[x+5]{.pnum} *Effects*: Equivalent to:

```cpp
return traversable_map.traverse(applicative_map, forward<F>(function),
                                forward<T>(value));
```

```cpp
template<class TRAVERSABLE_MAP, class T>
  [[nodiscard]] constexpr auto transpose_with(
      this auto&& self, const TRAVERSABLE_MAP& traversable_map,
      T&& value) noexcept(noexcept(self.traverse_with(traversable_map, identity{}, forward<T>(value))));
```

[x+6]{.pnum} *Constraints*: The element type of `TRAVERSABLE_MAP` is a context for which `applicative` names an applicative object, and `self.traverse_with(traversable_map, identity{}, value)`, with `value` forwarded, is a valid expression.

[x+7]{.pnum} *Effects*: Equivalent to:

```cpp
return self.traverse_with(traversable_map, identity{}, forward<T>(value));
```

:::

:::
