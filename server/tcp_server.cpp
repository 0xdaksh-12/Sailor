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
#include "session.hpp"
#include "transport/tls_transport.hpp"

TcpServer::TcpServer(int port, std::string cert_path, std::string key_path,
                     std::string user_db_path, std::string storage_root)
    : port_(port),
      tls_context_(std::move(cert_path), std::move(key_path)),
      auth_manager_(std::move(user_db_path)),
      file_service_(std::move(storage_root)),
      upload_service_(file_service_),
      rng_(std::random_device{}()) {}

TcpServer::~TcpServer() {
  if (server_fd_ >= 0) {
    close(server_fd_);
  }
}

uint64_t TcpServer::generateSessionId() { return rng_(); }

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

  std::cout << "Server listening on port " << port_
            << " (TLS, Auth, Upload Enabled, Storage: " << file_service_.root()
            << ")" << std::endl;
  return true;
}

void TcpServer::handleClient(int client_fd) {
  SSL* ssl = SSL_new(tls_context_.get());
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    close(client_fd);
    return;
  }

  SSL_set_fd(ssl, client_fd);

  if (SSL_accept(ssl) <= 0) {
    std::cerr << "TLS handshake failed" << std::endl;
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(client_fd);
    return;
  }

  std::cout << "TLS connected (Cipher: " << SSL_get_cipher(ssl) << ")"
            << std::endl;

  TlsTransport transport(client_fd, ssl, /*take_ownership=*/true);
  sailor::server::Session session{};
  ConnectionState state = ConnectionState::CONNECTED_UNAUTHENTICATED;

  Packet packet;
  while (PacketIO::receivePacket(transport, packet)) {
    auto p_type = static_cast<PacketType>(packet.header.type);

    switch (p_type) {
      case PacketType::PING: {
        std::cout << "Received PING" << std::endl;
        Packet pong = PacketBuilder::pong();
        PacketIO::sendPacket(transport, pong);
        break;
      }

      case PacketType::AUTH_REQUEST: {
        std::string username, password;
        if (!PacketBuilder::parseAuthRequest(packet, username, password)) {
          Packet err =
              PacketBuilder::error("Malformed AUTH_REQUEST packet");
          PacketIO::sendPacket(transport, err);
          break;
        }

        std::cout << "Authenticating user: " << username << std::endl;
        bool ok = auth_manager_.authenticate(username, password);

        if (ok) {
          session.id = generateSessionId();
          session.username = username;
          session.authenticated = true;
          state = ConnectionState::AUTHENTICATED;

          std::cout << "User '" << username
                    << "' authenticated successfully (Session: " << session.id
                    << ")" << std::endl;

          Packet resp = PacketBuilder::authResponse(
              true, session.id, "Authentication successful");
          PacketIO::sendPacket(transport, resp);
        } else {
          std::cout << "Authentication failed for user: " << username
                    << std::endl;
          Packet resp =
              PacketBuilder::authResponse(false, 0, "Invalid credentials");
          PacketIO::sendPacket(transport, resp);
        }
        break;
      }

      case PacketType::LIST: {
        if (!session.authenticated) {
          std::cerr << "[AUTH GUARD] Unauthorized LIST attempt" << std::endl;
          Packet err = PacketBuilder::error("Authentication required");
          PacketIO::sendPacket(transport, err);
          break;
        }

        std::string req_path;
        if (!PacketBuilder::parseListRequest(packet, req_path)) {
          Packet err = PacketBuilder::error("Malformed LIST packet");
          PacketIO::sendPacket(transport, err);
          break;
        }

        try {
          auto entries = file_service_.listDirectory(req_path);
          std::cout << "[LIST]\n  User: " << session.username
                    << "\n  Path: " << (req_path.empty() ? "/" : req_path)
                    << "\n  Entries: " << entries.size() << std::endl;

          Packet resp = PacketBuilder::listResponse(entries);
          PacketIO::sendPacket(transport, resp);
        } catch (const std::exception& e) {
          std::cerr << "[LIST ERROR] " << e.what() << std::endl;
          Packet err = PacketBuilder::error(e.what());
          PacketIO::sendPacket(transport, err);
        }
        break;
      }

      case PacketType::UPLOAD_BEGIN: {
        if (!session.authenticated) {
          Packet err = PacketBuilder::error("Authentication required");
          PacketIO::sendPacket(transport, err);
          break;
        }

        uint64_t up_id = 0, f_size = 0;
        std::string r_path, filename, sha256_hash;
        if (!PacketBuilder::parseUploadBegin(packet, up_id, f_size, r_path,
                                             filename, sha256_hash)) {
          Packet err =
              PacketBuilder::error("Malformed UPLOAD_BEGIN packet");
          PacketIO::sendPacket(transport, err);
          break;
        }

        std::cout << "[UPLOAD_BEGIN]\n  User: " << session.username
                  << "\n  File: " << filename
                  << "\n  Size: " << f_size << " bytes"
                  << "\n  SHA256: " << sha256_hash << std::endl;

        std::string err;
        if (!upload_service_.beginUpload(up_id, r_path, filename, f_size,
                                         sha256_hash, err)) {
          std::cerr << "[UPLOAD ERROR] " << err << std::endl;
          Packet resp = PacketBuilder::error(err);
          PacketIO::sendPacket(transport, resp);
        } else {
          Packet resp = PacketBuilder::success("Upload session ready");
          PacketIO::sendPacket(transport, resp);
        }
        break;
      }

      case PacketType::UPLOAD_CHUNK: {
        if (!session.authenticated) {
          Packet err = PacketBuilder::error("Authentication required");
          PacketIO::sendPacket(transport, err);
          break;
        }

        uint64_t up_id = 0, offset = 0;
        const uint8_t* chunk_data = nullptr;
        size_t chunk_size = 0;

        if (!PacketBuilder::parseUploadChunk(packet, up_id, offset, chunk_data,
                                             chunk_size)) {
          Packet err =
              PacketBuilder::error("Malformed UPLOAD_CHUNK packet");
          PacketIO::sendPacket(transport, err);
          break;
        }

        std::string err;
        if (!upload_service_.writeChunk(up_id, offset, chunk_data, chunk_size,
                                        err)) {
          std::cerr << "[UPLOAD CHUNK ERROR] " << err << std::endl;
          Packet resp = PacketBuilder::error(err);
          PacketIO::sendPacket(transport, resp);
        }
        break;
      }

      case PacketType::UPLOAD_END: {
        if (!session.authenticated) {
          Packet err = PacketBuilder::error("Authentication required");
          PacketIO::sendPacket(transport, err);
          break;
        }

        uint64_t up_id = 0;
        if (!PacketBuilder::parseUploadEnd(packet, up_id)) {
          Packet err = PacketBuilder::error("Malformed UPLOAD_END packet");
          PacketIO::sendPacket(transport, err);
          break;
        }

        std::string err;
        if (!upload_service_.finishUpload(up_id, err)) {
          std::cerr << "[UPLOAD FINISH ERROR] " << err << std::endl;
          Packet resp = PacketBuilder::error(err);
          PacketIO::sendPacket(transport, resp);
        } else {
          std::cout << "[UPLOAD]\n  User: " << session.username
                    << "\n  Status: SUCCESS (Checksum verified)" << std::endl;
          Packet resp =
              PacketBuilder::success("Upload verified and completed");
          PacketIO::sendPacket(transport, resp);
        }
        break;
      }

      case PacketType::DOWNLOAD_REQUEST:
      case PacketType::DELETE_FILE:
      case PacketType::RENAME_FILE:
      case PacketType::MKDIR: {
        if (!session.authenticated) {
          std::cerr << "Unauthorized request for packet type: "
                    << static_cast<uint32_t>(p_type) << std::endl;
          Packet err = PacketBuilder::error("Authentication required");
          PacketIO::sendPacket(transport, err);
          break;
        }
        break;
      }

      default:
        std::cout << "Unhandled packet type: "
                  << static_cast<uint32_t>(p_type) << std::endl;
        break;
    }
  }

  std::cout << "Client disconnected"
            << (session.authenticated ? (" (" + session.username + ")") : "")
            << std::endl;
  close(client_fd);
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

    handleClient(client_fd);
  }
}
