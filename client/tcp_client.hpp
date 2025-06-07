#pragma once

#include <string>

class TcpClient {
public:
    bool connectTo(
        const std::string& host,
        int port);

    bool ping();

private:
    int socket_fd_;
};
