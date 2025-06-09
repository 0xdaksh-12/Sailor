#pragma once

#include "protocol/packet.hpp"
#include "protocol/packet_type.hpp"

class PacketBuilder {
 public:
  static Packet ping() {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::PING);
    packet.header.payload_size = 0;
    return packet;
  }

  static Packet pong() {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::PONG);
    packet.header.payload_size = 0;
    return packet;
  }
};
