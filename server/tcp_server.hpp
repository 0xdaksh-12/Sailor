#pragma once

#include <random>
#include <string>

#include "auth/auth_manager.hpp"
#include "session.hpp"
#include "tls/tls_server.hpp"

class TcpServer {
 public:
  TcpServer(int port, std::string cert_path, std::string key_path,
            std::string user_db_path = "data/users.db");
  ~TcpServer();

  bool start();
  void run();

 private:
  void handleClient(int client_fd);
  uint64_t generateSessionId();

  int port_;
  int server_fd_{-1};
  TlsServerContext tls_context_;
  sailor::auth::AuthManager auth_manager_;
  std::mt19937_64 rng_;
};
