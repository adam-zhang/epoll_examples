cmake_minimum_required(VERSION 3.5)
project(server)
add_definitions(-Wall)
set(CMAKE_CXX_STANDARD 11)
include_directories(${CMAKE_SOURCE_DIR}/common)
set(sources main.cpp
	EpollTcpServer.cpp
	)
add_executable(${PROJECT_NAME} ${sources})

