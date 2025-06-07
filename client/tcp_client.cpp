#include "tcp_client.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

bool TcpClient::connectTo(
    const std::string& host,
    int port) {

    socket_fd_ =
        socket(
            AF_INET,
            SOCK_STREAM,
            0);

    if (socket_fd_ < 0) {
        return false;
    }

    sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    inet_pton(
        AF_INET,
        host.c_str(),
        &addr.sin_addr);

    if (connect(
            socket_fd_,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) < 0) {
        perror("connect");
        return false;
    }

    return true;
}

bool TcpClient::ping() {

    send(
        socket_fd_,
        "PING",
        4,
        0);

    char buffer[1024]{};

    ssize_t received =
        recv(
            socket_fd_,
            buffer,
            sizeof(buffer),
            0);

    if (received <= 0) {
        return false;
    }

    std::cout
        << "Response: "
        << std::string(buffer, received)
        << std::endl;

    close(socket_fd_);

    return true;
}
