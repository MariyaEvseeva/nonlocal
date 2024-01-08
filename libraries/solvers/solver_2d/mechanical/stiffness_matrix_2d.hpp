#ifndef NONLOCAL_STIFFNESS_MATRIX_2D_HPP
#define NONLOCAL_STIFFNESS_MATRIX_2D_HPP

#include "matrix_assembler_2d.hpp"
#include "mechanical_parameters_2d.hpp"

namespace nonlocal::mechanical {

template<class T, class I, class J>
class stiffness_matrix : public matrix_assembler_2d<T, I, J, 2> {
    using _base = matrix_assembler_2d<T, I, J, 2>;
    using hooke_parameter = equation_parameters<2, T, hooke_matrix>;
    using hooke_parameters = std::unordered_map<std::string, hooke_parameter>;
    using block_t = metamath::types::square_matrix<T, 2>;

    static constexpr bool SYMMETRIC = true;

protected:
    template<theory_t Theory>
    static hooke_parameters to_hooke(const parameters_2d<T>& parameters, const plane_t plane);

    static block_t calc_block(const hooke_matrix<T>& hooke, const block_t& integral) noexcept;
    static void add_to_integral(block_t& integral, const std::array<T, 2>& wdN, const std::array<T, 2>& dN) noexcept;
    block_t integrate_loc(const hooke_matrix<T>& hooke, const size_t e, const size_t i, const size_t j) const;
    block_t integrate_nonloc(const hooke_parameter& parameter, const size_t eL, const size_t eNL, const size_t iL, const size_t jNL) const;

    void create_matrix_portrait(const std::unordered_map<std::string, theory_t> theories,
                                const std::vector<bool>& is_inner, const bool is_neumann);

    T integrate_basic(const size_t e, const size_t i) const;
    void integral_condition();

public:
    explicit stiffness_matrix(const std::shared_ptr<mesh::mesh_2d<T, I>>& mesh);
    ~stiffness_matrix() noexcept override = default;

