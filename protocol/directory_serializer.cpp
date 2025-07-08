#include "directory_serializer.hpp"

#include <endian.h>

#include <cstring>

namespace sailor::protocol {

std::vector<uint8_t> DirectorySerializer::serialize(
    const std::vector<fs::DirectoryEntry>& entries) {
  std::vector<uint8_t> buffer;

  uint32_t count = htobe32(static_cast<uint32_t>(entries.size()));
  const auto* count_ptr = reinterpret_cast<const uint8_t*>(&count);
  buffer.insert(buffer.end(), count_ptr, count_ptr + sizeof(count));

  for (const auto& entry : entries) {
    uint16_t name_len = htobe16(static_cast<uint16_t>(entry.name.size()));
    const auto* nlen_ptr = reinterpret_cast<const uint8_t*>(&name_len);
    buffer.insert(buffer.end(), nlen_ptr, nlen_ptr + sizeof(name_len));

    buffer.insert(buffer.end(), entry.name.begin(), entry.name.end());

    buffer.push_back(entry.is_directory ? 1 : 0);

    uint64_t size_be = htobe64(entry.size);
    const auto* size_ptr = reinterpret_cast<const uint8_t*>(&size_be);
    buffer.insert(buffer.end(), size_ptr, size_ptr + sizeof(size_be));

    uint64_t mtime_be = htobe64(entry.modified_time);
    const auto* mtime_ptr = reinterpret_cast<const uint8_t*>(&mtime_be);
    buffer.insert(buffer.end(), mtime_ptr, mtime_ptr + sizeof(mtime_be));
  }

  return buffer;
}

bool DirectorySerializer::deserialize(
    const std::vector<uint8_t>& data,
    std::vector<fs::DirectoryEntry>& out_entries) {
  out_entries.clear();
  if (data.size() < sizeof(uint32_t)) return false;

  size_t offset = 0;
  uint32_t count = 0;
  std::memcpy(&count, data.data() + offset, sizeof(count));
  count = be32toh(count);
  offset += sizeof(count);

  out_entries.reserve(count);

  for (uint32_t i = 0; i < count; ++i) {
    if (data.size() < offset + sizeof(uint16_t)) return false;

    uint16_t name_len = 0;
    std::memcpy(&name_len, data.data() + offset, sizeof(name_len));
    name_len = be16toh(name_len);
    offset += sizeof(name_len);

    if (data.size() < offset + name_len + 1 + sizeof(uint64_t) * 2) return false;

    fs::DirectoryEntry entry;
    entry.name.assign(
        reinterpret_cast<const char*>(data.data() + offset), name_len);
    offset += name_len;

    entry.is_directory = (data[offset++] == 1);

    uint64_t size_be = 0;
    std::memcpy(&size_be, data.data() + offset, sizeof(size_be));
    entry.size = be64toh(size_be);
    offset += sizeof(size_be);

    uint64_t mtime_be = 0;
    std::memcpy(&mtime_be, data.data() + offset, sizeof(mtime_be));
    entry.modified_time = be64toh(mtime_be);
    offset += sizeof(mtime_be);

    out_entries.push_back(std::move(entry));
  }

  return true;
}

}  // namespace sailor::protocol
