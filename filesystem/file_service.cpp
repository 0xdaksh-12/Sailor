#include "file_service.hpp"

#include <algorithm>
#include <chrono>
#include <system_error>

namespace stdfs = std::filesystem;

static stdfs::path resolveStorageRoot(const std::string& path) {
  if (stdfs::exists(path)) return stdfs::canonical(path);
  if (stdfs::exists("../" + path)) return stdfs::canonical("../" + path);

  try {
    stdfs::path exe_dir = stdfs::canonical("/proc/self/exe").parent_path();
    if (stdfs::exists(exe_dir / path)) {
      return stdfs::canonical(exe_dir / path);
    }
    if (stdfs::exists(exe_dir.parent_path() / path)) {
      return stdfs::canonical(exe_dir.parent_path() / path);
    }
  } catch (...) {
  }

  // If path does not exist, create it locally
  stdfs::create_directories(path);
  return stdfs::canonical(path);
}

namespace sailor::fs {

FileService::FileService(const std::string& storage_root) {
  root_ = resolveStorageRoot(storage_root);
}

std::filesystem::path FileService::resolvePath(
    const std::string& relative_path) {
  std::string sanitized = relative_path;
  while (!sanitized.empty() &&
         (sanitized.front() == '/' || sanitized.front() == '\\')) {
    sanitized.erase(0, 1);
  }

  stdfs::path target = root_ / sanitized;
  std::error_code ec;
  stdfs::path resolved = stdfs::weakly_canonical(target, ec);

  if (ec) {
    throw std::runtime_error("Invalid path");
  }

  std::string root_str = root_.lexically_normal().string();
  std::string resolved_str = resolved.lexically_normal().string();

  // Ensure resolved path starts strictly with root path
  if (resolved_str.rfind(root_str, 0) != 0) {
    throw std::runtime_error("Path traversal attack detected");
  }

  return resolved;
}

std::vector<DirectoryEntry> FileService::listDirectory(
    const std::string& relative_path) {
  stdfs::path dir_path = resolvePath(relative_path);

  std::error_code ec;
  if (!stdfs::exists(dir_path, ec) || !stdfs::is_directory(dir_path, ec)) {
    throw std::runtime_error("Directory not found");
  }

  std::vector<DirectoryEntry> entries;

  for (const auto& dir_entry : stdfs::directory_iterator(dir_path, ec)) {
    DirectoryEntry item;
    item.name = dir_entry.path().filename().string();
    item.is_directory = dir_entry.is_directory(ec);

    if (item.is_directory) {
      item.size = 0;
    } else {
      item.size = dir_entry.file_size(ec);
      if (ec) item.size = 0;
    }

    auto ftime = dir_entry.last_write_time(ec);
    if (!ec) {
      auto sctp =
          std::chrono::time_point_cast<std::chrono::system_clock::duration>(
              ftime - stdfs::file_time_type::clock::now() +
              std::chrono::system_clock::now());
      item.modified_time = static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::seconds>(
              sctp.time_since_epoch())
              .count());
    } else {
      item.modified_time = 0;
    }

    entries.push_back(std::move(item));
  }

  // Sort: directories first, then alphabetical
  std::sort(entries.begin(), entries.end(),
            [](const DirectoryEntry& a, const DirectoryEntry& b) {
              if (a.is_directory != b.is_directory) {
                return a.is_directory > b.is_directory;
              }
              return a.name < b.name;
            });

  return entries;
}

}  // namespace sailor::fs
