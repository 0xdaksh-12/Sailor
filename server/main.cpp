#include "tcp_server.hpp"

int main() {
    TcpServer server(9000);

    if (!server.start()) {
        return 1;
    }

    server.run();

    return 0;
}
