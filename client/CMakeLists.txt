cmake_minimum_required(VERSION 3.5)
project(client)
add_definitions(-Wall)
set(CMAKE_CXX_STANDARD 11)
message("source_dir:${CMAKE_SOURCE_DIR}")
include_directories(${CMAKE_SOURCE_DIR}/common)
set(sources main.cpp
	EpollTcpClient.cpp
	)
add_executable(${PROJECT_NAME} ${sources})
