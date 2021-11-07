cmake_minimum_required (VERSION 2.8)

#### USER DEFINED ##############################################################

set(GROWS_HASHFCT XXHASH CACHE STRING
  "Changes the used hash function if XXHASH is not available, MURMUR2 is used as backoff!")
set_property(CACHE GROWS_HASHFCT PROPERTY STRINGS XXHASH MURMUR2 MURMUR3 CRC)

#### BASIC SETTINGS ############################################################

include_directories(.)

set (CMAKE_CXX_FLAGS "-std=c++14 -msse4.2 -Wall -Wextra -O3 -g")

#### HASH TABLES ###############################################################

set(HASH_LIST "hash_std;hash_original;hash_a;hash_b;hash_c")

foreach(h ${HASH_LIST})
  string(TOUPPER ${h} h_uc)
  add_executable(${h} tests/time.cpp)
  target_compile_definitions(${h} PRIVATE -D ${h_uc})
endforeach()
