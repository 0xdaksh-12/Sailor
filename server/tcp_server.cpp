#include "tcp_server.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "common/packet_builder.hpp"
#include "protocol/packet.hpp"
#include "protocol/packet_io.hpp"
#include "protocol/packet_type.hpp"

TcpServer::TcpServer(int port) : port_(port), server_fd_(-1) {}

bool TcpServer::start() {
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

  std::cout << "Server listening on port " << port_ << std::endl;
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

    Packet packet;
    if (PacketIO::receivePacket(client_fd, packet)) {
      auto p_type = static_cast<PacketType>(packet.header.type);
      std::cout << "Received packet type: " << static_cast<uint32_t>(p_type)
                << std::endl;

      switch (p_type) {
        case PacketType::PING: {
          std::cout << "Received packet type: PING" << std::endl;
          Packet pong = PacketBuilder::pong();
          PacketIO::sendPacket(client_fd, pong);
          break;
        }
        default:
          std::cout << "Unknown or unhandled packet type: "
                    << packet.header.type << std::endl;
          break;
      }
    }

    close(client_fd);
  }
}
