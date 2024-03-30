cmake_minimum_required(VERSION 3.26)
project(Client)

set(CMAKE_CXX_STANDARD 23)

add_executable(Client main.cpp)

target_link_libraries(Client Ws2_32)
