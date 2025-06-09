#pragma once

#include <vector>

#include "packet.hpp"

class PacketSerializer {
 public:
  static std::vector<uint8_t> serialize(const Packet& packet);
  static bool deserialize(const std::vector<uint8_t>& data, Packet& out_packet);
};
