cmake_minimum_required (VERSION 2.8)

project(Exercise4)

#### USER DEFINED ##############################################################

#### BASIC SETTINGS ############################################################

include_directories(.)

set (CMAKE_CXX_FLAGS "-std=c++17 -msse4.2 -Wall -Wextra -O3 -g")

#### TARGETS ###################################################################

add_executable(tree source/correctness_tree.cpp)
add_executable(dag  source/correctness_dag.cpp)

add_executable(generate_tree tools/tree_generator.cpp)
add_executable(generate_graph tools/graph_generator.cpp)

add_executable(microbenchmark_data_structures source/microbenchmark_data_structures.cpp)

add_executable(eval_tree source/eval_tree.cpp)
add_executable(eval_dag source/eval_dag.cpp)


add_executable(tree_ts source/correctness_tree.cpp)
target_compile_options(tree_ts PRIVATE -fsanitize=thread)
target_link_options(tree_ts PRIVATE -fsanitize=thread)

add_executable(dag_ts source/correctness_dag.cpp)
target_compile_options(dag_ts PRIVATE -fsanitize=thread)
target_link_options(dag_ts PRIVATE -fsanitize=thread)


add_executable(correctness_concurrent_queue source/correctness_concurrent_queue.cpp)

add_executable(correctness_concurrent_queue_ts source/correctness_concurrent_queue.cpp)
target_compile_options(correctness_concurrent_queue_ts PRIVATE -fsanitize=thread)
target_link_options(correctness_concurrent_queue_ts PRIVATE -fsanitize=thread)


add_executable(correctness_concurrent_container source/correctness_concurrent_container.cpp)

add_executable(correctness_concurrent_container_ts source/correctness_concurrent_container.cpp)
target_compile_options(correctness_concurrent_container_ts PRIVATE -fsanitize=thread)
target_link_options(correctness_concurrent_container_ts PRIVATE -fsanitize=thread)
