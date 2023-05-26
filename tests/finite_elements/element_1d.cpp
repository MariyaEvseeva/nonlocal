#include "init_element.hpp"

#include <boost/ut.hpp>

#include <numeric>
#include <memory>
#include <iostream>

namespace {

using namespace boost::ut;
using namespace metamath::utils;
using namespace metamath::finite_element;

template<class T>
struct basis_summator final {
    const element_1d_integrate_base<T>& element;
    T point = T{0};

    std::pair<T, T> operator()(const std::pair<T, T>& sum, const size_t i) {
        return {
            sum.first + element.N(i, point),
            sum.second + element.Nxi(i, point)
        };
    }
};

const suite<"lagrangian_element_1d"> _ = [] {
    test("lagrangian_element_1d") = []<class T> {
        size_t order = 0;
        for(const auto& element : unit_tests::init_elements<T>()) {
            const std::string suffix = "_order_" + std::to_string(order) + '_' + std::string{reflection::type_name<T>()};

            test("nodes_count" + suffix) = [&element, order] {
                expect(eq(element->nodes_count(), order + 1)) << "Unexpected nodes count.";
            };

            test("boundaries" + suffix) = [&element] {
                expect(eq(element->boundary(side_1d::LEFT), T{-1})) << "Unexpected left boundary position.";
                expect(eq(element->boundary(side_1d::RIGHT), T{1})) << "Unexpected right boundary position.";
            };

            test("node_values" + suffix) = [&element, nodes = element->nodes()] {
                for(const size_t i : nodes)
                    for(const size_t j : nodes)
                        expect(eq(element->N(i, element->node(j)), T(i == j))) <<
                            "Unexpected value of function " + std::to_string(i) + " at node " + std::to_string(j) + '.';
            };

            const T element_length = element->boundary(side_1d::RIGHT) - element->boundary(side_1d::LEFT);

            test("weights_sum" + suffix) = [&element, element_length] {
                const auto qnodes = element->qnodes();
                const auto weight_summator = [&element](const T sum, const size_t qnode) {
                    return sum + element->weight(qnode);
                };
                const T weights_sum = std::accumulate(qnodes.begin(), qnodes.end(), T{0}, weight_summator);
                static constexpr T epsilon = std::is_same_v<T, float> ? T{1e-7} : T{1e-16};
                expect(approx(weights_sum, element_length, epsilon)) <<
                    "The weights sum does not match the element length.";
            };

            test("element_integral" + suffix) = [&element, element_length] {
                const auto integrator = [&element](const T sum, const size_t q) {
                    const auto nodes = element->nodes();
                    const auto& summator = [&element, q](const T sum, const size_t i) { return sum + element->qN(i, q); };
                    return sum + element->weight(q) * std::accumulate(nodes.begin(), nodes.end(), T{0}, summator);
                };
                const auto qnodes = element->qnodes();
                const T integral = std::accumulate(qnodes.begin(), qnodes.end(), T{0}, integrator);
                static constexpr T epsilon = std::is_same_v<T, float> ? T{1e-7} : T{5e-16};
                expect(approx(integral, element_length, epsilon)) << 
                    "The sum of the integrals of all basis functions does not match with the element length";
            };

            static constexpr auto points = std::array{T{-0.89}, T{-0.5234}, T{-0.03}, T{0.11}, T{0.57}, T{0.9844}}; // some test points

            basis_summator<T> summator{*element};
            test("basis_sum" + suffix) = [&element, &summator, nodes = element->nodes()] {
                for(const T point : points) {
                    summator.point = point;
                    const auto [N_sum, Nxi_sum] = std::accumulate(nodes.begin(), nodes.end(), std::pair{T{0}, T{0}}, summator);
                    static constexpr T epsilon_N = std::is_same_v<T, float> ? T{2e-7} : T{4e-16};
                    expect(approx(N_sum, T{1}, epsilon_N)) << 
                        "Unexpected basis functions sum in point = " + std::to_string(point) + '.';
                    static constexpr T epsilon_Nxi = std::is_same_v<T, float> ? T{3e-6} : T{5e-15};
                    expect(approx(Nxi_sum, T{0}, epsilon_Nxi)) << 
                        "Unexpected basis functions derivatives sum in point = " + std::to_string(point) + '.';
                }                
            };

            test("basis_incomplete_sum" + suffix) = [&element, &summator, nodes = element->nodes()] {
                for(const T point : points) {
                    summator.point = point;
                    const auto [N_sum, Nxi_sum] = std::accumulate(nodes.begin(), std::ranges::prev(nodes.end()), std::pair{T{0}, T{0}}, summator);
                    static constexpr T epsilon = std::is_same_v<T, float> ? T{1e-7} : T{1e-16};
                    expect(ge(std::abs(N_sum - T{1}), element->nodes_count() > 1 ? epsilon : T{0}))
                        << "The partial sum at point " + std::to_string(point) + " must not be equal to 1.";
                    expect(ge(std::abs(Nxi_sum), element->nodes_count() > 1 ? epsilon : T{0}))
                        << "The partial sum at point " + std::to_string(point) + " must not be equal to 0.";
                }
            };

            ++order;
        }
    } | std::tuple<float, double, long double>{};
};

}