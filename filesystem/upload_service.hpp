#pragma once

#include <openssl/evp.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "file_service.hpp"

namespace sailor::fs {

struct UploadSession {
  uint64_t upload_id{0};
  std::string filename;
  std::filesystem::path target_path;
  uint64_t expected_size{0};
  uint64_t received_bytes{0};
  std::string expected_sha256;
  std::ofstream file_stream;
  EVP_MD_CTX* md_ctx{nullptr};
  bool active{false};

  UploadSession() = default;
  ~UploadSession();

  UploadSession(const UploadSession&) = delete;
  UploadSession& operator=(const UploadSession&) = delete;
  UploadSession(UploadSession&& other) noexcept;
  UploadSession& operator=(UploadSession&& other) noexcept;
};

class UploadService {
 public:
  explicit UploadService(FileService& file_service);
  ~UploadService() = default;

  bool beginUpload(uint64_t upload_id, const std::string& remote_dir,
                   const std::string& filename, uint64_t file_size,
                   const std::string& expected_sha256, std::string& out_error);

  bool writeChunk(uint64_t upload_id, uint64_t offset, const uint8_t* data,
                  size_t size, std::string& out_error);

  bool finishUpload(uint64_t upload_id, std::string& out_error);

  void abortUpload(uint64_t upload_id);

 private:
  FileService& file_service_;
  std::unordered_map<uint64_t, UploadSession> sessions_;
};

}  // namespace sailor::fs
