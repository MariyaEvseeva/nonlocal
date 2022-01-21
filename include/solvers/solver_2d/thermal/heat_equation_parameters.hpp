#ifndef HEAT_EQUATION_PARAMETERS_HPP
#define HEAT_EQUATION_PARAMETERS_HPP

#include "../../solvers_constants.hpp"
#include <array>
#include <string>
#include <vector>
#include <unordered_map>

namespace nonlocal::thermal {

template<class T, material_t Material>
struct equation_parameters final {
    static_assert(Material == material_t::ISOTROPIC ||
                  Material == material_t::ORTHOTROPIC,
                  "Only isotropic and orthotropic materials are supported");

    std::conditional_t<
        Material == material_t::ISOTROPIC,
        T,
        std::array<T, 2>
    > lambda;                                 // Коэффициент теплопроводности
    std::unordered_map<std::string, T> alpha; // Коэффициент теплоотдачи
    T c        = T{1};                        // Коэффициент теплоёмкости
    T rho      = T{1};                        // Плотность материала
    T integral = T{0};                        // Интеграл от решения (для задачи Неймана)

    explicit equation_parameters(const std::vector<std::string>& names = {}) noexcept {
        if constexpr(Material == material_t::ISOTROPIC)
            lambda = T{1};
        if constexpr(Material == material_t::ORTHOTROPIC)
            lambda.fill(T{1});
        for(const std::string& name : names)
            alpha[name] = T{1};
    }
};

/*
template<class T>
struct solver_parameters final {
    std::string save_path; // Путь куда сохранять данные
    std::array<T, 2> time_interval = {0, 1};
    uintmax_t steps = 100,
              save_freq = 1; // Частота сохранения
    bool save_vtk = true,    // Сохранять .vtk файлы
         save_csv = true,    // Сохранять .csv файлы в формате (x1, x2, T)
         calc_energy = true; // Вычислять энергия при сохранении, иногда полезно для контроля расчёта
};
*/

}

#endif