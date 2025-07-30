#include "delete_service.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>

namespace stdfs = std::filesystem;

namespace sailor::fs {

DeleteService::DeleteService(FileService& file_service,
                             const std::string& audit_log_path)
    : file_service_(file_service), audit_log_path_(audit_log_path) {
  stdfs::path log_path(audit_log_path_);
  if (log_path.has_parent_path()) {
    std::error_code ec;
    stdfs::create_directories(log_path.parent_path(), ec);
  }
}

void DeleteService::logAudit(const std::string& username,
                             const std::string& path, const std::string& status,
                             const std::string& detail) {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf{};
  gmtime_r(&now_c, &tm_buf);

  char time_str[32];
  std::strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);

  std::ofstream log_file(audit_log_path_, std::ios::app);
  if (log_file.is_open()) {
    log_file << time_str << " | " << username << " | DELETE | " << path << " | "
             << status;
    if (!detail.empty()) {
      log_file << " (" << detail << ")";
    }
    log_file << "\n";
  }
}

bool DeleteService::deletePath(const std::string& remote_path,
                               const std::string& username,
                               std::string& out_message) {
  if (remote_path.empty() || remote_path == "/" || remote_path == ".") {
    out_message = "Cannot delete the storage root directory";
    logAudit(username, remote_path, "FAILED", out_message);
    return false;
  }

  stdfs::path resolved;
  try {
    resolved = file_service_.resolvePath(remote_path);
  } catch (const std::exception& e) {
    out_message = std::string("Invalid path: ") + e.what();
    logAudit(username, remote_path, "FAILED", out_message);
    return false;
  }

  // Explicitly protect root path identity
  if (resolved == file_service_.root()) {
    out_message = "Cannot delete the storage root directory";
    logAudit(username, remote_path, "FAILED", out_message);
    return false;
  }

  std::error_code ec;
  if (!stdfs::exists(resolved, ec)) {
    out_message = "File or directory not found";
    logAudit(username, remote_path, "FAILED", out_message);
    return false;
  }

  // Option A: Non-empty directories are prohibited
  if (stdfs::is_directory(resolved, ec)) {
    auto iter = stdfs::directory_iterator(resolved, ec);
    if (!ec && iter != stdfs::directory_iterator{}) {
      out_message = "Directory not empty";
      logAudit(username, remote_path, "FAILED", out_message);
      return false;
    }
  }

  if (!stdfs::remove(resolved, ec)) {
    out_message = ec ? ec.message() : "Permission denied or unable to delete";
    logAudit(username, remote_path, "FAILED", out_message);
    return false;
  }

  out_message = "Deleted successfully";
  logAudit(username, remote_path, "SUCCESS", "");
  return true;
}

}  // namespace sailor::fs
