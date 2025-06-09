#include "packet_serializer.hpp"

#include <endian.h>

#include <cstring>

std::vector<uint8_t> PacketSerializer::serialize(const Packet& packet) {
  constexpr size_t header_len = sizeof(PacketHeader);
  std::vector<uint8_t> buffer(header_len + packet.payload.size());

  // Convert to network byte order (Big-Endian)
  PacketHeader wire_header;
  wire_header.type = htobe32(packet.header.type);
  wire_header.payload_size = htobe64(packet.header.payload_size);

  std::memcpy(buffer.data(), &wire_header, header_len);

  if (!packet.payload.empty()) {
    std::memcpy(buffer.data() + header_len, packet.payload.data(),
                packet.payload.size());
  }

  return buffer;
}

bool PacketSerializer::deserialize(const std::vector<uint8_t>& data,
                                   Packet& out_packet) {
  constexpr size_t header_len = sizeof(PacketHeader);
  if (data.size() < header_len) {
    return false;
  }

  PacketHeader wire_header;
  std::memcpy(&wire_header, data.data(), header_len);

  out_packet.header.type = be32toh(wire_header.type);
  out_packet.header.payload_size = be64toh(wire_header.payload_size);

  if (data.size() < header_len + out_packet.header.payload_size) {
    return false;
  }

  out_packet.payload.assign(
      data.begin() + header_len,
      data.begin() + header_len + out_packet.header.payload_size);

  return true;
}
