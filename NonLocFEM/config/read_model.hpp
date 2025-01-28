#pragma once

#include "config_utils.hpp"
#include "../equation_parameters.hpp"
#include "influence_functions_1d.hpp"
#include "influence_functions_2d.hpp"

namespace nonlocal::config {

std::string get_model_field(const nlohmann::json& config, const std::string& path_with_access, const std::string& prefix);

template<std::floating_point T, size_t Dimension>
auto read_nonlocal_radius(const nlohmann::json& config, const std::string& path) {
    static_assert(Dimension > 0u, "The Dimension must be greater than 0.");
    std::array<T, Dimension> result;
    if (config.is_number())
        result.fill(config.get<T>());
    else if (config.is_array() && config.size() == Dimension)
        for(const size_t i : std::ranges::iota_view{0u, Dimension})
            result[i] = config[i].get<T>();
    else
        throw std::domain_error{"Field \"" + path + "\" must be an array with length " + std::to_string(Dimension)};

    if constexpr (Dimension == 1u)
        return result.front();
    else
        return result;
}

template<std::floating_point T>
model_parameters<1u, T> read_model_1d(const nlohmann::json& config, const std::string& path) {
    const std::string path_with_access = append_access_sign(path);
    check_required_fields(config, { "local_weight", "nonlocal_radius" }, path_with_access);
    return {
        .influence = influence::polynomial_1d<T, 1u, 1u>{read_nonlocal_radius<T, 1u>(config["nonlocal_radius"], path_with_access + "nonlocal_radius")},
        .local_weight = config["local_weight"].get<T>()
    };
}

template<std::floating_point T>
model_parameters<2u, T> read_model_2d(const nlohmann::json& config, const std::string& path) {
    const std::string path_with_access = append_access_sign(path);
    check_required_fields(config, { "local_weight", "nonlocal_radius" }, path_with_access);
    return {
        .influence = influence::polynomial_2d<T, 2u, 1u>{read_nonlocal_radius<T, 2u>(config["nonlocal_radius"], path_with_access + "nonlocal_radius")},
        .local_weight = config["local_weight"].get<T>()
    };
}

template<std::floating_point T>
std::unordered_map<std::string, T> read_search_radii(const nlohmann::json& config, const std::string& path, const std::string& prefix = "") {
    std::unordered_map<std::string, T> radii;
    for(const auto& [name, material] : config.items()) {
        const std::string path_with_material = append_access_sign(path) + name;
        if (const std::string model_field = get_model_field(material, path_with_material, prefix); !model_field.empty()) {
            const std::string path_with_access = append_access_sign(append_access_sign(path_with_material) + model_field);
            const nlohmann::json& config_model = material[model_field];
            static constexpr auto max = [](const std::array<T, 2>& radii) noexcept { return std::max(radii[0], radii[1]); };
            if (config_model.contains("search_radius"))
                radii[name] = max(read_nonlocal_radius<T, 2u>(config_model["search_radius"], path_with_access + "search_radius"));
            else if (config_model.contains("nonlocal_radius"))
                radii[name] = max(read_nonlocal_radius<T, 2u>(config_model["nonlocal_radius"], path_with_access + "nonlocal_radius"));
        }
    }
    return radii;
}

}