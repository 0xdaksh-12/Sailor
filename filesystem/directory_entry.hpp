#pragma once

#include <cstdint>
#include <string>

namespace sailor::fs {

struct DirectoryEntry {
  std::string name;
  bool is_directory{false};
  uint64_t size{0};
  uint64_t modified_time{0};
};

// C-ABI-safe representation prepared for future Flutter FFI bindings
extern "C" {
struct FfiDirectoryEntry {
  const char* name;
  bool is_directory;
  uint64_t size;
  uint64_t modified_time;
};
}

}  // namespace sailor::fs
