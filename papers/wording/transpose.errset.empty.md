::: add

::: wording

## The empty error set [transpose.errset.empty]{- .sref} {-}

```cpp
error_set_of() = delete;
```

[x]{.pnum} *Remarks*: The empty error set is uninhabited: a computation graded by it raises nothing, so there is no value to construct.

```cpp
template<class ERROR>
  static constexpr auto contains() noexcept -> bool;
```

[x+1]{.pnum} *Returns*: `false`.

```cpp
friend auto operator==(const error_set_of&, const error_set_of&) -> bool = default;
```

[x+2]{.pnum} *Returns*: `true`.

[x+3]{.pnum} *Remarks*: Vacuous, and present for the reason the valued set's equality is: `expected<T, error_set<>>`, the explicit uniform spelling of the empty grade, compares equal whenever its value does, as the bare value it stands for would.

```cpp
template<class... ERRORS>
  using error_set = $see below$;
```

[x+4]{.pnum} *Remarks*: `error_set<ERRORS...>` is the canonicalizing spelling of an error set: the pack is sorted and deduplicated, so `error_set<A, B>` and `error_set<B, A, A>` denote the same type. A grade is therefore normalized by type identity, not by source spelling or alternative position, and every inclusion has one canonical target.

:::

:::
