#pragma once

#include <string>

#include "file_service.hpp"

namespace sailor::fs {

class RenameService {
 public:
  explicit RenameService(FileService& file_service,
                         const std::string& audit_log_path = "logs/audit.log");

  bool renamePath(const std::string& source, const std::string& destination,
                  const std::string& username, std::string& out_message);

 private:
  void logAudit(const std::string& username, const std::string& source,
                const std::string& destination, const std::string& status,
                const std::string& detail);

  FileService& file_service_;
  std::string audit_log_path_;
};

}  // namespace sailor::fs
