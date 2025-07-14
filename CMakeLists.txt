cmake_minimum_required(VERSION 3.16)

project(Sailor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

set(SHARED_SOURCES
    common/socket_utils.cpp
    common/hash.cpp
    protocol/packet_serializer.cpp
    protocol/packet_io.cpp
    protocol/directory_serializer.cpp
    transport/tcp_transport.cpp
    transport/tls_transport.cpp
    tls/tls_server.cpp
    tls/tls_client.cpp
    auth/user_store.cpp
    auth/auth_manager.cpp
    filesystem/file_service.cpp
    filesystem/upload_service.cpp
)

add_executable(
    sailor-server
    server/main.cpp
    server/tcp_server.cpp
    ${SHARED_SOURCES}
)

add_executable(
    sailor-client
    client/main.cpp
    client/tcp_client.cpp
    ${SHARED_SOURCES}
)

target_include_directories(
    sailor-server
    PRIVATE
    server
    common
    protocol
    transport
    tls
    auth
    filesystem
    .
)

target_include_directories(
    sailor-client
    PRIVATE
    client
    common
    protocol
    transport
    tls
    auth
    filesystem
    .
)

target_link_libraries(
    sailor-server
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries(
    sailor-client
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)
