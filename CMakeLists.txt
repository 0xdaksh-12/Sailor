cmake_minimum_required(VERSION 3.16)

project(Sailor VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

set(CORE_SOURCES
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
    filesystem/download_service.cpp
    filesystem/delete_service.cpp
    filesystem/rename_service.cpp
    filesystem/directory_service.cpp
)

# libsailor Shared Library (Target for Flutter FFI)
add_library(sailor SHARED
    sdk/src/sailor_ffi.cpp
    sdk/src/sailor_client_internal.cpp
    ${CORE_SOURCES}
)

target_compile_definitions(sailor PRIVATE SAILOR_EXPORTS)

target_include_directories(sailor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/sdk/src
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol
        ${CMAKE_CURRENT_SOURCE_DIR}/transport
        ${CMAKE_CURRENT_SOURCE_DIR}/tls
        ${CMAKE_CURRENT_SOURCE_DIR}/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/filesystem
)

target_link_libraries(sailor
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Server Executable
add_executable(sailor-server
    server/main.cpp
    server/tcp_server.cpp
    ${CORE_SOURCES}
)

target_include_directories(sailor-server PRIVATE
    server common protocol transport tls auth filesystem .
)

target_link_libraries(sailor-server PRIVATE
    OpenSSL::SSL OpenSSL::Crypto Threads::Threads
)

# Client CLI Executable (Uses libsailor)
add_executable(sailor-client client/main.cpp)

target_include_directories(sailor-client PRIVATE
    client sdk/include tls
)

target_link_libraries(sailor-client PRIVATE
    sailor OpenSSL::SSL OpenSSL::Crypto
)
