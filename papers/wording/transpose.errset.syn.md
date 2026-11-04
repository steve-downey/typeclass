::: add

```cpp
namespace std {
  template<class... ERRORS>
  inline constexpr bool $error-set-is-canonical-v$ =
      is_same_v<$canonical-t$<ERRORS...>, $type-list$<ERRORS...>>; // exposition only
}
```

```cpp
namespace std {
  template<class... ERRORS>
  inline constexpr bool $error-set-names-distinct-v$ =
      $names-are-distinct-v$<ERRORS...>; // exposition only
}
```

```cpp
namespace std {
  template<class T, class... ERRORS>
  inline constexpr bool $error-set-has-v$ =
      (is_same_v<T, ERRORS> || ...); // exposition only
}
```

```cpp
namespace std {
  template<class... ERRORS>
  class error_set_of {
  public:
    // @[transpose.errset.obs]{- .sref}@, observers
    template<class ERROR> static constexpr auto contains() noexcept -> bool;

    // @[transpose.errset.cons]{- .sref}@, constructors
    template<class ERROR>
      requires($error-set-has-v$<remove_cvref_t<ERROR>, ERRORS...>)
    constexpr error_set_of(ERROR&& error) noexcept(
        is_nothrow_constructible_v<remove_cvref_t<ERROR>, ERROR> &&
        (is_nothrow_move_constructible_v<ERRORS> && ...));
    template<class... NARROWER>
      requires(sizeof...(NARROWER) > 0) &&
              (!is_same_v<error_set_of<NARROWER...>, error_set_of<ERRORS...>>) &&
              ($error-set-has-v$<NARROWER, ERRORS...> && ...)
    constexpr error_set_of(const error_set_of<NARROWER...>& narrower) noexcept(
        (is_nothrow_copy_constructible_v<NARROWER> && ...) &&
        (is_nothrow_move_constructible_v<ERRORS> && ...));
    // @[transpose.errset.obs]{- .sref}@, observers
    template<class ERROR>
      requires($error-set-has-v$<ERROR, ERRORS...>)
    constexpr auto holds() const noexcept -> bool;

    template<class ERROR>
      requires($error-set-has-v$<ERROR, ERRORS...>)
    constexpr auto witness() const noexcept -> const optional<ERROR>&;

    constexpr auto witness_count() const noexcept -> size_t;

    template<class HANDLER>
    constexpr auto visit(HANDLER&& handler) const
        noexcept((is_nothrow_copy_constructible_v<ERRORS> && ...) &&
                 (is_nothrow_invocable_v<HANDLER, ERRORS> && ...)) -> decltype(auto);

    // @[transpose.errset.ops]{- .sref}@, operations
    friend auto operator==(const error_set_of&, const error_set_of&) -> bool = default;

    [[nodiscard]] constexpr auto combined_with(const error_set_of& other) const
        noexcept((is_nothrow_copy_constructible_v<ERRORS> && ...)) -> error_set_of;

  private:
    tuple<optional<ERRORS>...> $d-witnesses$; // exposition only
  };
}
```

::: wording

[x]{.pnum} *Mandates*: The pack is canonical: sorted and deduplicated. Spell `error_set<ERRORS...>`, which canonicalizes, rather than naming this template directly. No two alternatives render to the same name.

[x+1]{.pnum} *Remarks*: A value of this type witnesses a non-empty subset of the alternatives, with at most one witness per raised type. The set says what a computation may raise; a value records what it did raise.

:::

:::
