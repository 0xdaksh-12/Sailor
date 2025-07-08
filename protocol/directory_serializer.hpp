#pragma once

#include <vector>

#include "filesystem/directory_entry.hpp"

namespace sailor::protocol {

class DirectorySerializer {
 public:
  static std::vector<uint8_t> serialize(
      const std::vector<fs::DirectoryEntry>& entries);
  static bool deserialize(const std::vector<uint8_t>& data,
                          std::vector<fs::DirectoryEntry>& out_entries);
};

}  // namespace sailor::protocol
