cmake_minimum_required(VERSION 3.17)
project(Peace)

set(CMAKE_CXX_STANDARD 14)

add_executable(Peace main.cpp)
find_library(OPENDHT_LIB opendht)
target_link_libraries(Peace PRIVATE "${OPENDHT_LIB}")