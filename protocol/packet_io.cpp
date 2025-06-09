#include "packet_io.hpp"

#include <endian.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>

#include "packet_serializer.hpp"

bool PacketIO::sendAll(int fd, const uint8_t* data, size_t length) {
  size_t total = 0;
  while (total < length) {
    ssize_t sent = send(fd, data + total, length - total, 0);
    if (sent <= 0) {
      return false;
    }
    total += static_cast<size_t>(sent);
  }
  return true;
}

bool PacketIO::recvAll(int fd, uint8_t* data, size_t length) {
  size_t total = 0;
  while (total < length) {
    ssize_t received = recv(fd, data + total, length - total, 0);
    if (received <= 0) {
      return false;
    }
    total += static_cast<size_t>(received);
  }
  return true;
}

bool PacketIO::sendPacket(int fd, const Packet& packet) {
  auto buffer = PacketSerializer::serialize(packet);
  return sendAll(fd, buffer.data(), buffer.size());
}

bool PacketIO::receivePacket(int fd, Packet& packet) {
  PacketHeader wire_header;
  if (!recvAll(fd, reinterpret_cast<uint8_t*>(&wire_header),
               sizeof(wire_header))) {
    return false;
  }

  packet.header.type = be32toh(wire_header.type);
  packet.header.payload_size = be64toh(wire_header.payload_size);

  packet.payload.resize(packet.header.payload_size);

  if (packet.header.payload_size > 0) {
    if (!recvAll(fd, packet.payload.data(), packet.header.payload_size)) {
      return false;
    }
  }

  return true;
}
