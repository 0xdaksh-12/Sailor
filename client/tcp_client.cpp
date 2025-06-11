#include "tcp_client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "common/packet_builder.hpp"
#include "protocol/packet.hpp"
#include "protocol/packet_io.hpp"
#include "protocol/packet_type.hpp"

bool TcpClient::connectTo(const std::string& host, int port) {
  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    perror("socket");
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
    perror("inet_pton");
    close(socket_fd_);
    return false;
  }

  if (connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
      0) {
    perror("connect");
    close(socket_fd_);
    return false;
  }

  return true;
}

bool TcpClient::ping() {
  Packet ping_pkt = PacketBuilder::ping();
  if (!PacketIO::sendPacket(socket_fd_, ping_pkt)) {
    std::cerr << "Failed to send PING packet" << std::endl;
    close(socket_fd_);
    return false;
  }

  Packet response;
  if (!PacketIO::receivePacket(socket_fd_, response)) {
    std::cerr << "Failed to receive response packet" << std::endl;
    close(socket_fd_);
    return false;
  }

  if (static_cast<PacketType>(response.header.type) == PacketType::PONG) {
    std::cout << "PONG" << std::endl;
  } else {
    std::cout << "Received unexpected packet type: " << response.header.type
              << std::endl;
  }

  close(socket_fd_);
  return true;
}
