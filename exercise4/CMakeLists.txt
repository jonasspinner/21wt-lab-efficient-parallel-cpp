cmake_minimum_required (VERSION 2.8)

#### USER DEFINED ##############################################################

#### BASIC SETTINGS ############################################################

include_directories(.)

set (CMAKE_CXX_FLAGS "-std=c++17 -msse4.2 -Wall -Wextra -O3 -g")

#### TARGETS ###################################################################

add_executable(tree source/correctness_tree.cpp)
add_executable(dag  source/correctness_dag.cpp)

add_executable(generate_tree tools/tree_generator.cpp)
add_executable(generate_graph tools/graph_generator.cpp)
