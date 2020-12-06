cmake_minimum_required(VERSION 3.13)
project(peace)

set(CMAKE_CXX_STANDARD 14)

add_executable(peace main.cpp)
find_library(OPENDHT_LIB opendht)
target_link_libraries(peace PRIVATE "${OPENDHT_LIB}")