    void compute(const parameters_2d<T>& parameters, const plane_t plane, const std::vector<bool>& is_inner);
};

template<class T, class I, class J>
stiffness_matrix<T, I, J>::stiffness_matrix(const std::shared_ptr<mesh::mesh_2d<T, I>>& mesh)
    : _base{mesh} {}

template<class T, class I, class J>
template<theory_t Theory>
stiffness_matrix<T, I, J>::hooke_parameters stiffness_matrix<T, I, J>::to_hooke(const parameters_2d<T>& parameters, const plane_t plane) {
    hooke_parameters params;
    for(const auto& [group, equation_parameters] : parameters) {
        const T factor = Theory == theory_t::LOCAL ?
                         equation_parameters.model.local_weight :
                         nonlocal_weight(equation_parameters.model.local_weight);
        using namespace metamath::functions;
        params[group] = {
            .model = equation_parameters.model,
            .physical = factor * equation_parameters.physical.hooke(plane)
        };
    }
    return params;
}

template<class T, class I, class J>
metamath::types::square_matrix<T, 2> stiffness_matrix<T, I, J>::calc_block(
    const hooke_matrix<T>& hooke, const metamath::types::square_matrix<T, 2>& integral) noexcept {
    return {
        hooke[0] * integral[X][X] + hooke[2] * integral[Y][Y],
        hooke[1] * integral[X][Y] + hooke[2] * integral[Y][X],
        hooke[1] * integral[Y][X] + hooke[2] * integral[X][Y],
        hooke[0] * integral[Y][Y] + hooke[2] * integral[X][X],
    };
}

template<class T, class I, class J>
void stiffness_matrix<T, I, J>::add_to_integral(block_t& integral, const std::array<T, 2>& wdN, const std::array<T, 2>& dN) noexcept {
    for(const size_t i : std::ranges::iota_view{0u, wdN.size()})
        for(const size_t j : std::ranges::iota_view{0u, dN.size()})
            integral[i][j] += wdN[i] * dN[j];
}

template<class T, class I, class J>
stiffness_matrix<T, I, J>::block_t stiffness_matrix<T, I, J>::integrate_loc(
    const hooke_matrix<T>& hooke, const size_t e, const size_t i, const size_t j) const {
    block_t integral = {};
    const auto& el = _base::mesh().container().element_2d(e);
    for(const size_t q : std::ranges::iota_view{0u, el.qnodes_count()}) {
        using namespace metamath::functions;
        const T weight = el.weight(q) / mesh::jacobian(_base::mesh().jacobi_matrix(e, q));
        add_to_integral(integral, weight * _base::mesh().derivatives(e, i, q), _base::mesh().derivatives(e, j, q));
    }
    return calc_block(hooke, integral);
}

template<class T, class I, class J>
stiffness_matrix<T, I, J>::block_t stiffness_matrix<T, I, J>::integrate_nonloc(
    const hooke_parameter& parameter, const size_t eL, const size_t eNL, const size_t iL, const size_t jNL) const {
    block_t integral = {};
    const auto& elL  = _base::mesh().container().element_2d(eL );
    const auto& elNL = _base::mesh().container().element_2d(eNL);
    for(const size_t qL : elL.qnodes()) {
        using namespace metamath::functions;
        std::array<T, 2> inner_integral = {};
        const auto influence = [&model = parameter.model, &qnodeL = _base::mesh().quad_coord(eL,  qL )](const std::array<T, 2>& qnodeNL) {
            return model.influence(qnodeL, qnodeNL);
        };
        for(const size_t qNL : elNL.qnodes())
            inner_integral += elNL.weight(qNL) * influence(_base::mesh().quad_coord(eNL, qNL)) * _base::mesh().derivatives(eNL, jNL, qNL);
        add_to_integral(integral, elL.weight(qL) * _base::mesh().derivatives(eL, iL, qL), inner_integral);
    }
    return calc_block(parameter.physical, integral);
}

template<class T, class I, class J>
void stiffness_matrix<T, I, J>::create_matrix_portrait(const std::unordered_map<std::string, theory_t> theories,
                                                       const std::vector<bool>& is_inner, const bool is_neumann) {
    const size_t rows = 2 * _base::mesh().process_nodes().size() + (is_neumann && parallel_utils::is_last_process());
    const size_t cols = 2 * _base::mesh().container().nodes_count() + is_neumann;
    _base::matrix()[matrix_part::INNER].resize(rows, cols);
    _base::matrix()[matrix_part::BOUND].resize(rows, cols);
    if (is_neumann)
        for(const size_t row : std::views::iota(0u, _base::mesh().container().nodes_count()))
            _base::matrix()[matrix_part::INNER].outerIndexPtr()[2 * row + 1] = 1;
    _base::init_shifts(theories, is_inner, SYMMETRIC);
    static constexpr bool SORT_INDICES = false;
    _base::init_indices(theories, is_inner, SYMMETRIC, SORT_INDICES);
    if (is_neumann)
        for(const size_t row : std::ranges::iota_view{0u, _base::mesh().container().nodes_count()})
            _base::matrix()[matrix_part::INNER].innerIndexPtr()[_base::matrix()[matrix_part::INNER].outerIndexPtr()[2 * row + 1] - 1] = 2 * _base::mesh().container().nodes_count();
    utils::sort_indices(_base::matrix()[matrix_part::INNER]);
    utils::sort_indices(_base::matrix()[matrix_part::BOUND]);
}

template<class T, class I, class J>
T stiffness_matrix<T, I, J>::integrate_basic(const size_t e, const size_t i) const {
    T integral = 0;
    const auto& el = _base::mesh().container().element_2d(e);
    for(const size_t q : el.qnodes())
        integral += el.weight(q) * el.qN(i, q) * mesh::jacobian(_base::mesh().jacobi_matrix(e, q));
    return integral;
}

template<class T, class I, class J>
void stiffness_matrix<T, I, J>::integral_condition() {
    const auto process_nodes = _base::mesh().process_nodes();
#pragma omp parallel for default(none) shared(process_nodes)
    for(size_t node = process_nodes.front(); node < *process_nodes.end(); ++node) {
        T& val = _base::matrix()[matrix_part::INNER].coeffRef(2 * (node - process_nodes.front()), 2 * _base::mesh().container().nodes_count());
        for(const I e : _base::mesh().elements(node))
            val += integrate_basic(e, _base::mesh().global_to_local(e, node));
    }
}

template<class T, class I, class J>
void stiffness_matrix<T, I, J>::compute(const parameters_2d<T>& parameters, const plane_t plane, const std::vector<bool>& is_inner) {
    const std::unordered_map<std::string, theory_t> theories = theories_types(parameters);
    static constexpr bool NEUMANN = false;
    create_matrix_portrait(theories, is_inner, NEUMANN);
    _base::calc_coeffs(theories, is_inner, SYMMETRIC,
        [this, hooke = to_hooke<theory_t::LOCAL>(parameters, plane)]
        (const std::string& group, const size_t e, const size_t i, const size_t j) {
            return integrate_loc(hooke.at(group).physical, e, i, j);
        },
        [this, hooke = to_hooke<theory_t::NONLOCAL>(parameters, plane)]
        (const std::string& group, const size_t eL, const size_t eNL, const size_t iL, const size_t jNL) {
            return integrate_nonloc(hooke.at(group), eL, eNL, iL, jNL);
        }
    );
    if (NEUMANN)
        integral_condition();
}

}

#endif