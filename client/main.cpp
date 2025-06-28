#include <string>

#include "tcp_client.hpp"
#include "tls/tls_init.hpp"

int main(int argc, char* argv[]) {
  initializeTls();

  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  int port = (argc > 2) ? std::stoi(argv[2]) : 9000;
  std::string user = (argc > 3) ? argv[3] : "admin";
  std::string pass = (argc > 4) ? argv[4] : "password123";

  TcpClient client;

  if (!client.connectTo(host, port)) {
    cleanupTls();
    return 1;
  }

  // Ping before login (testing pre-auth behavior)
  client.ping();

  // Authenticate
  if (client.login(user, pass)) {
    // Ping after login over the authenticated session
    client.ping();
  }

  client.disconnect();
  cleanupTls();
  return 0;
}
