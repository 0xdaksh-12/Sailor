#pragma once

#include <endian.h>

#include <cstring>
#include <string>
#include <vector>

#include "filesystem/directory_entry.hpp"
#include "protocol/directory_serializer.hpp"
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

  // AUTH_REQUEST wire format:
  // [uint16_t ulen][username bytes][uint16_t plen][password bytes]
  static Packet authRequest(const std::string& username,
                            const std::string& password) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::AUTH_REQUEST);

    uint16_t ulen = htobe16(static_cast<uint16_t>(username.size()));
    uint16_t plen = htobe16(static_cast<uint16_t>(password.size()));

    size_t total_size =
        sizeof(ulen) + username.size() + sizeof(plen) + password.size();
    packet.payload.resize(total_size);

    size_t offset = 0;
    std::memcpy(packet.payload.data() + offset, &ulen, sizeof(ulen));
    offset += sizeof(ulen);

    std::memcpy(packet.payload.data() + offset, username.data(),
                username.size());
    offset += username.size();

    std::memcpy(packet.payload.data() + offset, &plen, sizeof(plen));
    offset += sizeof(plen);

    std::memcpy(packet.payload.data() + offset, password.data(),
                password.size());

    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // AUTH_RESPONSE wire format:
  // [uint8_t success (1 or 0)][uint64_t session_id][uint16_t msg_len][msg bytes]
  static Packet authResponse(bool success, uint64_t session_id,
                             const std::string& message) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::AUTH_RESPONSE);

    uint8_t status = success ? 1 : 0;
    uint64_t be_session_id = htobe64(session_id);
    uint16_t msg_len = htobe16(static_cast<uint16_t>(message.size()));

    size_t total_size = sizeof(status) + sizeof(be_session_id) +
                        sizeof(msg_len) + message.size();
    packet.payload.resize(total_size);

    size_t offset = 0;
    packet.payload[offset++] = status;

    std::memcpy(packet.payload.data() + offset, &be_session_id,
                sizeof(be_session_id));
    offset += sizeof(be_session_id);

    std::memcpy(packet.payload.data() + offset, &msg_len, sizeof(msg_len));
    offset += sizeof(msg_len);

    if (!message.empty()) {
      std::memcpy(packet.payload.data() + offset, message.data(),
                  message.size());
    }

    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // ERROR packet: [uint16_t msg_len][msg bytes]
  static Packet error(const std::string& message) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::ERROR);

    uint16_t msg_len = htobe16(static_cast<uint16_t>(message.size()));
    packet.payload.resize(sizeof(msg_len) + message.size());

    std::memcpy(packet.payload.data(), &msg_len, sizeof(msg_len));
    if (!message.empty()) {
      std::memcpy(packet.payload.data() + sizeof(msg_len), message.data(),
                  message.size());
    }

    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // LIST Request: [uint16_t path_len][path bytes]
  static Packet listRequest(const std::string& path) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::LIST);

    uint16_t path_len = htobe16(static_cast<uint16_t>(path.size()));
    packet.payload.resize(sizeof(path_len) + path.size());

    std::memcpy(packet.payload.data(), &path_len, sizeof(path_len));
    if (!path.empty()) {
      std::memcpy(packet.payload.data() + sizeof(path_len), path.data(),
                  path.size());
    }

    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // LIST Response: serialized directory entries
  static Packet listResponse(
      const std::vector<sailor::fs::DirectoryEntry>& entries) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::LIST_RESPONSE);
    packet.payload =
        sailor::protocol::DirectorySerializer::serialize(entries);
    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // Parsers
  static bool parseAuthRequest(const Packet& packet, std::string& out_user,
                               std::string& out_pass) {
    if (packet.payload.size() < sizeof(uint16_t) * 2) return false;

    size_t offset = 0;
    uint16_t ulen = 0;
    std::memcpy(&ulen, packet.payload.data() + offset, sizeof(ulen));
    ulen = be16toh(ulen);
    offset += sizeof(ulen);

    if (packet.payload.size() < offset + ulen + sizeof(uint16_t)) return false;
    out_user.assign(
        reinterpret_cast<const char*>(packet.payload.data() + offset), ulen);
    offset += ulen;

    uint16_t plen = 0;
    std::memcpy(&plen, packet.payload.data() + offset, sizeof(plen));
    plen = be16toh(plen);
    offset += sizeof(plen);

    if (packet.payload.size() < offset + plen) return false;
    out_pass.assign(
        reinterpret_cast<const char*>(packet.payload.data() + offset), plen);

    return true;
  }

  static bool parseAuthResponse(const Packet& packet, bool& out_success,
                                uint64_t& out_session_id,
                                std::string& out_msg) {
    if (packet.payload.size() <
        sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint16_t))
      return false;

    size_t offset = 0;
    out_success = (packet.payload[offset++] == 1);

    uint64_t be_sid = 0;
    std::memcpy(&be_sid, packet.payload.data() + offset, sizeof(be_sid));
    out_session_id = be64toh(be_sid);
    offset += sizeof(be_sid);

    uint16_t mlen = 0;
    std::memcpy(&mlen, packet.payload.data() + offset, sizeof(mlen));
    mlen = be16toh(mlen);
    offset += sizeof(mlen);

    if (packet.payload.size() < offset + mlen) return false;
    out_msg.assign(
        reinterpret_cast<const char*>(packet.payload.data() + offset), mlen);

    return true;
  }

  static bool parseError(const Packet& packet, std::string& out_msg) {
    if (packet.payload.size() < sizeof(uint16_t)) return false;

    uint16_t mlen = 0;
    std::memcpy(&mlen, packet.payload.data(), sizeof(mlen));
    mlen = be16toh(mlen);

    if (packet.payload.size() < sizeof(uint16_t) + mlen) return false;
    out_msg.assign(
        reinterpret_cast<const char*>(packet.payload.data() + sizeof(uint16_t)),
        mlen);
    return true;
  }

  static bool parseListRequest(const Packet& packet, std::string& out_path) {
    if (packet.payload.size() < sizeof(uint16_t)) return false;

    uint16_t plen = 0;
    std::memcpy(&plen, packet.payload.data(), sizeof(plen));
    plen = be16toh(plen);

    if (packet.payload.size() < sizeof(uint16_t) + plen) return false;
    out_path.assign(
        reinterpret_cast<const char*>(packet.payload.data() + sizeof(uint16_t)),
        plen);
    return true;
  }

  static bool parseListResponse(
      const Packet& packet,
      std::vector<sailor::fs::DirectoryEntry>& out_entries) {
    return sailor::protocol::DirectorySerializer::deserialize(packet.payload,
                                                              out_entries);
  }
};
