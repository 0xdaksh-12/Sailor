#include "directory_service.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>

namespace stdfs = std::filesystem;

namespace sailor::fs {

DirectoryService::DirectoryService(FileService& file_service,
                                   const std::string& audit_log_path)
    : file_service_(file_service), audit_log_path_(audit_log_path) {
  stdfs::path log_path(audit_log_path_);
  if (log_path.has_parent_path()) {
    std::error_code ec;
    stdfs::create_directories(log_path.parent_path(), ec);
  }
}

void DirectoryService::logAudit(const std::string& username,
                                const std::string& action,
                                const std::string& path,
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
    log_file << time_str << " | " << username << " | " << action << " | "
             << path << " | " << status;
    if (!detail.empty()) {
      log_file << " (" << detail << ")";
    }
    log_file << "\n";
  }
}

bool DirectoryService::createDirectory(const std::string& path,
                                       const std::string& username,
                                       std::string& out_message) {
  if (path.empty() || path == "/" || path == ".") {
    out_message = "Invalid directory path";
    logAudit(username, "MKDIR", path, "FAILED", out_message);
    return false;
  }

  stdfs::path resolved;
  try {
    resolved = file_service_.resolvePath(path);
  } catch (const std::exception& e) {
    out_message = std::string("Invalid path: ") + e.what();
    logAudit(username, "MKDIR", path, "FAILED", out_message);
    return false;
  }

  if (resolved == file_service_.root()) {
    out_message = "Cannot recreate root directory";
    logAudit(username, "MKDIR", path, "FAILED", out_message);
    return false;
  }

  std::error_code ec;
  if (stdfs::exists(resolved, ec)) {
    out_message = "Directory already exists";
    logAudit(username, "MKDIR", path, "FAILED", out_message);
    return false;
  }

  if (!stdfs::create_directories(resolved, ec) && ec) {
    out_message = ec.message();
    logAudit(username, "MKDIR", path, "FAILED", out_message);
    return false;
  }

  out_message = "Directory created";
  logAudit(username, "MKDIR", path, "SUCCESS", "");
  return true;
}

bool DirectoryService::removeDirectory(const std::string& path,
                                       const std::string& username,
                                       std::string& out_message) {
  if (path.empty() || path == "/" || path == ".") {
    out_message = "Cannot delete the storage root directory";
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  stdfs::path resolved;
  try {
    resolved = file_service_.resolvePath(path);
  } catch (const std::exception& e) {
    out_message = std::string("Invalid path: ") + e.what();
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  if (resolved == file_service_.root()) {
    out_message = "Cannot delete the storage root directory";
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  std::error_code ec;
  if (!stdfs::exists(resolved, ec)) {
    out_message = "Directory not found";
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  if (!stdfs::is_directory(resolved, ec)) {
    out_message = "Target is not a directory";
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  auto iter = stdfs::directory_iterator(resolved, ec);
  if (!ec && iter != stdfs::directory_iterator{}) {
    out_message = "Directory not empty";
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  if (!stdfs::remove(resolved, ec)) {
    out_message = ec ? ec.message() : "Unable to remove directory";
    logAudit(username, "RMDIR", path, "FAILED", out_message);
    return false;
  }

  out_message = "Directory removed";
  logAudit(username, "RMDIR", path, "SUCCESS", "");
  return true;
}

}  // namespace sailor::fs
