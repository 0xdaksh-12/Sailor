cmake_minimum_required(VERSION 3.16)

project(Sailor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    sailor-server
    server/main.cpp
    server/tcp_server.cpp
)

add_executable(
    sailor-client
    client/main.cpp
    client/tcp_client.cpp
)

target_include_directories(
    sailor-server
    PRIVATE
    server
    common
)

target_include_directories(
    sailor-client
    PRIVATE
    client
    common
)
