::: add

::: wording

```cpp
inline constexpr $unspecified$ applicative_eval;
```

[x]{.pnum} *Remarks*: `applicative_eval` applies a callable to one argument. It is the evaluator that recovers one-step application from the n-ary core: `ap(f, x)` is `invoke(applicative_eval, f, x)`. Naming it is what makes `ap`'s second alternative expressible, and that alternative is satisfied only where the context can hold a callable.

:::

::: wording

```cpp
inline constexpr $unspecified$ discard_first_eval;
```

[x+1]{.pnum} *Remarks*: `discard_first_eval` discards its first argument and returns its second. It is the evaluator `discard_first`'s second alternative probes, and that alternative is satisfied exactly when `discard_first`'s own `invoke`-based derivation would be.

:::

::: wording

```cpp
inline constexpr $unspecified$ discard_second_eval;
```

[x+2]{.pnum} *Remarks*: `discard_second_eval` discards its second argument and returns its first. It is the evaluator `discard_second`'s second alternative probes, and that alternative is satisfied exactly when `discard_second`'s own `invoke`-based derivation would be.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class FUNCTION, class... ARGUMENTS>
  inline constexpr bool $applicative-invoke-nothrow$ =
      requires($impl-ref-t$<IMPL, SELF> impl) {
        impl.invoke(declval<FUNCTION>(), declval<ARGUMENTS>()...);
      } ? requires($impl-ref-t$<IMPL, SELF> impl) {
        { impl.invoke(declval<FUNCTION>(), declval<ARGUMENTS>()...) } noexcept;
      } : $nothrow-invocable-through-ap$<SELF, FUNCTION, ARGUMENTS...>; // exposition only
}
```

::: wording

[x+3]{.pnum} *Remarks*: `$applicative-invoke-nothrow$<IMPL, SELF, FUNCTION, ARGUMENTS...>` is `true` when the route `derive_applicative<IMPL>::invoke` takes for these arguments is non-throwing: `IMPL`'s own `invoke` where it accepts them, and otherwise the embedding of the function with `pure` followed by one `ap` per argument.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT>
  inline constexpr bool $applicative-ap-nothrow$ = requires($impl-ref-t$<IMPL, SELF> impl) {
    impl.ap(declval<FUNCTION_IN_CONTEXT>(), declval<ARGUMENT_IN_CONTEXT>());
  } ? requires($impl-ref-t$<IMPL, SELF> impl) {
    {
      impl.ap(declval<FUNCTION_IN_CONTEXT>(), declval<ARGUMENT_IN_CONTEXT>())
    } noexcept;
  } : requires($impl-ref-t$<IMPL, SELF> impl) {
    {
      impl.invoke(applicative_eval, declval<FUNCTION_IN_CONTEXT>(),
                  declval<ARGUMENT_IN_CONTEXT>())
    } noexcept;
  }; // exposition only
}
```

::: wording

[x+4]{.pnum} *Remarks*: `$applicative-ap-nothrow$<IMPL, SELF, FUNCTION_IN_CONTEXT, ARGUMENT_IN_CONTEXT>` is `true` when the route `derive_applicative<IMPL>::ap` takes is non-throwing: `IMPL`'s own `ap` where it accepts the arguments, and otherwise `IMPL`'s `invoke` applied to `applicative_eval` and the arguments.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class FUNCTION, class ARGUMENT>
  inline constexpr bool $applicative-map-nothrow$ =
      requires($impl-ref-t$<IMPL, SELF> impl) {
        impl.map(declval<FUNCTION>(), declval<ARGUMENT>());
      } ? requires($impl-ref-t$<IMPL, SELF> impl) {
        { impl.map(declval<FUNCTION>(), declval<ARGUMENT>()) } noexcept;
      } : requires(SELF& self) {
        { self.invoke(declval<FUNCTION>(), declval<ARGUMENT>()) } noexcept;
      }; // exposition only
}
```

::: wording

[x+5]{.pnum} *Remarks*: `$applicative-map-nothrow$<IMPL, SELF, FUNCTION, ARGUMENT>` is `true` when the route `derive_applicative<IMPL>::map` takes is non-throwing: `IMPL`'s own `map` where it accepts the arguments, and otherwise the object's own `invoke` applied to them.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class VALUE>
  inline constexpr bool $applicative-lift-nothrow$ =
      requires($impl-ref-t$<IMPL, SELF> impl) {
        impl.lift(declval<VALUE>());
      } ? requires($impl-ref-t$<IMPL, SELF> impl) {
        { impl.lift(declval<VALUE>()) } noexcept;
      } : requires(SELF& self) {
        { self.pure(declval<VALUE>()) } noexcept;
      }; // exposition only
}
```

