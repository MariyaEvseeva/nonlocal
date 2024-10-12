#include "tests_config_utils.hpp"
#include "required_fields_json.h"

#include "nonlocal_config.hpp"

#include <boost/ut.hpp>

namespace {

using namespace boost::ut;
using namespace unit_tests;
using namespace nonlocal::config;

template<class T>
auto expect_throw(const nlohmann::json& name) {
    return [&name] {
        expect(throws([&name] { T{name}; }));
    };
}

template<class T>
auto expect_no_throw(const nlohmann::json& name) {
    return [&name] {
        expect(nothrow([&name] { T{name}; }));
    };
}

auto mesh_data_test(const nlohmann::json& config) {
    return [&config] {
        test("mesh_2d_data_missed_all")          = expect_throw<mesh_data<2>>(config["mesh_dim_missed_all"]);
        test("mesh_3d_data_missed_all")          = expect_throw<mesh_data<3>>(config["mesh_dim_missed_all"]);
        test("mesh_1d_data_missed_all")          = expect_no_throw<mesh_data<1>>(config["mesh_dim_all_required_exists"]);
        test("mesh_2d_data_all_required_exists") = expect_no_throw<mesh_data<2>>(config["mesh_dim_all_required_exists"]);
        test("mesh_3d_data_all_required_exists") = expect_no_throw<mesh_data<3>>(config["mesh_dim_all_required_exists"]);
    };
}

template<std::floating_point T>
auto time_data_test(const nlohmann::json& config) {
    return [&config] {
        const std::string suffix = '_' + std::string{reflection::type_name<T>()};
        test("time_missed_all" + suffix)          = expect_throw<time_data<T>>(config["time_missed_all"]);
        test("time_missed_time_step" + suffix)    = expect_throw<time_data<T>>(config["time_missed_time_step"]);
        test("time_missed_steps_count" + suffix)  = expect_throw<time_data<T>>(config["time_missed_steps_count"]);
        test("time_all_required_exists" + suffix) = expect_no_throw<time_data<T>>(config["time_all_required_exists"]);
    };
}

template<std::floating_point T>
auto boundaries_conditions_data_test(const nlohmann::json& config) {
    return [&config] {
        const std::string suffix = '_' + std::string{reflection::type_name<T>()};
        test("boundaries_conditions_1d_missed_all" + suffix)          = expect_throw<boundaries_conditions_data<mock_data, T, 1>>(config["boundaries_conditions_missed_all"]);
        test("boundaries_conditions_2d_missed_all" + suffix)          = expect_no_throw<boundaries_conditions_data<mock_data, T, 2>>(config["boundaries_conditions_missed_all"]);
        test("boundaries_conditions_3d_missed_all" + suffix)          = expect_no_throw<boundaries_conditions_data<mock_data, T, 3>>(config["boundaries_conditions_missed_all"]);
        test("boundaries_conditions_1d_missed_left" + suffix)         = expect_throw<boundaries_conditions_data<mock_data, T, 1>>(config["boundaries_conditions_missed_left"]);
        test("boundaries_conditions_1d_missed_right" + suffix)        = expect_throw<boundaries_conditions_data<mock_data, T, 1>>(config["boundaries_conditions_missed_right"]);
        test("boundaries_conditions_1d_all_required_exists" + suffix) = expect_no_throw<boundaries_conditions_data<mock_data, T, 1>>(config["boundaries_conditions_all_required_exists"]);
    };
}

template<std::floating_point T>
auto model_data_test(const nlohmann::json& config) {
    return [&config] {
        const std::string suffix = '_' + std::string{reflection::type_name<T>()};
        test("model_1d_missed_all" + suffix)             = expect_throw<model_data<T, 1>>(config["model_dim_missed_all"]);
        test("model_2d_missed_all" + suffix)             = expect_throw<model_data<T, 2>>(config["model_dim_missed_all"]);
        test("model_3d_missed_all" + suffix)             = expect_throw<model_data<T, 3>>(config["model_dim_missed_all"]);
        test("model_1d_missed_local_weight" + suffix)    = expect_throw<model_data<T, 1>>(config["model_dim_missed_local_weight"]);
        test("model_1d_missed_nonlocal_radius" + suffix) = expect_throw<model_data<T, 1>>(config["model_dim_missed_nonlocal_radius"]);
        test("model_2d_missed_local_weight" + suffix)    = expect_throw<model_data<T, 2>>(config["model_dim_missed_local_weight"]);
        test("model_2d_missed_nonlocal_radius" + suffix) = expect_throw<model_data<T, 2>>(config["model_dim_missed_nonlocal_radius"]);
        test("model_3d_missed_local_weight" + suffix)    = expect_throw<model_data<T, 3>>(config["model_dim_missed_local_weight"]);
        test("model_3d_missed_nonlocal_radius" + suffix) = expect_throw<model_data<T, 3>>(config["model_dim_missed_nonlocal_radius"]);
        test("model_1d_all_required_exists" + suffix)    = expect_no_throw<model_data<T, 1>>(config["model_1d_all_required_exists"]);
        test("model_2d_all_required_exists" + suffix)    = expect_no_throw<model_data<T, 2>>(config["model_2d_all_required_exists"]);
        test("model_3d_all_required_exists" + suffix)    = expect_no_throw<model_data<T, 3>>(config["model_3d_all_required_exists"]);
    };
}

template<std::floating_point T>
auto material_data_test(const nlohmann::json& config) {
    return [&config] {
        const std::string suffix = '_' + std::string{reflection::type_name<T>()};
        test("material_1d_missed_all" + suffix)            = expect_throw<material_data<mock_data, T, 1>>(config["material_dim_missed_all"]);
        test("material_2d_missed_all" + suffix)            = expect_throw<material_data<mock_data, T, 2>>(config["material_dim_missed_all"]);
        test("material_3d_missed_all" + suffix)            = expect_throw<material_data<mock_data, T, 3>>(config["material_dim_missed_all"]);
        test("material_1d_missed_elements_count" + suffix) = expect_throw<material_data<mock_data, T, 1>>(config["material_1d_missed_elements_count"]);
        test("material_1d_missed_length" + suffix)         = expect_throw<material_data<mock_data, T, 1>>(config["material_1d_missed_length"]);
        test("material_1d_missed_physical" + suffix)       = expect_throw<material_data<mock_data, T, 1>>(config["material_1d_missed_physical"]);
        test("material_1d_all_required_exists" + suffix)   = expect_no_throw<material_data<mock_data, T, 1>>(config["material_1d_all_required_exists"]);
        test("material_2d_all_required_exists" + suffix)   = expect_no_throw<material_data<mock_data, T, 2>>(config["material_dim_all_required_exists"]);
        test("material_3d_all_required_exists" + suffix)   = expect_no_throw<material_data<mock_data, T, 3>>(config["material_dim_all_required_exists"]);
    };
}

template<std::floating_point T>
auto thermal_boundary_condition_data_test(const nlohmann::json& config) {
    return [&config] {
        const std::string suffix = '_' + std::string{reflection::type_name<T>()};
        test("thermal_boundary_condition_missed_all" + suffix)     = expect_throw<thermal_boundary_condition_data<T, 1>>(config["thermal_boundary_condition_missed_all"]);
        test("thermal_boundary_condition_missed_kind" + suffix)    = expect_throw<thermal_boundary_condition_data<T, 1>>(config["thermal_boundary_condition_missed_kind"]);
        test("temperature_condition_missed_temperature" + suffix)  = expect_throw<thermal_boundary_condition_data<T, 1>>(config["temperature_condition_missed_temperature"]);
        test("flux_condition_missed_flux" + suffix)                = expect_throw<thermal_boundary_condition_data<T, 1>>(config["flux_condition_missed_flux"]);
        test("convection_condition_missed_temperature" + suffix)   = expect_throw<thermal_boundary_condition_data<T, 1>>(config["convection_condition_missed_temperature"]);
        test("convection_condition_missed_heat_transfer" + suffix) = expect_throw<thermal_boundary_condition_data<T, 1>>(config["convection_condition_missed_heat_transfer"]);
        test("radiation_condition_missed_emissivity" + suffix)     = expect_throw<thermal_boundary_condition_data<T, 1>>(config["radiation_condition_missed_emissivity"]);
        test("temperature_condition_all_required_exists" + suffix) = expect_no_throw<thermal_boundary_condition_data<T, 1>>(config["temperature_condition_all_required_exists"]);
        test("flux_condition_all_required_exists" + suffix)        = expect_no_throw<thermal_boundary_condition_data<T, 1>>(config["flux_condition_all_required_exists"]);
        test("convection_condition_all_required_exists" + suffix)  = expect_no_throw<thermal_boundary_condition_data<T, 1>>(config["convection_condition_all_required_exists"]);
        test("radiation_condition_all_required_exists" + suffix)   = expect_no_throw<thermal_boundary_condition_data<T, 1>>(config["radiation_condition_all_required_exists"]);
        test("combined_condition_flux_missed_all" + suffix)        = expect_no_throw<thermal_boundary_condition_data<T, 1>>(config["combined_condition_flux_missed_all"]);
    };
}

template<std::floating_point T>
auto thermal_material_data_test(const nlohmann::json& config) {
    return [&config] {
        const std::string suffix = '_' + std::string{reflection::type_name<T>()};
        test("thermal_material_1d_missed_all" + suffix)                      = expect_throw<thermal_material_data<T, 1>>(config["thermal_material_dim_missed_all"]);
        test("thermal_material_2d_missed_all" + suffix)                      = expect_throw<thermal_material_data<T, 2>>(config["thermal_material_dim_missed_all"]);
        test("thermal_material_1d_all_required_exists" + suffix)             = expect_no_throw<thermal_material_data<T, 1>>(config["thermal_material_dim_all_required_exists"]);
        test("thermal_material_2d_all_required_exists" + suffix)             = expect_no_throw<thermal_material_data<T, 2>>(config["thermal_material_dim_all_required_exists"]);
        test("thermal_material_2d_orthotropic_all_required_exists" + suffix) = expect_no_throw<thermal_material_data<T, 2>>(config["thermal_material_2d_orthotropic_all_required_exists"]);
        test("thermal_material_2d_anisotropic_all_required_exists" + suffix) = expect_no_throw<thermal_material_data<T, 2>>(config["thermal_material_2d_anisotropic_all_required_exists"]);
    };
}

const suite<"config_required_fields"> _ = [] {
    const nlohmann::json config = nlohmann::json::parse(required_fields_json_data);
    test("mesh_data") = mesh_data_test(config);
    test("time_data") = time_data_test<double>(config);
    test("boundaries_conditions_data") = boundaries_conditions_data_test<double>(config);
    test("model_data") = model_data_test<double>(config);
    test("material_data") = material_data_test<double>(config);
    test("thermal_boundary_condition") = thermal_boundary_condition_data_test<double>(config);
    test("thermal_material") = thermal_material_data_test<double>(config);
};

}