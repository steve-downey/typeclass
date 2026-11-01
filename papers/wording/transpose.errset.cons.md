::: add

::: wording

## Constructors [transpose.errset.cons]{- .sref} {-}

```cpp
template<class ERROR>
constexpr error_set_of(ERROR&& error) noexcept(
    is_nothrow_constructible_v<remove_cvref_t<ERROR>, ERROR> &&
    (is_nothrow_move_constructible_v<ERRORS> && ...));
```

[x]{.pnum} *Constraints*: `ERROR`, with cv-qualification and references removed, is one of the alternatives.

[x+1]{.pnum} *Effects*: Constructs a value witnessing exactly the alternative `ERROR` names, holding `error` as that witness. Every other slot is empty.

[x+2]{.pnum} *Remarks*: This is the injection of an error value into the set it belongs to. It is not explicit: a raised error is usable as the set that may raise it. The exception specification is that of constructing the witness from `error` and placing it in its slot.

```cpp
template<class... NARROWER>
constexpr error_set_of(const error_set_of<NARROWER...>& narrower) noexcept(
    (is_nothrow_copy_constructible_v<NARROWER> && ...) &&
    (is_nothrow_move_constructible_v<ERRORS> && ...));
```

[x+3]{.pnum} *Constraints*: `NARROWER` is non-empty, names a different set, and every one of its alternatives is an alternative of `*this`.

[x+4]{.pnum} *Effects*: Constructs a value witnessing exactly what `narrower` witnesses. Slots for alternatives `NARROWER` cannot name are empty.

[x+5]{.pnum} *Remarks*: This is subsumption, and widening along inclusion is the only conversion there is: there is none in the other direction, and none between incomparable sets. There is deliberately no conversion from the empty set, which is uninhabited, so no value ever needs one. The exception specification is that of copying each of `narrower`'s witnesses into its slot.

:::

:::
