cmake_minimum_required(VERSION 3.16)
project(finite_element_solver_1d)

add_subdirectory(base)
add_subdirectory(thermal)

add_library(finite_element_solver_1d_lib INTERFACE)
target_sources(finite_element_solver_1d_lib INTERFACE influence_functions_1d.hpp)
target_include_directories(finite_element_solver_1d_lib INTERFACE ${LIBRARIES_DIR})
target_link_libraries(finite_element_solver_1d_lib INTERFACE
    finite_element_solver_1d_base_lib
    heat_equation_solver_1d_lib
)