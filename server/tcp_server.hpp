#pragma once

#include <string>

#include "tls/tls_server.hpp"

class TcpServer {
 public:
  TcpServer(int port, std::string cert_path, std::string key_path);
  ~TcpServer();

  bool start();
  void run();

 private:
  int port_;
  int server_fd_{-1};
  TlsServerContext tls_context_;
};
