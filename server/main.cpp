#include "tcp_server.hpp"
#include "tls/tls_init.hpp"

#include <string>

int main(int argc, char* argv[]) {
  initializeTls();

  std::string cert_path = (argc > 1) ? argv[1] : "certs/cert.pem";
  std::string key_path = (argc > 2) ? argv[2] : "certs/key.pem";
  int port = (argc > 3) ? std::stoi(argv[3]) : 9000;

  TcpServer server(port, cert_path, key_path);
  if (!server.start()) {
    return 1;
  }

  server.run();

  cleanupTls();
  return 0;
}