::: wording

[x+6]{.pnum} *Remarks*: `$applicative-lift-nothrow$<IMPL, SELF, VALUE>` is `true` when the route `derive_applicative<IMPL>::lift` takes is non-throwing: `IMPL`'s own `lift` where it accepts `VALUE`, and otherwise the object's own `pure`.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class FUNCTION, class FIRST_ARGUMENT,
           class SECOND_ARGUMENT>
  inline constexpr bool $applicative-zip-with-nothrow$ =
      requires($impl-ref-t$<IMPL, SELF> impl) {
        impl.zip_with(declval<FUNCTION>(), declval<FIRST_ARGUMENT>(),
                      declval<SECOND_ARGUMENT>());
      } ? requires($impl-ref-t$<IMPL, SELF> impl) {
        {
          impl.zip_with(declval<FUNCTION>(), declval<FIRST_ARGUMENT>(),
                        declval<SECOND_ARGUMENT>())
        } noexcept;
      } : requires(SELF& self) {
        {
          self.invoke(declval<FUNCTION>(), declval<FIRST_ARGUMENT>(),
                      declval<SECOND_ARGUMENT>())
        } noexcept;
      }; // exposition only
}
```

::: wording

[x+7]{.pnum} *Remarks*: `$applicative-zip-with-nothrow$<IMPL, SELF, FUNCTION, FIRST_ARGUMENT, SECOND_ARGUMENT>` is `true` when the route `derive_applicative<IMPL>::zip_with` takes is non-throwing: `IMPL`'s own `zip_with` where it accepts the arguments, and otherwise the object's own `invoke` applied to them.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
  inline constexpr bool $applicative-discard-first-nothrow$ =
      requires($impl-ref-t$<IMPL, SELF> impl) {
        impl.discard_first(declval<FIRST_ARGUMENT>(), declval<SECOND_ARGUMENT>());
      } ? requires($impl-ref-t$<IMPL, SELF> impl) {
        {
          impl.discard_first(declval<FIRST_ARGUMENT>(), declval<SECOND_ARGUMENT>())
        } noexcept;
      } : requires(SELF& self) {
        {
          self.invoke(discard_first_eval, declval<FIRST_ARGUMENT>(),
                      declval<SECOND_ARGUMENT>())
        } noexcept;
      }; // exposition only
}
```

::: wording

[x+8]{.pnum} *Remarks*: `$applicative-discard-first-nothrow$<IMPL, SELF, FIRST_ARGUMENT, SECOND_ARGUMENT>` is `true` when the route `derive_applicative<IMPL>::discard_first` takes is non-throwing: `IMPL`'s own `discard_first` where it accepts the arguments, and otherwise the object's own `invoke` applied to `discard_first_eval` and them.

:::

```cpp
namespace std {
  template<class IMPL, class SELF, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
  inline constexpr bool $applicative-discard-second-nothrow$ =
      requires($impl-ref-t$<IMPL, SELF> impl) {
        impl.discard_second(declval<FIRST_ARGUMENT>(), declval<SECOND_ARGUMENT>());
      } ? requires($impl-ref-t$<IMPL, SELF> impl) {
        {
          impl.discard_second(declval<FIRST_ARGUMENT>(), declval<SECOND_ARGUMENT>())
        } noexcept;
      } : requires(SELF& self) {
        {
          self.invoke(discard_second_eval, declval<FIRST_ARGUMENT>(),
                      declval<SECOND_ARGUMENT>())
        } noexcept;
      }; // exposition only
}
```

::: wording

[x+9]{.pnum} *Remarks*: `$applicative-discard-second-nothrow$<IMPL, SELF, FIRST_ARGUMENT, SECOND_ARGUMENT>` is `true` when the route `derive_applicative<IMPL>::discard_second` takes is non-throwing: `IMPL`'s own `discard_second` where it accepts the arguments, and otherwise the object's own `invoke` applied to `discard_second_eval` and them.

