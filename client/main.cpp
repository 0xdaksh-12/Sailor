#include "tcp_client.hpp"
#include "tls/tls_init.hpp"

#include <string>

int main(int argc, char* argv[]) {
  initializeTls();

  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  int port = (argc > 2) ? std::stoi(argv[2]) : 9000;

  TcpClient client;

  if (!client.connectTo(host, port)) {
    cleanupTls();
    return 1;
  }

  client.ping();
  client.disconnect();

  cleanupTls();
  return 0;
}
