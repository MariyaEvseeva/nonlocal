#pragma once

#include "config_utils.hpp"

namespace nonlocal::config {

template<std::floating_point T>
struct thermal_auxiliary_data final {
    T energy = 0;               // Used for Neumann problem
    T right_part = 0;
    T initial_distribution = 0; // Used for nonstationary and nonlinear problems

    explicit constexpr thermal_auxiliary_data() noexcept = default;
    explicit thermal_auxiliary_data(const nlohmann::json& config, const std::string& path = {}) {
        check_optional_fields(config, {"energy", "right_part", "initial_distribution"}, append_access_sign(path));
        energy = config.value("energy", T{0});
        right_part = config.value("right_part", T{0});
        initial_distribution = config.value("initial_distribution", T{0});
    }
    
    operator nlohmann::json() const {
        return {
            {"energy", energy},
            {"right_part", right_part},
            {"initial_distribution", initial_distribution}
        };
    }
};

}