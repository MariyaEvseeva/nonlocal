#ifndef NONLOCAL_CONSTANTS_HPP
#define NONLOCAL_CONSTANTS_HPP

#include <cstdint>

namespace nonlocal {

enum axis : uint8_t {
    X,
    Y,
    Z
};

enum class theory_t : bool {
    LOCAL,
    NONLOCAL
};

template<class T>
inline constexpr T MAX_NONLOCAL_WEIGHT = T{0.999};

}

#endif