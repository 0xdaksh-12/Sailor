#pragma once

#include <memory>
#include <string>

#include "common/connection_state.hpp"
#include "tls/tls_client.hpp"
#include "transport/tls_transport.hpp"

class TcpClient {
 public:
  TcpClient();
  ~TcpClient();

  bool connectTo(const std::string& host, int port);
  bool login(const std::string& username, const std::string& password);
  bool ping();
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
