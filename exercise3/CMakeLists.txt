cmake_minimum_required (VERSION 3.9)

#### BASIC SETTINGS ############################################################

project("exercise 3" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_FLAGS "-march=native -Wall -Wextra -Wpedantic -O3")

include_directories(.)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(OpenMP REQUIRED)

#### TARGETS ###################################################################

add_executable(test_correctness tests/correctness.cpp)
target_sources(test_correctness PRIVATE implementation/dynamic_connectivity.cpp)
target_link_libraries(test_correctness PRIVATE Threads::Threads)
target_link_libraries(test_correctness PRIVATE OpenMP::OpenMP_CXX)

add_executable(graph_construction tools/graph_generator.cpp)
target_sources(graph_construction PRIVATE tools/graph_generator.cpp)
#target_link_libraries(graph_construction Threads::Threads)

add_executable(microbenchmark_access_patterns implementation/microbenchmark_access_patterns.cpp)
target_link_libraries(microbenchmark_access_patterns PRIVATE Threads::Threads)
target_link_libraries(microbenchmark_access_patterns PRIVATE OpenMP::OpenMP_CXX)
