::: add

```cpp
namespace std {
  template<class VALUE_TYPE>
  struct $vector-traversable-impl$ {
    using element_type = VALUE_TYPE;

    static constexpr bool consumes_rvalue_structure = true;

    // @[transpose.range.traverse]{- .sref}@, traversable instance for vector
    template<class APPLICATIVE, class FUNCTION, class VALUES>
    constexpr auto traverse(this auto&&, const APPLICATIVE& applicative,
                            FUNCTION&& function, VALUES&& values)
      requires same_as<remove_cvref_t<VALUES>, vector<VALUE_TYPE>> &&
               invocable<FUNCTION&,
                         $traversal-argument-t$<vector_traversable_impl, VALUES>> &&
               applicative_object_for<
                   APPLICATIVE,
                   remove_cvref_t<invoke_result_t<
                       FUNCTION&, $traversal-argument-t$<vector_traversable_impl, VALUES>>>>
    ;
  }; // exposition only
}
```

::: wording

[x]{.pnum} *Remarks*: A `vector` handed over as a non-`const`, non-`volatile` rvalue owns its elements and this instance presents each element to the traversal function as an rvalue. The declaration is what permits traversal to infer the consuming function's context before selecting this object.

:::

```cpp
namespace std {
  template<class VALUE_TYPE>
  struct $vector-traversable-map$
      : derive_traversable<$vector-traversable-impl$<VALUE_TYPE>> {
    using $vector-traversable-impl$<VALUE_TYPE>::traverse;
  }; // exposition only
}
```

:::
