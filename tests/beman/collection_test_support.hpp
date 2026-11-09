// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TEST_COLLECTION_TEST_SUPPORT_HPP
#define BEMAN_TEST_COLLECTION_TEST_SUPPORT_HPP

#include <beman/transpose/apply.hpp>

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace collection_test {

template <class T, bool LABELLED = false>
struct Writer {
    using value_type = T;
    T           held;
    std::string log;
};

// Separate pure and composition carriers exercise policies such as senders.
template <bool COLLECT, bool WIDEN = false>
struct WriterImpl {
    inline static int collects = 0;

    template <class T>
    auto pure(this auto&&, T&& value) {
        return Writer<std::remove_cvref_t<T>>{std::forward<T>(value), {}};
    }

    template <class F, class... CONTEXTS>
    auto invoke(this auto&&, F&& function, CONTEXTS&&... contexts) {
        std::string log;
        (log.append(contexts.log), ...);
        using Result =
            std::remove_cvref_t<std::invoke_result_t<F&, typename std::remove_cvref_t<CONTEXTS>::value_type...>>;
        return Writer<Result, WIDEN>{std::invoke(function, std::forward<CONTEXTS>(contexts).held...), std::move(log)};
    }

    template <class T, bool LABELLED>
    auto collect(this auto&&, std::vector<Writer<T, LABELLED>> contexts)
        requires COLLECT
    {
        ++collects;
        std::vector<T> values;
        std::string    log;
        for (auto& context : contexts) {
            values.push_back(std::move(context.held));
            log += context.log;
        }
        return Writer<std::vector<T>, WIDEN>{std::move(values), std::move(log)};
    }
};

template <bool COLLECT, bool WIDEN = false>
struct WriterMap : beman::transpose::derive_applicative<WriterImpl<COLLECT, WIDEN>> {
    using WriterImpl<COLLECT, WIDEN>::pure;
    using WriterImpl<COLLECT, WIDEN>::invoke;
    using WriterImpl<COLLECT, WIDEN>::collect;
    using WriterImpl<COLLECT, WIDEN>::collects;
};

template <class T>
struct Deferred {
    using value_type = T;
    std::function<T()> run;
};

// Both paths defer composition, so captured topology and interval metadata
// must survive the traversal call and the destruction of its input.
template <bool COLLECT>
struct DeferredImpl {
    template <class T>
    auto pure(this auto&&, T&& value) {
        using U = std::remove_cvref_t<T>;
        return Deferred<U>{[held = std::forward<T>(value)] { return held; }};
    }

    template <class F, class... CONTEXTS>
    auto invoke(this auto&&, F&& function, CONTEXTS&&... contexts) {
        using Result =
            std::remove_cvref_t<std::invoke_result_t<F&, typename std::remove_cvref_t<CONTEXTS>::value_type...>>;
        return Deferred<Result>{[function = std::forward<F>(function),
                                 operands = std::tuple{std::forward<CONTEXTS>(contexts)...}]() mutable {
            return std::apply(
                [&](auto&... operands) {
                    // Braced tuple initialization guarantees operand execution order.
                    auto values = std::tuple{operands.run()...};
                    return std::apply(function, std::move(values));
                },
                operands);
        }};
    }

    template <class T>
    auto collect(this auto&&, std::vector<Deferred<T>> contexts)
        requires COLLECT
    {
        return Deferred<std::vector<T>>{[contexts = std::move(contexts)] {
            std::vector<T> values;
            for (auto& context : contexts)
                values.push_back(context.run());
            return values;
        }};
    }
};

template <bool COLLECT>
struct DeferredMap : beman::transpose::derive_applicative<DeferredImpl<COLLECT>> {
    using DeferredImpl<COLLECT>::pure;
    using DeferredImpl<COLLECT>::invoke;
    using DeferredImpl<COLLECT>::collect;
};

} // namespace collection_test

#endif
