#include "tcp_client.hpp"

int main() {

    TcpClient client;

    if (!client.connectTo(
            "127.0.0.1",
            9000)) {
        return 1;
    }

    client.ping();

    return 0;
}
