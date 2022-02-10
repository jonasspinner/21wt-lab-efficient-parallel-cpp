cmake_minimum_required(VERSION 3.12)
project(Exercise5)

include_directories(./src)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-std=c++20 -Wall -Wextra -mcx16 -lpthread)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Wextra -mcx16 -lpthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")


# See: https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
#         URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_compile_definitions("TBB_TEST=OFF")
FetchContent_Declare(
        oneTBB
        URL https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.5.0.zip
)
FetchContent_MakeAvailable(oneTBB)


#find_package(benchmark REQUIRED)
#find_package(Threads REQUIRED)
#include_directories(${CMAKE_CURRENT_LIST_DIR}/extern/oneTBB)
#find_package(TBB REQUIRED HINTS ${CMAKE_CURRENT_LIST_DIR}/extern/oneTBB)
#add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/oneTBB" EXCLUDE_FROM_ALL)
#include_directories("${CMAKE_CURRENT_LIST_DIR}/extern/oneTBB/include")

#add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/growt" EXCLUDE_FROM_ALL)

#### TARGETS ###################################################################


add_executable(external_maps app/external_maps.cpp)
target_link_libraries(external_maps TBB::tbb TBB::tbbmalloc)

add_executable(generate app/generate.cpp)

#### TESTS #####################################################################

enable_testing()
include(GoogleTest)

add_executable(list_test tests/list_test.cpp)
target_link_libraries(list_test gtest gtest_main TBB::tbb TBB::tbbmalloc)
gtest_discover_tests(list_test)

add_executable(bucket_test tests/bucket_test.cpp)
target_link_libraries(bucket_test gtest gtest_main)
gtest_discover_tests(bucket_test)

add_executable(atomic_shared_ptr_test tests/atomic_shared_ptr_test.cpp)
target_link_libraries(atomic_shared_ptr_test gtest gtest_main)
gtest_discover_tests(atomic_shared_ptr_test)

add_executable(marked_ptr_test tests/marked_ptr_test.cpp)
target_link_libraries(marked_ptr_test gtest gtest_main)
gtest_discover_tests(marked_ptr_test)

add_executable(hash_map_test tests/hash_map_test.cpp)
target_link_libraries(hash_map_test gtest gtest_main TBB::tbb TBB::tbbmalloc)
gtest_discover_tests(hash_map_test)


#### BENCHMARKS ################################################################

add_executable(list_benchmark benchmarks/list_benchmark.cpp)
target_link_libraries(list_benchmark TBB::tbb TBB::tbbmalloc)

add_executable(hash_map_benchmark benchmarks/hash_map_benchmark.cpp)
target_link_libraries(hash_map_benchmark TBB::tbb TBB::tbbmalloc)

