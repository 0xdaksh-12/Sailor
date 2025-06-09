#pragma once

#include <cstdint>
#include <vector>

struct __attribute__((packed)) PacketHeader {
  uint32_t type{0};
  uint64_t payload_size{0};
};

struct Packet {
  PacketHeader header{};
  std::vector<uint8_t> payload;
};