:::

::: wording

```cpp
template <class IMPL, class CONTEXT>
concept applicative_impl =
    requires(const IMPL &impl) {
        impl.pure(declval<$applicative-value-t$<CONTEXT>>());
    } &&
    (requires(const IMPL &impl, const CONTEXT &context) {
        impl.invoke($probe-witness$<$applicative-value-t$<CONTEXT>>{}, context);
    } || requires(const IMPL &impl, const CONTEXT &context) {
        impl.ap(impl.pure($probe-witness$<$applicative-value-t$<CONTEXT>>{}),
                context);
    });
```

[x+10]{.pnum} *Remarks*: This concept is satisfied when `IMPL` supplies the minimal complete basis the `Applicative` CRTP base needs: `pure`, together with either `invoke` or `ap`. This is the `MINIMAL` pragma to `applicative_object`'s class declaration -- an `IMPL` may satisfy this concept and still fail `applicative_object`, which is exactly the bargain the CRTP base exists to keep. `map`, `lift`, `zip_with`, `discard_first`, `discard_second`, `invoke_with` and `subsume` are all derived and belong to `applicative_object` alone. `pure` is checked for existence only, matching `applicative_object`'s own treatment, and is probed with an rvalue element for the reason `applicative_object` records.

:::

```cpp
namespace std {
  template<class Impl>
  struct derive_applicative : protected Impl {
    using Impl::pure;

    // @[transpose.applicative.basis]{- .sref}@, basis operations
    template<class FUNCTION, class FIRST_ARGUMENT, class... REST_ARGUMENTS>
    [[nodiscard]] constexpr auto invoke(
        this auto&& self, FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
        REST_ARGUMENTS&&... rest_arguments) noexcept($applicative-invoke-nothrow$<Impl,
                                                                                decltype(self),
                                                                                FUNCTION,
                                                                                FIRST_ARGUMENT,
                                                                                REST_ARGUMENTS...>)
      requires requires(const Impl& impl) {
        impl.invoke(forward<FUNCTION>(function),
                    forward<FIRST_ARGUMENT>(first_argument),
                    forward<REST_ARGUMENTS>(rest_arguments)...);
      } || $invocable-through-ap$<decltype(self), FUNCTION, FIRST_ARGUMENT,
                                REST_ARGUMENTS...>;

    template<class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT>
    [[nodiscard]] constexpr auto ap(
        this auto&& self, FUNCTION_IN_CONTEXT&& function,
        ARGUMENT_IN_CONTEXT&&
            argument) noexcept($applicative-ap-nothrow$<Impl, decltype(self),
                                                      FUNCTION_IN_CONTEXT,
                                                      ARGUMENT_IN_CONTEXT>)
      requires requires(const Impl& impl) {
        impl.ap(forward<FUNCTION_IN_CONTEXT>(function),
                forward<ARGUMENT_IN_CONTEXT>(argument));
      } || requires(const Impl& impl) {
        impl.invoke(applicative_eval, forward<FUNCTION_IN_CONTEXT>(function),
                    forward<ARGUMENT_IN_CONTEXT>(argument));
      };

    // @[transpose.applicative.derived]{- .sref}@, derived operations
    template<class FUNCTION, class ARGUMENT>
    [[nodiscard]] constexpr auto map(
        this auto&& self, FUNCTION&& function,
        ARGUMENT&& argument) noexcept($applicative-map-nothrow$<Impl, decltype(self),
                                                              FUNCTION, ARGUMENT>)
      requires requires(const Impl& impl) {
        impl.map(forward<FUNCTION>(function), forward<ARGUMENT>(argument));
      } || requires {
        self.invoke(forward<FUNCTION>(function), forward<ARGUMENT>(argument));
      };

    template<class VALUE>
    [[nodiscard]] constexpr auto lift(this auto&& self, VALUE&& value) noexcept(
        $applicative-lift-nothrow$<Impl, decltype(self), VALUE>)
      requires requires(const Impl& impl) { impl.lift(forward<VALUE>(value)); } ||
               requires { self.pure(forward<VALUE>(value)); };

    template<class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    [[nodiscard]] constexpr auto zip_with(
        this auto&& self, FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
        SECOND_ARGUMENT&&
            second_argument) noexcept($applicative-zip-with-nothrow$<Impl, decltype(self),
                                                                   FUNCTION,
                                                                   FIRST_ARGUMENT,
                                                                   SECOND_ARGUMENT>)
      requires requires(const Impl& impl) {
        impl.zip_with(forward<FUNCTION>(function),
                      forward<FIRST_ARGUMENT>(first_argument),
                      forward<SECOND_ARGUMENT>(second_argument));
      } || requires {
        self.invoke(forward<FUNCTION>(function),
                    forward<FIRST_ARGUMENT>(first_argument),
                    forward<SECOND_ARGUMENT>(second_argument));
      };

    template<class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    [[nodiscard]] constexpr auto discard_first(
        this auto&& self, FIRST_ARGUMENT&& first_argument,
        SECOND_ARGUMENT&&
            second_argument) noexcept($applicative-discard-first-nothrow$<Impl,
                                                                        decltype(self),
                                                                        FIRST_ARGUMENT,
                                                                        SECOND_ARGUMENT>)
      requires requires(const Impl& impl) {
        impl.discard_first(forward<FIRST_ARGUMENT>(first_argument),
                           forward<SECOND_ARGUMENT>(second_argument));
      } || requires {
        self.invoke(discard_first_eval, forward<FIRST_ARGUMENT>(first_argument),
                    forward<SECOND_ARGUMENT>(second_argument));
      };

    template<class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    [[nodiscard]] constexpr auto discard_second(
        this auto&& self, FIRST_ARGUMENT&& first_argument,
        SECOND_ARGUMENT&&
            second_argument) noexcept($applicative-discard-second-nothrow$<Impl,
                                                                         decltype(self),
                                                                         FIRST_ARGUMENT,
                                                                         SECOND_ARGUMENT>)
      requires requires(const Impl& impl) {
        impl.discard_second(forward<FIRST_ARGUMENT>(first_argument),
                            forward<SECOND_ARGUMENT>(second_argument));
      } || requires {
        self.invoke(discard_second_eval, forward<FIRST_ARGUMENT>(first_argument),
                    forward<SECOND_ARGUMENT>(second_argument));
      };

    // @[transpose.applicative.grade]{- .sref}@, grade re-indexing
    template<class TARGET_GRADE, class CARRIER>
    [[nodiscard]] constexpr auto subsume(this auto&&, CARRIER&& value) noexcept(
        noexcept(grade_subsume<TARGET_GRADE>(forward<CARRIER>(value))))
      requires requires { grade_subsume<TARGET_GRADE>(forward<CARRIER>(value)); };

    // @[transpose.applicative.delegate]{- .sref}@, delegated application
    template<class APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
             class... REST_ARGUMENTS>
    [[nodiscard]] constexpr auto invoke_with(
        this auto&&, const APPLICATIVE_MAP& applicative_map, FUNCTION&& function,
        FIRST_ARGUMENT&& first_argument,
        REST_ARGUMENTS&&... rest_arguments) noexcept(noexcept(applicative_map
                                                                  .invoke(
                                                                      forward<FUNCTION>(
                                                                          function),
                                                                      forward<
                                                                          FIRST_ARGUMENT>(
                                                                          first_argument),
                                                                      forward<
                                                                          REST_ARGUMENTS>(
                                                                          rest_arguments)...)))
      requires requires {
        applicative_map.invoke(forward<FUNCTION>(function),
                               forward<FIRST_ARGUMENT>(first_argument),
                               forward<REST_ARGUMENTS>(rest_arguments)...);
      };

    template<const auto& APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
             class... REST_ARGUMENTS>
    [[nodiscard]] constexpr auto invoke_with(
        this auto&&, FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
        REST_ARGUMENTS&&... rest_arguments) noexcept(noexcept(APPLICATIVE_MAP
                                                                  .invoke(
                                                                      forward<FUNCTION>(
                                                                          function),
                                                                      forward<
                                                                          FIRST_ARGUMENT>(
                                                                          first_argument),
                                                                      forward<
                                                                          REST_ARGUMENTS>(
                                                                          rest_arguments)...)))
      requires requires {
        APPLICATIVE_MAP.invoke(forward<FUNCTION>(function),
                               forward<FIRST_ARGUMENT>(first_argument),
                               forward<REST_ARGUMENTS>(rest_arguments)...);
      };
  };
}
```

