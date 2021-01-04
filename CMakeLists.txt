cmake_minimum_required(VERSION 3.13)
project(peace)

set(CMAKE_CXX_STANDARD 14)

add_executable(peace peace.cpp)
find_library(OPENDHT_LIB opendht)
# find_package(OpenSSL REQUIRED)
target_link_libraries(peace PRIVATE "${OPENDHT_LIB}")
# target_link_libraries(peace PRIVATE OpenSSL::SSL)