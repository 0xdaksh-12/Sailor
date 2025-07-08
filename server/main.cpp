#include "tcp_server.hpp"
#include "tls/tls_init.hpp"

#include <string>

int main(int argc, char* argv[]) {
  initializeTls();

  std::string cert_path = (argc > 1) ? argv[1] : "certs/cert.pem";
  std::string key_path = (argc > 2) ? argv[2] : "certs/key.pem";
  int port = (argc > 3) ? std::stoi(argv[3]) : 9000;
  std::string user_db = (argc > 4) ? argv[4] : "data/users.db";
  std::string storage_dir = (argc > 5) ? argv[5] : "server_storage";

  TcpServer server(port, cert_path, key_path, user_db, storage_dir);
  if (!server.start()) {
    cleanupTls();
    return 1;
  }

  server.run();

  cleanupTls();
  return 0;
}
