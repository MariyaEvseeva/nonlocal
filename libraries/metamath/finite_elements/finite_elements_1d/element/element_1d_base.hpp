#ifndef FINITE_ELEMENT_1D_BASE_HPP
#define FINITE_ELEMENT_1D_BASE_HPP

#include "finite_element_base.hpp"
#include "side_1d.hpp"

#include <type_traits>

namespace metamath::finite_element {

template<class T>
class element_1d_base : public element_base {
    static_assert(std::is_floating_point_v<T>, "The T must be floating point.");

public:
    ~element_1d_base() override = default;

    virtual size_t max_derivative_order() const = 0;
    virtual T node(const size_t i) const = 0;
    virtual T N(const size_t d, const size_t i, const T xi) const = 0;
    virtual T boundary(const side_1d bound) const = 0;
};

}

#endif