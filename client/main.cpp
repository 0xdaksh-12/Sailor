#include <iomanip>
#include <iostream>
#include <string>

#include "tcp_client.hpp"
#include "tls/tls_init.hpp"

int main(int argc, char* argv[]) {
  initializeTls();

  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  int port = (argc > 2) ? std::stoi(argv[2]) : 9000;
  std::string user = (argc > 3) ? argv[3] : "admin";
  std::string pass = (argc > 4) ? argv[4] : "password123";
  std::string path = (argc > 5) ? argv[5] : "/";

  TcpClient client;

  if (!client.connectTo(host, port)) {
    cleanupTls();
    return 1;
  }

  if (client.login(user, pass)) {
    std::cout << "\nListing directory: " << path << "\n" << std::endl;
    std::vector<sailor::fs::DirectoryEntry> entries;

    if (client.list(path, entries)) {
      for (const auto& item : entries) {
        if (item.is_directory) {
          std::cout << " [D] " << item.name << "/" << std::endl;
        } else {
          std::cout << " [F] " << std::left << std::setw(24) << item.name
                    << " (" << item.size << " bytes)" << std::endl;
        }
      }
    }
  }

  client.disconnect();
  cleanupTls();
  return 0;
}
