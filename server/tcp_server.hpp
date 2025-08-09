#pragma once

#include <random>
#include <string>

#include "auth/auth_manager.hpp"
#include "filesystem/delete_service.hpp"
#include "filesystem/directory_service.hpp"
#include "filesystem/download_service.hpp"
#include "filesystem/file_service.hpp"
#include "filesystem/rename_service.hpp"
#include "filesystem/upload_service.hpp"
#include "tls/tls_server.hpp"

class TcpServer {
 public:
  TcpServer(int port, std::string cert_path, std::string key_path,
            std::string user_db_path = "data/users.db",
            std::string storage_root = "server_storage");
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
  sailor::fs::FileService file_service_;
  sailor::fs::UploadService upload_service_;
  sailor::fs::DownloadService download_service_;
  sailor::fs::DeleteService delete_service_;
  sailor::fs::RenameService rename_service_;
  sailor::fs::DirectoryService directory_service_;
  std::mt19937_64 rng_;
};
