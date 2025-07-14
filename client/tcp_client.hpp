#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/connection_state.hpp"
#include "filesystem/directory_entry.hpp"
#include "tls/tls_client.hpp"
#include "transport/tls_transport.hpp"

class TcpClient {
 public:
  using ProgressCallback =
      std::function<void(uint64_t bytes_sent, uint64_t total_bytes)>;

  TcpClient();
  ~TcpClient();

  bool connectTo(const std::string& host, int port);
  bool login(const std::string& username, const std::string& password);
  bool ping();
  bool list(const std::string& path,
            std::vector<sailor::fs::DirectoryEntry>& out_entries);
  bool upload(const std::string& local_file_path, const std::string& remote_dir,
              ProgressCallback progress_cb = nullptr);
  void disconnect();

  bool isAuthenticated() const {
    return state_ == ConnectionState::AUTHENTICATED;
  }
  uint64_t sessionId() const { return session_id_; }

 private:
  int socket_fd_{-1};
  uint64_t session_id_{0};
  ConnectionState state_{ConnectionState::DISCONNECTED};
  TlsClientContext tls_context_;
  std::unique_ptr<TlsTransport> transport_{nullptr};
};
