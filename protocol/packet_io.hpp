#pragma once

#include "packet.hpp"

class PacketIO {
 public:
  static bool sendPacket(int socket_fd, const Packet& packet);
  static bool receivePacket(int socket_fd, Packet& packet);

 private:
  static bool sendAll(int fd, const uint8_t* data, size_t length);
  static bool recvAll(int fd, uint8_t* data, size_t length);
};
