cmake_minimum_required (VERSION 2.8)

#### USER DEFINED ##############################################################

#### BASIC SETTINGS ############################################################

include_directories(.)

set (CMAKE_CXX_FLAGS "-std=c++2a -msse4.2 -Wall -Wextra -O3 -g")

#### TARGETS ###################################################################

# set(PROGRAM_LIST "graph_naive;graph_adj_list;graph_adj_array")

# foreach(p ${PROGRAM_LIST})
#   string(TOUPPER ${p} p_uc)
#   add_executable(${p} tests/time.cpp)
#   target_compile_definitions(${p} PRIVATE -D ${p_uc})
# endforeach()

set(GTEST_ROOT /usr/src/googletest)

find_path(GTEST_ROOT googletest/CMakeLists.txt HINTS ${GTEST_ROOT})

if (NOT ${GTEST_ROOT} STREQUAL GTEST_ROOT-NOTFOUND)
    enable_testing()

    add_subdirectory(${GTEST_ROOT} ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

    add_executable(graph_test tests/correctness.cpp)
    target_link_libraries(graph_test gtest_main)

    include(GoogleTest)
    gtest_discover_tests(graph_test)
endif()
