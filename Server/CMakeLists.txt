cmake_minimum_required(VERSION 3.26)
project(Server)

set(CMAKE_CXX_STANDARD 23)

add_executable(Server main.cpp)

target_link_libraries(Server Ws2_32)
