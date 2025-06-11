cmake_minimum_required(VERSION 3.16)

project(Sailor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared protocol and utility sources
set(COMMON_SOURCES
    common/socket_utils.cpp
    protocol/packet_serializer.cpp
    protocol/packet_io.cpp
)

add_executable(
    sailor-server
    server/main.cpp
    server/tcp_server.cpp
    ${COMMON_SOURCES}
)

add_executable(
    sailor-client
    client/main.cpp
    client/tcp_client.cpp
    ${COMMON_SOURCES}
)

target_include_directories(
    sailor-server
    PRIVATE
    server
    common
    protocol
    .
)

target_include_directories(
    sailor-client
    PRIVATE
    client
    common
    protocol
    .
)
