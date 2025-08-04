#include "rename_service.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>

namespace stdfs = std::filesystem;

namespace sailor::fs {

RenameService::RenameService(FileService& file_service,
                             const std::string& audit_log_path)
    : file_service_(file_service), audit_log_path_(audit_log_path) {
  stdfs::path log_path(audit_log_path_);
  if (log_path.has_parent_path()) {
    std::error_code ec;
    stdfs::create_directories(log_path.parent_path(), ec);
  }
}

void RenameService::logAudit(const std::string& username,
                             const std::string& source,
                             const std::string& destination,
                             const std::string& status,
                             const std::string& detail) {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf{};
  gmtime_r(&now_c, &tm_buf);

  char time_str[32];
  std::strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);

  std::ofstream log_file(audit_log_path_, std::ios::app);
  if (log_file.is_open()) {
    log_file << time_str << " | " << username << " | RENAME | " << source
             << " -> " << destination << " | " << status;
    if (!detail.empty()) {
      log_file << " (" << detail << ")";
    }
    log_file << "\n";
  }
}

bool RenameService::renamePath(const std::string& source,
                               const std::string& destination,
                               const std::string& username,
                               std::string& out_message) {
  if (source.empty() || destination.empty()) {
    out_message = "Source and destination paths cannot be empty";
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  if (source == "/" || source == "." || destination == "/" ||
      destination == ".") {
    out_message = "Cannot rename or overwrite the storage root directory";
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  stdfs::path src_path;
  stdfs::path dst_path;
  try {
    src_path = file_service_.resolvePath(source);
    dst_path = file_service_.resolvePath(destination);
  } catch (const std::exception& e) {
    out_message = std::string("Invalid path: ") + e.what();
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  if (src_path == file_service_.root() || dst_path == file_service_.root()) {
    out_message = "Cannot rename or overwrite the storage root directory";
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  std::error_code ec;
  if (!stdfs::exists(src_path, ec)) {
    out_message = "Source file or directory not found";
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  if (stdfs::exists(dst_path, ec)) {
    out_message = "Destination already exists";
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  // Verify parent directory of destination exists
  if (dst_path.has_parent_path() &&
      !stdfs::exists(dst_path.parent_path(), ec)) {
    out_message = "Destination parent directory does not exist";
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  // Guard against moving a directory inside itself
  std::string src_canonical = src_path.lexically_normal().string();
  std::string dst_canonical = dst_path.lexically_normal().string();
  if (stdfs::is_directory(src_path, ec)) {
    if (dst_canonical.rfind(src_canonical + "/", 0) == 0) {
      out_message =
          "Cannot move a directory into itself or its subdirectory";
      logAudit(username, source, destination, "FAILED", out_message);
      return false;
    }
  }

  stdfs::rename(src_path, dst_path, ec);
  if (ec) {
    out_message = ec.message();
    logAudit(username, source, destination, "FAILED", out_message);
    return false;
  }

  out_message = "Renamed successfully";
  logAudit(username, source, destination, "SUCCESS", "");
  return true;
}

}  // namespace sailor::fs
