#pragma once

#include "packet.hpp"
#include "transport/itransport.hpp"

class PacketIO {
 public:
  static bool sendPacket(ITransport& transport, const Packet& packet);
  static bool receivePacket(ITransport& transport, Packet& packet);
};
