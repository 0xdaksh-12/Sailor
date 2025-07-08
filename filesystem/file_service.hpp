#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "directory_entry.hpp"

namespace sailor::fs {

class FileService {
 public:
  explicit FileService(const std::string& storage_root = "server_storage");

  std::vector<DirectoryEntry> listDirectory(const std::string& relative_path);

  const std::filesystem::path& root() const { return root_; }

 private:
  std::filesystem::path resolvePath(const std::string& relative_path);

  std::filesystem::path root_;
};

}  // namespace sailor::fs
