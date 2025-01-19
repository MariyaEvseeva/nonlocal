#pragma once

#include "config_utils.hpp"

#include "nonlocal_constants.hpp"
#include "logger.hpp"

#include <limits>
#include <type_traits>
#include <exception>
#include <ranges>
#include <bitset>

namespace nonlocal::config {

template<std::floating_point T, size_t Dimension>
struct mechanical_material_data;

template<std::floating_point T>
struct mechanical_material_data<T, 1> final {
    static constexpr std::string_view Prefix = "mechanical";

    T youngs_modulus = T{1}; // required

    explicit constexpr mechanical_material_data() noexcept = default;
    explicit mechanical_material_data(const nlohmann::json& config, const std::string& path = {}) {
        const std::string right_part = append_access_sign(path);
        check_required_fields(config, { "youngs_modulus" }, right_part);
        youngs_modulus = config["youngs_modulus"].get<T>();
    }

    operator nlohmann::json() const {
        return {
            {"youngs_modulus", youngs_modulus}
        };
    }
};

template<std::floating_point T>
class mechanical_material_data<T, 2> final {

    std::array<T, 2> read_parameters(const nlohmann::json& elastic_parameter, const std::bitset<2>& null) {
        std::array<T, 2> parameter {0, 0};
        if (elastic_parameter.is_number()) {
            parameter.fill(elastic_parameter.get<T>());
        }
        else if (elastic_parameter.is_array() && elastic_parameter.size() == 2) {
            parameter[0] = null[0] ? elastic_parameter[1].get<T>() : elastic_parameter[0].get<T>();
            parameter[1] = null[1] ? elastic_parameter[0].get<T>() : elastic_parameter[1].get<T>();
        } else
            throw std::domain_error{"The orthotropic material parameter must be specified as an array of dimension 2"};
        return parameter;
    };

    std::bitset<2> check_is_null(const nlohmann::json& elastic_parameter) {
        std::bitset<2> is_null{"00"};
        if (elastic_parameter.is_number()) {
            is_null.flip(1);
        } else if (elastic_parameter.is_array() && elastic_parameter.size() == 2) {
            material = material_t::ORTHOTROPIC; 
            is_null[0] = elastic_parameter[0].is_null();
            is_null[1] = elastic_parameter[1].is_null();
        } else
            throw std::domain_error{"The orthotropic material parameter must be specified as an array of dimension 2"};
        return is_null;
    }

    void read_elasticity_parameters(const nlohmann::json& config) {
        const std::bitset<2> null_E  = check_is_null(config["youngs_modulus"]);
        const std::bitset<2> null_nu = check_is_null(config["poissons_ratio"]);
        if (material == material_t::ORTHOTROPIC && null_E.count() + null_nu.count() != 1)
            throw std::domain_error{"Input format error. Choose three of the input parameters : Ex, Ey, nuxy, nuyx. The fourth will be calculated automatically"};
        youngs_modulus = read_parameters(config["youngs_modulus"], null_E);
        poissons_ratio = read_parameters(config["poissons_ratio"], null_nu);
        calculate_elasticity_parameters(null_E, null_nu);
        if (!check_elasticity_parameters())
            throw std::domain_error{"Input format error. All elasticity parameters must be nonzero. Moreover : poissons_ratio lies in (-1, 0) U (0, 0.5)"};
    }

    void calculate_elasticity_parameters(const std::bitset<2>& null_E, const std::bitset<2>& null_nu) {
        // youngs_modulus[1] * poissons_ratio[0] = Ey * nuxy = Ex * nuyx = youngs_modulus[0] * poissons_ratio[1]
        if (material == material_t::ISOTROPIC) 
            return;
        if (!null_E[0] && null_E[1]) {
            youngs_modulus[1] = youngs_modulus[0] * poissons_ratio[1] / poissons_ratio[0];
            return;
        }
        if (null_E[0] && !null_E[1]) {
            youngs_modulus[0] = youngs_modulus[1] * poissons_ratio[0] / poissons_ratio[1];
            return;
        }
        if (!null_nu[0] && null_nu[1]) {
            poissons_ratio[1] = youngs_modulus[1] * poissons_ratio[0] / youngs_modulus[0];
            return;
        }
        if (null_nu[0] && !null_nu[1]) {
            poissons_ratio[0] = youngs_modulus[0] * poissons_ratio[1] / youngs_modulus[1];
            return;
        }
    }  

    bool check_elasticity_parameters() const noexcept {
        for(const size_t i : std::ranges::iota_view{0u, 2u}) {
            if (std::isinf(poissons_ratio[i]) || std::isinf(youngs_modulus[i]))
                return false;
            if (poissons_ratio[i] < -1 || poissons_ratio[i] > 0.5 || std::abs(poissons_ratio[i]) < std::numeric_limits<T>::epsilon())
                return false;
            if (youngs_modulus[i] <= 0)
                return false;
        }
        if (shear_modulus <= 0)
            return false;
        return true;
    }

public:
    static constexpr std::string_view Prefix = "mechanical";
    material_t material = material_t::ISOTROPIC;        // not json field
    std::array<T, 2> youngs_modulus = {T{1},   T{1}};   // required
    std::array<T, 2> poissons_ratio = {T{0.3}, T{0.3}}; // required
    T shear_modulus = T{1};     // required for orthotropic
    T thermal_expansion = T{0}; // optional

    explicit constexpr mechanical_material_data() noexcept = default;
    explicit mechanical_material_data(const nlohmann::json& config, const std::string& path = {}) {
        const std::string right_part = append_access_sign(path);
        check_required_fields(config, { "youngs_modulus", "poissons_ratio" }, right_part);
        read_elasticity_parameters(config);
        if (material == material_t::ORTHOTROPIC) {
            check_required_fields(config, {"shear_modulus"}, right_part);
            shear_modulus = config.value("shear_modulus", T{1});
        } else if (material == material_t::ISOTROPIC && config.contains("shear_modulus")) {
            logger::warning() << "Material is isotropic. Shear modulus parameter will be ignored." << std::endl;
        }
        check_optional_fields(config, {"thermal_expansion"}, right_part); 
        thermal_expansion = config.value("thermal_expansion", T{0});
    }

    operator nlohmann::json() const {
        return {
            {"youngs_modulus", youngs_modulus},
            {"poissons_ratio", poissons_ratio},
            {"shear_modulus", shear_modulus},
            {"thermal_expansion", thermal_expansion}
        };
    }
};

}