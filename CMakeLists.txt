cmake_minimum_required(VERSION 3.5)
project(epollTest)
add_definitions(-Wall)
set(CMAKE_CXX_STANDARD 11)
add_subdirectory(server)
add_subdirectory(client)
