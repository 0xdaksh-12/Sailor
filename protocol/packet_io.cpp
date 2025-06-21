#include "packet_io.hpp"

#include <endian.h>

#include <cstdint>

#include "packet_serializer.hpp"

bool PacketIO::sendPacket(ITransport& transport, const Packet& packet) {
  auto buffer = PacketSerializer::serialize(packet);
  return transport.sendAll(buffer.data(), buffer.size());
}

bool PacketIO::receivePacket(ITransport& transport, Packet& packet) {
  PacketHeader wire_header;
  if (!transport.recvAll(reinterpret_cast<uint8_t*>(&wire_header),
                         sizeof(wire_header))) {
    return false;
  }

  packet.header.type = be32toh(wire_header.type);
  packet.header.payload_size = be64toh(wire_header.payload_size);

  packet.payload.resize(packet.header.payload_size);

  if (packet.header.payload_size > 0) {
    if (!transport.recvAll(packet.payload.data(),
                           packet.header.payload_size)) {
      return false;
    }
  }

  return true;
}
