#pragma once

#include "binary_expression.hpp"
#include "constant.hpp"

namespace metamath::symbolic {

template<class E1, class E2>
class plus : public binary_expression<E1, E2, plus> {
    using _base = binary_expression<E1, E2, plus>;

public:
    constexpr explicit plus(const expression<E1>& e1, const expression<E2>& e2) noexcept
        : _base{e1(), e2()} {}

    template<class... Args>
    constexpr auto operator()(const Args&... args) const {
        const auto [e1, e2] = _base::expr();
        return e1(args...) + e2(args...);
    }

    template<auto X>
    constexpr auto derivative() const noexcept {
        const auto [e1, e2] = _base::expr();
        return e1.template derivative<X>() + e2.template derivative<X>();
    }
};

template<class E1, class E2>
constexpr plus<E1, E2> operator+(const expression<E1>& e1, const expression<E2>& e2) noexcept {
    return plus<E1, E2>{e1(), e2()};
}

template<class T, class E, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
constexpr plus<constant<T>, E> operator+(const T& c, const expression<E>& e) noexcept {
    return plus<constant<T>, E>{constant<T>{c}, e()};
}

template<class E, class T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
constexpr plus<E, constant<T>> operator+(const expression<E>& e, const T& c) noexcept {
    return plus<E, constant<T>>{e(), constant<T>{c}};
}

template<auto N1, auto N2>
constexpr integral_constant<N1 + N2> simplify(const plus<integral_constant<N1>, integral_constant<N2>>) noexcept {
    return {};
}

template<auto N, class T>
constexpr constant<decltype(N + T{})> simplify(const plus<integral_constant<N>, constant<T>>& e) noexcept {
    const auto [_, c] = e.expr();
    return {N + c()};
}

template<class T, auto N>
constexpr constant<decltype(T{} + N)> simplify(const plus<constant<T>, integral_constant<N>>& e) noexcept {
    const auto [c, _] = e.expr();
    return {c() + N};
}

template<class T1, class T2>
constexpr constant<decltype(T1{} + T2{})> simplify(const plus<constant<T1>, constant<T2>>& e) noexcept {
    const auto [c1, c2] = e.expr();
    return {c1() + c2()};
}

template<auto N, class E, std::enable_if_t<N == 0, bool> = true>
constexpr auto simplify(const plus<integral_constant<N>, E>& p) noexcept {
    const auto [_, e] = p.expr();
    return simplify(e);
}

template<class E, auto N, std::enable_if_t<N == 0, bool> = true>
constexpr auto simplify(const plus<E, integral_constant<N>>& p) noexcept {
    const auto [e, _] = p.expr();
    return simplify(e);
}

}