::: wording

[x+11]{.pnum} A program that instantiates `derive_applicative<Impl>` is ill-formed unless `is_same_v<Impl, false_type>` is `false`.

:::

::: wording

```cpp
template<class T> inline constexpr auto applicative = false_type{};
```

[x+12]{.pnum} *Remarks*: This variable template is the lookup point for the Applicative object of a context type. A program may specialize it for a program-defined context. The primary template names no applicative object. The type of each object named by a specialization this document declares is a class type that is not final and models `semiregular`, so a program may derive from it to combine several objects in one algorithm object.

:::

::: wording

```cpp
template<class T> inline constexpr auto accumulating_applicative = false_type{};
```

[x+13]{.pnum} *Remarks*: This variable template is a second lookup point, over the same carrier and grade algebra as `applicative`, naming the accumulating Applicative object. Where the object named by `applicative` stops at the first failing operand, this object combines the evidence of every failing operand. Neither object is selected automatically for a carrier: the context type alone does not determine which composition discipline a caller wants. This object has no Monad instance, because sequencing requires a value from a computation that accumulation admits may have failed. The type of each object named by a specialization this document declares is a class type that is not final and models `semiregular`, so a program may derive from it to combine several objects in one algorithm object.

:::

::: wording

```cpp
template <class OBJ, class CONTEXT>
concept applicative_object =
    requires(const OBJ &obj, const CONTEXT &context) {
        obj.pure(declval<$applicative-value-t$<CONTEXT>>());
        obj.invoke($probe-witness$<$applicative-value-t$<CONTEXT>>{}, context);
        obj.map($probe-witness$<$applicative-value-t$<CONTEXT>>{}, context);
        obj.lift(declval<$applicative-value-t$<CONTEXT>>());
        obj.zip_with($probe-witness2$<$applicative-value-t$<CONTEXT>>{}, context,
                     context);
        obj.discard_first(declval<CONTEXT>(), declval<CONTEXT>());
        obj.discard_second(declval<CONTEXT>(), declval<CONTEXT>());
        obj.invoke_with(obj, $probe-witness$<$applicative-value-t$<CONTEXT>>{},
                        context);
    } &&
    (!requires(const OBJ &obj) {
        obj.pure($probe-witness$<$applicative-value-t$<CONTEXT>>{});
    } || requires(const OBJ &obj, const CONTEXT &context) {
        obj.ap(obj.pure($probe-witness$<$applicative-value-t$<CONTEXT>>{}),
               context);
    }) &&
    (!graded_context<CONTEXT> ||
     requires(const OBJ &obj, const CONTEXT &context) {
         obj.template subsume<$grade-of-t$<CONTEXT>>(context);
     });
```

