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


set(TASK_LIST "sequential;b;c;d;e")

foreach(t ${TASK_LIST})
    string(TOUPPER ${t} t_uc)

    add_executable(test_correctness_${t} tests/correctness.cpp)
    target_compile_definitions(test_correctness_${t} PRIVATE DC_${t_uc})
    target_sources(test_correctness_${t} PRIVATE implementation/dynamic_connectivity_${t}.cpp)

    target_link_libraries(test_correctness_${t} PRIVATE Threads::Threads)
    target_link_libraries(test_correctness_${t} PRIVATE OpenMP::OpenMP_CXX)
endforeach()

add_executable(test_correctness tests/correctness.cpp)
target_compile_definitions(test_correctness PRIVATE DC_F)
target_sources(test_correctness PRIVATE implementation/dynamic_connectivity.cpp)
target_link_libraries(test_correctness PRIVATE Threads::Threads)
target_link_libraries(test_correctness PRIVATE OpenMP::OpenMP_CXX)


add_executable(graph_construction tools/graph_generator.cpp)
target_sources(graph_construction PRIVATE tools/graph_generator.cpp)
#target_link_libraries(graph_construction Threads::Threads)


add_executable(microbenchmark_access_patterns tests/microbenchmark_access_patterns.cpp)
target_link_libraries(microbenchmark_access_patterns PRIVATE Threads::Threads)
target_link_libraries(microbenchmark_access_patterns PRIVATE OpenMP::OpenMP_CXX)



foreach(t ${TASK_LIST})
    string(TOUPPER ${t} t_uc)

    add_executable(benchmark_dynamic_connectivity_${t} tests/benchmark_dynamic_connectivity.cpp)
    target_compile_definitions(benchmark_dynamic_connectivity_${t} PRIVATE DC_${t_uc})
    target_sources(benchmark_dynamic_connectivity_${t} PRIVATE implementation/dynamic_connectivity_${t}.cpp)

    target_link_libraries(benchmark_dynamic_connectivity_${t} PRIVATE Threads::Threads)
    target_link_libraries(benchmark_dynamic_connectivity_${t} PRIVATE OpenMP::OpenMP_CXX)
endforeach()

add_executable(benchmark_dynamic_connectivity tests/benchmark_dynamic_connectivity.cpp)
target_compile_definitions(benchmark_dynamic_connectivity PRIVATE DC_F)
target_sources(benchmark_dynamic_connectivity PRIVATE implementation/dynamic_connectivity.cpp)
target_link_libraries(benchmark_dynamic_connectivity PRIVATE Threads::Threads)
target_link_libraries(benchmark_dynamic_connectivity PRIVATE OpenMP::OpenMP_CXX)

