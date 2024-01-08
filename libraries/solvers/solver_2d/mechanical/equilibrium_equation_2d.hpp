#ifndef NONLOCAL_EQUILIBRIUM_EQUATION_2D_HPP
#define NONLOCAL_EQUILIBRIUM_EQUATION_2D_HPP

#include "solvers_utils.hpp"
#include "stiffness_matrix_2d.hpp"
#include "mechanical_boundary_conditions_2d.hpp"
#include "boundary_condition_first_kind_2d.hpp"
#include "boundary_condition_second_kind_2d.hpp"
#include "right_part_2d.hpp"
#include "mechanical_solution_2d.hpp"
#include "temperature_condition_2d.hpp"

#include "conjugate_gradient.hpp"

#include <chrono>

namespace nonlocal::mechanical {

template<class Matrix_Index, class T, class I, class Right_Part>
mechanical::mechanical_solution_2d<T, I> equilibrium_equation(const std::shared_ptr<mesh::mesh_2d<T, I>>& mesh,
                                                              const mechanical_parameters_2d<T>& parameters,
                                                              const mechanical_boundaries_conditions_2d<T>& boundaries_conditions,
                                                              const Right_Part& right_part) {
    auto start_time = std::chrono::high_resolution_clock::now();
    stiffness_matrix<T, I, Matrix_Index> stiffness{mesh};
    stiffness.compute(parameters.materials, parameters.plane, utils::inner_nodes(mesh->container(), boundaries_conditions));
    std::chrono::duration<double> elapsed_seconds = std::chrono::high_resolution_clock::now() - start_time;
    std::cout << "Stiffness matrix calculated time: " << elapsed_seconds.count() << 's' << std::endl;

    Eigen::Matrix<T, Eigen::Dynamic, 1> f = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(stiffness.matrix()[matrix_part::INNER].rows());
    boundary_condition_second_kind_2d(f, *mesh, boundaries_conditions);
    integrate_right_part<2>(f, *mesh, right_part);
    temperature_condition(f, *mesh, parameters);

    auto local_parameters = parameters;
    local_parameters.materials["DEFAULT"].model.local_weight = T{1};
    stiffness_matrix<T, I, Matrix_Index> local_stiffness{mesh};
    local_stiffness.compute(local_parameters.materials, local_parameters.plane, utils::inner_nodes(mesh->container(), boundaries_conditions));
    slae::conjugate_gradient<T, Matrix_Index> local_solver{local_stiffness.matrix()[matrix_part::INNER]};
    Eigen::Matrix<T, Eigen::Dynamic, 1> initial = local_solver.solve(f);

    start_time = std::chrono::high_resolution_clock::now();
    slae::conjugate_gradient<
        T, Matrix_Index, 
        Eigen::IncompleteCholesky<T, Eigen::Upper, Eigen::NaturalOrdering<Matrix_Index>>
    > solver{stiffness.matrix()[matrix_part::INNER]};
    solver.preconditioner().compute(local_stiffness.matrix()[matrix_part::INNER]);
    const auto displacement = solver.solve(f, initial);
    elapsed_seconds = std::chrono::high_resolution_clock::now() - start_time;
    std::cout << "SLAE solution time: " << elapsed_seconds.count() << 's' << std::endl;
    std::cout << "Iterations: " << solver.iterations() << std::endl;
    return mechanical_solution_2d<T, I>{mesh, parameters, displacement};
}

}

#endif