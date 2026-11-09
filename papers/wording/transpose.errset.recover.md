::: add

::: wording

## recover [transpose.errset.recover]{- .sref} {-}

```cpp
template<class... HANDLED, class VALUE, class... ERRORS, class HANDLER>
  constexpr auto recover(const expected<VALUE, error_set_of<ERRORS...>>& computation, HANDLER&& handler) -> $see below$;
```

[x]{.pnum} *Constraints*: `sizeof...(ERRORS) > 0` is `true`.

[x+1]{.pnum} *Mandates*: `HANDLED` is not empty; every type in `HANDLED` is one of the computation's alternatives; and for each such type `handler` returns either `VALUE` or a `std::expected<VALUE, error_set<...>>`.

[x+2]{.pnum} *Effects*: For each type in `HANDLED` that the computation's error witnesses, invokes `handler` with that witness. A handler either recovers a value or raises an error set of its own. Witnesses for types outside `HANDLED`, and any a handler raises, survive into the result.

[x+3]{.pnum} *Returns*: The computation at the narrowed grade: the alternatives it may raise, less those handled, plus any a handler raises. The result holds a value when every witness present was recovered, and otherwise the surviving witnesses.

[x+4]{.pnum} *Remarks*: The handled set is annotated rather than inferred: it is given as explicit template arguments and checked against the computation's grade. Recovering one alternative from a value that raised two leaves the other, which is still an error. If more than one handled type is witnessed and each independently recovers a value, the first in canonical order is the result, matching the tie-break of `combined_with`.

```cpp
template<class... HANDLED, class VALUE, class HANDLER>
  constexpr auto recover(const expected<VALUE, error_set_of<>>& computation, HANDLER&&)
      -> expected<VALUE, error_set_of<>>;
```

[x+5]{.pnum} *Constraints*: The computation's grade is the empty error set.

[x+6]{.pnum} *Mandates*: `sizeof...(HANDLED) > 0` is `true`.

[x+7]{.pnum} *Effects*: None. `handler` is not invoked, and no requirement is placed on it.

[x+8]{.pnum} *Returns*: `computation`.

[x+9]{.pnum} *Remarks*: At the empty grade no alternative can be witnessed, so there is nothing to recover from and nothing to check `HANDLED` against. This is what keeps a generic recovery step well-formed when an earlier stage has statically eliminated every failure.

```cpp
namespace std {
  template<class T> inline constexpr bool $is-expected-v$ = false; // exposition only
}
```

```cpp
namespace std {
  template<class VALUE, class ERROR>
  inline constexpr bool $is-expected-v$<expected<VALUE, ERROR>> = true; // exposition only
}
```

:::

:::
