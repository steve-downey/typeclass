::: add

::: wording

## Traversal operations [transpose.traversable.ops]{- .sref} {-}

```cpp
template<class T, class F>
constexpr auto for_each(this auto&& self, T&& value, F&& function) noexcept(noexcept(
    self.traverse(applicative<remove_cvref_t<
                      invoke_result_t<F&, $traversal-argument-t$<decltype(self), T>>>>,
                  forward<F>(function), forward<T>(value))));
```

[x]{.pnum} *Constraints*: Let `C` be the context `function` returns for an argument of type `$traversal-argument-t$<decltype(self), T>`. `self.traverse` accepts the object `applicative<C>` names, `function`, and `value`.

[x+1]{.pnum} *Effects*: Applies `function` to each element of `value` and transposes the resulting contextual values, preserving the shape of `value`. The element is presented through `$traversal-argument-t$<decltype(self), T>`. The applicative object is the one `applicative` names for the context `function` returns for that argument type.

[x+2]{.pnum} *Returns*: The shape of `value` held in that single context.

[x+3]{.pnum} *Complexity*: Exactly one application of `function` per element of `value`.

[x+4]{.pnum} *Remarks*: Elements are visited in the structure's iteration order. If `self` declares consumption and `value` is a non-`const`, non-`volatile` rvalue, `function` receives rvalue elements; otherwise it receives `const` lvalue elements. The exception specification is that of the `traverse` call the operation makes. The result is the only operation of this object whose result a caller may discard: `for_each` is `traverse` with its arguments the other way round, and is the spelling for a traversal made for its effects.

```cpp
template<class T>
[[nodiscard]] constexpr auto transpose(this auto&& self, T&& value) noexcept(
    noexcept(self.traverse(applicative<typename Impl::element_type>, identity{},
                           forward<T>(value))));
```

[x+5]{.pnum} *Constraints*: `element_type` is a context for which `applicative` names an applicative object, and `self.traverse` accepts that object, an identity function, and `value`.

[x+6]{.pnum} *Effects*: Equivalent to traversing `value` with the identity function: a structure of contextual values becomes a single contextual value of the structure, preserving shape.

[x+7]{.pnum} *Returns*: That single contextual value.

[x+8]{.pnum} *Remarks*: Elements are visited in the structure's iteration order. The constraint is what lets a caller ask whether this operation is available for a structure whose elements are not a context: the return type is deduced, so without it the only answer available is a diagnostic from inside the body. The exception specification is that of the `traverse` call the operation makes.

:::

:::
