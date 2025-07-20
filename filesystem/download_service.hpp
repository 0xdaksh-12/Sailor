#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "file_service.hpp"
#include "transport/itransport.hpp"

namespace sailor::fs {

struct DownloadFileMetadata {
  uint64_t download_id{0};
  uint64_t file_size{0};
  std::string filename;
  std::string sha256_hash;
  std::filesystem::path full_path;
};

class DownloadService {
 public:
  explicit DownloadService(FileService& file_service);

  bool prepareDownload(const std::string& remote_path,
                       DownloadFileMetadata& out_meta, std::string& out_error);

  bool streamFile(ITransport& transport, const DownloadFileMetadata& meta,
                  std::string& out_error);

 private:
  FileService& file_service_;
};

}  // namespace sailor::fs
