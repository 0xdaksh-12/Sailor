#include "tcp_server.hpp"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "common/connection_state.hpp"
#include "common/packet_builder.hpp"
#include "protocol/packet.hpp"
#include "protocol/packet_io.hpp"
#include "protocol/packet_type.hpp"
#include "transport/tls_transport.hpp"

TcpServer::TcpServer(int port, std::string cert_path, std::string key_path)
    : port_(port), tls_context_(std::move(cert_path), std::move(key_path)) {}

TcpServer::~TcpServer() {
  if (server_fd_ >= 0) {
    close(server_fd_);
  }
}

bool TcpServer::start() {
  if (!tls_context_.initialize()) {
    std::cerr << "Failed to initialize TLS server context" << std::endl;
    return false;
  }

  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    perror("socket");
    return false;
  }

  int opt = 1;
  setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    perror("bind");
    return false;
  }

  if (listen(server_fd_, 10) < 0) {
    perror("listen");
    return false;
  }

  std::cout << "Server listening on port " << port_ << " (TLS enabled)"
            << std::endl;
  return true;
}

void TcpServer::run() {
  while (true) {
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);

    int client_fd =
        accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    ConnectionState state = ConnectionState::TLS_HANDSHAKE;

    SSL* ssl = SSL_new(tls_context_.get());
    if (!ssl) {
      ERR_print_errors_fp(stderr);
      close(client_fd);
      continue;
    }

    SSL_set_fd(ssl, client_fd);

    if (SSL_accept(ssl) <= 0) {
      std::cerr << "TLS handshake failed" << std::endl;
      ERR_print_errors_fp(stderr);
      SSL_free(ssl);
      close(client_fd);
      continue;
    }

    state = ConnectionState::CONNECTED_UNAUTHENTICATED;
    std::cout << "TLS handshake success (Cipher: " << SSL_get_cipher(ssl)
              << ")" << std::endl;

    // TlsTransport manages the lifecycle of ssl
    TlsTransport transport(client_fd, ssl, /*take_ownership=*/true);

    Packet packet;
    if (PacketIO::receivePacket(transport, packet)) {
      auto p_type = static_cast<PacketType>(packet.header.type);

      switch (p_type) {
        case PacketType::PING: {
          std::cout << "Received PING" << std::endl;
          Packet pong = PacketBuilder::pong();
          PacketIO::sendPacket(transport, pong);
          break;
        }
        default:
          std::cout << "Received packet type: "
                    << static_cast<uint32_t>(p_type) << std::endl;
          break;
      }
    }

    state = ConnectionState::DISCONNECTED;
    close(client_fd);
  }
}
