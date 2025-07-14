#include <iomanip>
#include <iostream>
#include <string>

#include "tcp_client.hpp"
#include "tls/tls_init.hpp"

void printProgressBar(uint64_t sent, uint64_t total) {
  if (total == 0) return;
  double ratio = static_cast<double>(sent) / static_cast<double>(total);
  int percent = static_cast<int>(ratio * 100.0);
  int bar_width = 40;

  std::cout << "\r[";
  int pos = static_cast<int>(bar_width * ratio);
  for (int i = 0; i < bar_width; ++i) {
    if (i < pos)
      std::cout << "=";
    else if (i == pos)
      std::cout << ">";
    else
      std::cout << " ";
  }
  std::cout << "] " << std::setw(3) << percent << "% (" << (sent / 1024) << "/"
            << (total / 1024) << " KB)" << std::flush;
}

int main(int argc, char* argv[]) {
  initializeTls();

  std::string host = "127.0.0.1";
  int port = 9000;
  std::string user = "admin";
  std::string pass = "password123";

  // Mode: "list" (default) or "upload"
  std::string command = (argc > 1) ? argv[1] : "list";

  TcpClient client;
  if (!client.connectTo(host, port)) {
    cleanupTls();
    return 1;
  }

  if (!client.login(user, pass)) {
    client.disconnect();
    cleanupTls();
    return 1;
  }

  if (command == "upload") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " upload <local_file> [remote_dir]"
                << std::endl;
      client.disconnect();
      cleanupTls();
      return 1;
    }

    std::string local_file = argv[2];
    std::string remote_dir = (argc > 3) ? argv[3] : "/";

    std::cout << "Starting upload: " << local_file << " -> " << remote_dir
              << std::endl;
    bool ok = client.upload(local_file, remote_dir, printProgressBar);
    std::cout << (ok ? "\nUpload complete." : "\nUpload failed.") << std::endl;
  } else {
    std::string path = (argc > 2) ? argv[2] : "/";
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
