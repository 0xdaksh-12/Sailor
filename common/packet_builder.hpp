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

  static Packet success(const std::string& message = "") {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::SUCCESS);

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

  static Packet listResponse(
      const std::vector<sailor::fs::DirectoryEntry>& entries) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::LIST_RESPONSE);
    packet.payload =
        sailor::protocol::DirectorySerializer::serialize(entries);
    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // UPLOAD_BEGIN:
  // [uint64_t upload_id][uint64_t file_size][uint16_t path_len][path][uint16_t fn_len][filename][uint16_t hash_len][sha256]
  static Packet uploadBegin(uint64_t upload_id, uint64_t file_size,
                            const std::string& remote_path,
                            const std::string& filename,
                            const std::string& sha256_hash) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::UPLOAD_BEGIN);

    uint64_t be_id = htobe64(upload_id);
    uint64_t be_sz = htobe64(file_size);
    uint16_t p_len = htobe16(static_cast<uint16_t>(remote_path.size()));
    uint16_t f_len = htobe16(static_cast<uint16_t>(filename.size()));
    uint16_t h_len = htobe16(static_cast<uint16_t>(sha256_hash.size()));

    size_t total = sizeof(be_id) + sizeof(be_sz) + sizeof(p_len) +
                   remote_path.size() + sizeof(f_len) + filename.size() +
                   sizeof(h_len) + sha256_hash.size();
    packet.payload.resize(total);

    size_t offset = 0;
    std::memcpy(packet.payload.data() + offset, &be_id, sizeof(be_id));
    offset += sizeof(be_id);
    std::memcpy(packet.payload.data() + offset, &be_sz, sizeof(be_sz));
    offset += sizeof(be_sz);
    std::memcpy(packet.payload.data() + offset, &p_len, sizeof(p_len));
    offset += sizeof(p_len);
    std::memcpy(packet.payload.data() + offset, remote_path.data(),
                remote_path.size());
    offset += remote_path.size();
    std::memcpy(packet.payload.data() + offset, &f_len, sizeof(f_len));
    offset += sizeof(f_len);
    std::memcpy(packet.payload.data() + offset, filename.data(),
                filename.size());
    offset += filename.size();
    std::memcpy(packet.payload.data() + offset, &h_len, sizeof(h_len));
    offset += sizeof(h_len);
    std::memcpy(packet.payload.data() + offset, sha256_hash.data(),
                sha256_hash.size());

    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // UPLOAD_CHUNK:
  // [uint64_t upload_id][uint64_t offset][uint32_t chunk_len][data]
  static Packet uploadChunk(uint64_t upload_id, uint64_t offset,
                            const uint8_t* data, size_t size) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::UPLOAD_CHUNK);

    uint64_t be_id = htobe64(upload_id);
    uint64_t be_off = htobe64(offset);
    uint32_t be_len = htobe32(static_cast<uint32_t>(size));

    size_t total = sizeof(be_id) + sizeof(be_off) + sizeof(be_len) + size;
    packet.payload.resize(total);

    size_t pos = 0;
    std::memcpy(packet.payload.data() + pos, &be_id, sizeof(be_id));
    pos += sizeof(be_id);
    std::memcpy(packet.payload.data() + pos, &be_off, sizeof(be_off));
    pos += sizeof(be_off);
    std::memcpy(packet.payload.data() + pos, &be_len, sizeof(be_len));
    pos += sizeof(be_len);
    std::memcpy(packet.payload.data() + pos, data, size);

    packet.header.payload_size = packet.payload.size();
    return packet;
  }

  // UPLOAD_END:
  // [uint64_t upload_id]
  static Packet uploadEnd(uint64_t upload_id) {
    Packet packet;
    packet.header.type = static_cast<uint32_t>(PacketType::UPLOAD_END);

    uint64_t be_id = htobe64(upload_id);
    packet.payload.resize(sizeof(be_id));
    std::memcpy(packet.payload.data(), &be_id, sizeof(be_id));

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

  static bool parseSuccess(const Packet& packet, std::string& out_msg) {
    return parseError(packet, out_msg);
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

  static bool parseUploadBegin(const Packet& packet, uint64_t& out_id,
                               uint64_t& out_size, std::string& out_path,
                               std::string& out_filename,
                               std::string& out_hash) {
    if (packet.payload.size() < sizeof(uint64_t) * 2 + sizeof(uint16_t) * 3)
      return false;

    size_t pos = 0;
    uint64_t be_id = 0;
    std::memcpy(&be_id, packet.payload.data() + pos, sizeof(be_id));
    pos += sizeof(be_id);
    out_id = be64toh(be_id);

    uint64_t be_sz = 0;
    std::memcpy(&be_sz, packet.payload.data() + pos, sizeof(be_sz));
    pos += sizeof(be_sz);
    out_size = be64toh(be_sz);

    uint16_t p_len = 0;
    std::memcpy(&p_len, packet.payload.data() + pos, sizeof(p_len));
    pos += sizeof(p_len);
    p_len = be16toh(p_len);
    if (packet.payload.size() < pos + p_len + sizeof(uint16_t)) return false;
    out_path.assign(
        reinterpret_cast<const char*>(packet.payload.data() + pos), p_len);
    pos += p_len;

    uint16_t f_len = 0;
    std::memcpy(&f_len, packet.payload.data() + pos, sizeof(f_len));
    pos += sizeof(f_len);
    f_len = be16toh(f_len);
    if (packet.payload.size() < pos + f_len + sizeof(uint16_t)) return false;
    out_filename.assign(
        reinterpret_cast<const char*>(packet.payload.data() + pos), f_len);
    pos += f_len;

    uint16_t h_len = 0;
    std::memcpy(&h_len, packet.payload.data() + pos, sizeof(h_len));
    pos += sizeof(h_len);
    h_len = be16toh(h_len);
    if (packet.payload.size() < pos + h_len) return false;
    out_hash.assign(
        reinterpret_cast<const char*>(packet.payload.data() + pos), h_len);

    return true;
  }

  static bool parseUploadChunk(const Packet& packet, uint64_t& out_id,
                               uint64_t& out_offset, const uint8_t*& out_data,
                               size_t& out_size) {
    if (packet.payload.size() < sizeof(uint64_t) * 2 + sizeof(uint32_t))
      return false;

    size_t pos = 0;
    uint64_t be_id = 0;
    std::memcpy(&be_id, packet.payload.data() + pos, sizeof(be_id));
    pos += sizeof(be_id);
    out_id = be64toh(be_id);

    uint64_t be_off = 0;
    std::memcpy(&be_off, packet.payload.data() + pos, sizeof(be_off));
    pos += sizeof(be_off);
    out_offset = be64toh(be_off);

    uint32_t be_len = 0;
    std::memcpy(&be_len, packet.payload.data() + pos, sizeof(be_len));
    pos += sizeof(be_len);
    out_size = be32toh(be_len);

    if (packet.payload.size() < pos + out_size) return false;
    out_data = packet.payload.data() + pos;
    return true;
  }

  static bool parseUploadEnd(const Packet& packet, uint64_t& out_id) {
    if (packet.payload.size() < sizeof(uint64_t)) return false;
    uint64_t be_id = 0;
    std::memcpy(&be_id, packet.payload.data(), sizeof(be_id));
    out_id = be64toh(be_id);
    return true;
  }
};
