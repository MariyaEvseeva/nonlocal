cmake_minimum_required(VERSION 3.16)

project(logger)

add_library(logger_lib STATIC logger.cpp)
target_include_directories(logger_lib PUBLIC ${LIBRARIES_DIR})
target_link_libraries(logger_lib parallel_lib)