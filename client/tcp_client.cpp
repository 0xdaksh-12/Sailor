#include "tcp_client.hpp"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "common/packet_builder.hpp"
#include "protocol/packet.hpp"
#include "protocol/packet_io.hpp"
#include "protocol/packet_type.hpp"

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() { disconnect(); }

void TcpClient::disconnect() {
  transport_.reset();
  if (socket_fd_ >= 0) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
  state_ = ConnectionState::DISCONNECTED;
  session_id_ = 0;
}

bool TcpClient::connectTo(const std::string& host, int port) {
  if (!tls_context_.initialize()) {
    std::cerr << "Failed to initialize TLS client context" << std::endl;
    return false;
  }

  state_ = ConnectionState::CONNECTING;

  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    perror("socket");
    disconnect();
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
    perror("inet_pton");
    disconnect();
    return false;
  }

  if (connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
      0) {
    perror("connect");
    disconnect();
    return false;
  }

  state_ = ConnectionState::TLS_HANDSHAKE;

  SSL* ssl = SSL_new(tls_context_.get());
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    disconnect();
    return false;
  }

  SSL_set_fd(ssl, socket_fd_);

  if (SSL_connect(ssl) <= 0) {
    std::cerr << "TLS connection failed" << std::endl;
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    disconnect();
    return false;
  }

  transport_ =
      std::make_unique<TlsTransport>(socket_fd_, ssl, /*take_ownership=*/true);
  state_ = ConnectionState::CONNECTED_UNAUTHENTICATED;

  std::cout << "TLS connected (Cipher: " << SSL_get_cipher(ssl) << ")"
            << std::endl;
  return true;
}

bool TcpClient::login(const std::string& username,
                      const std::string& password) {
  if (!transport_ || state_ == ConnectionState::DISCONNECTED) {
    std::cerr << "Cannot login: not connected" << std::endl;
    return false;
  }

  state_ = ConnectionState::AUTHENTICATING;

  Packet req = PacketBuilder::authRequest(username, password);
  if (!PacketIO::sendPacket(*transport_, req)) {
    std::cerr << "Failed to send AUTH_REQUEST" << std::endl;
    disconnect();
    return false;
  }

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp)) {
    std::cerr << "Failed to receive AUTH_RESPONSE" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(resp.header.type) ==
      PacketType::AUTH_RESPONSE) {
    bool ok = false;
    uint64_t sid = 0;
    std::string msg;

    if (!PacketBuilder::parseAuthResponse(resp, ok, sid, msg)) {
      std::cerr << "Malformed AUTH_RESPONSE" << std::endl;
      return false;
    }

    if (ok) {
      session_id_ = sid;
      state_ = ConnectionState::AUTHENTICATED;
      std::cout << "Login successful. Session ID: " << session_id_ << " ("
                << msg << ")" << std::endl;
      return true;
    } else {
      state_ = ConnectionState::CONNECTED_UNAUTHENTICATED;
      std::cerr << "Login failed: " << msg << std::endl;
      return false;
    }
  } else if (static_cast<PacketType>(resp.header.type) == PacketType::ERROR) {
    std::string err_msg;
    PacketBuilder::parseError(resp, err_msg);
    std::cerr << "Server Error: " << err_msg << std::endl;
    return false;
  }

  std::cerr << "Unexpected packet response: " << resp.header.type << std::endl;
  return false;
}

bool TcpClient::ping() {
  if (!transport_ || state_ == ConnectionState::DISCONNECTED) {
    std::cerr << "Cannot ping: not connected" << std::endl;
    return false;
  }

  Packet ping_pkt = PacketBuilder::ping();
  if (!PacketIO::sendPacket(*transport_, ping_pkt)) {
    std::cerr << "Failed to send PING" << std::endl;
    disconnect();
    return false;
  }

  Packet response;
  if (!PacketIO::receivePacket(*transport_, response)) {
    std::cerr << "Failed to receive response packet" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(response.header.type) == PacketType::PONG) {
    std::cout << "Received PONG" << std::endl;
    return true;
  } else {
    std::cout << "Received unexpected packet type: " << response.header.type
              << std::endl;
    return false;
  }
}

bool TcpClient::list(const std::string& path,
                     std::vector<sailor::fs::DirectoryEntry>& out_entries) {
  if (!transport_ || state_ != ConnectionState::AUTHENTICATED) {
    std::cerr << "Cannot list files: not authenticated" << std::endl;
    return false;
  }

  Packet req = PacketBuilder::listRequest(path);
  if (!PacketIO::sendPacket(*transport_, req)) {
    std::cerr << "Failed to send LIST packet" << std::endl;
    disconnect();
    return false;
  }

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp)) {
    std::cerr << "Failed to receive LIST response" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(resp.header.type) ==
      PacketType::LIST_RESPONSE) {
    if (!PacketBuilder::parseListResponse(resp, out_entries)) {
      std::cerr << "Failed to parse LIST_RESPONSE" << std::endl;
      return false;
    }
    return true;
  } else if (static_cast<PacketType>(resp.header.type) == PacketType::ERROR) {
    std::string err_msg;
    PacketBuilder::parseError(resp, err_msg);
    std::cerr << "Server returned error on LIST: " << err_msg << std::endl;
    return false;
  }

  std::cerr << "Unexpected packet response: " << resp.header.type << std::endl;
  return false;
}
