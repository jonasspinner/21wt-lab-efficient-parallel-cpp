cmake_minimum_required (VERSION 3.10)

#### BASIC SETTINGS ############################################################

project(priority-queues)
set(CMAKE_CXX_STANDARD 17)

include_directories(.)

set (CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -g -O3 -march=native")

#### HASH TABLES ###############################################################

set(PQ_LIST "pq_std;pq_a;pq_b;pq_c;pq_d")

foreach(pq ${PQ_LIST})
  string(TOUPPER ${pq} pq_uc)
  add_executable(${pq} tests/time.cpp)
  target_compile_definitions(${pq} PRIVATE -D ${pq_uc})
endforeach()

#### TESTS #####################################################################

set(GTEST_ROOT_HINTS "/usr/src/googletest ./googletest")

find_path(GTEST_ROOT googletest/CMakeLists.txt HINTS ${GTEST_ROOT_HINTS})

if (NOT ${GTEST_ROOT} STREQUAL GTEST_ROOT-NOTFOUND)
    enable_testing()

    add_subdirectory(${GTEST_ROOT} ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

    add_executable(pq_test tests/correctness.cpp)
    target_link_libraries(pq_test gtest_main)

    include(GoogleTest)
    gtest_discover_tests(pq_test)
endif()