[x+14]{.pnum} *Remarks*: This concept is satisfied when `OBJ` provides the full Applicative object surface over `CONTEXT`: `pure`, the `invoke` basis, and the derived `map`, `lift`, `zip_with`, `discard_first`, `discard_second` and `invoke_with`. `ap` and `subsume` are required only where their own condition -- the same one their own declarations carry, not a second spelling of it -- licenses them: `ap` where `CONTEXT` can hold a callable (probed by lifting a witness callable through `OBJ`'s own `pure`, the same mechanism the library's own ap-from-invoke derivation uses), `subsume` where `CONTEXT` participates in grading. Operations templated over an arbitrary callable are probed with one representative witness ($probe-witness$/$probe-witness2$): this checks that the operation exists, not that it holds for every callable. Conformance here is structural, so a hand-implemented object that never derives from `derive_applicative<Impl>` can satisfy this concept.

[x+15]{.pnum} Every probe is written in the value category the operation is used in. `pure` and `lift` take a value *into* the context and are probed with an rvalue element; `discard_first` and `discard_second` return a value *out* of one of their operands and are probed with rvalue operands. Probing either with a `const` lvalue asks the context to copy, which no operation's specification requires, and which for an element type that can be moved but not copied is not merely a stricter test but an ill-formed one: these operations have deduced return types, so a probe instantiates the body, and a body that cannot copy is a diagnostic rather than an unsatisfied constraint. A concept that diagnoses cannot be used to detect anything.

:::

:::
