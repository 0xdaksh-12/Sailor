cmake_minimum_required(VERSION 3.10)

project(Sailor)

set (CMAKE_CXX_STANDARD 17)

add_executable(server server/server.cpp)
add_executable(client client/client.cpp)

