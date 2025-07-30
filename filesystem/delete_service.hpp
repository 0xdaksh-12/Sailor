#pragma once

#include <string>

#include "file_service.hpp"

namespace sailor::fs {

class DeleteService {
 public:
  explicit DeleteService(FileService& file_service,
                         const std::string& audit_log_path = "logs/audit.log");

  bool deletePath(const std::string& remote_path, const std::string& username,
                  std::string& out_message);

 private:
  void logAudit(const std::string& username, const std::string& path,
                const std::string& status, const std::string& detail);

  FileService& file_service_;
  std::string audit_log_path_;
};

}  // namespace sailor::fs
