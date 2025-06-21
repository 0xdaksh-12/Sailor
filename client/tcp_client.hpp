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
  bool ping();
  void disconnect();

 private:
  int socket_fd_{-1};
  ConnectionState state_{ConnectionState::DISCONNECTED};
  TlsClientContext tls_context_;
  std::unique_ptr<TlsTransport> transport_{nullptr};
};
