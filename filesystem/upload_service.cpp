#include "upload_service.hpp"

#include <openssl/crypto.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace stdfs = std::filesystem;

namespace sailor::fs {

UploadSession::~UploadSession() {
  if (file_stream.is_open()) {
    file_stream.close();
  }
  if (md_ctx) {
    EVP_MD_CTX_free(md_ctx);
    md_ctx = nullptr;
  }
}

UploadSession::UploadSession(UploadSession&& other) noexcept
    : upload_id(other.upload_id),
      filename(std::move(other.filename)),
      target_path(std::move(other.target_path)),
      expected_size(other.expected_size),
      received_bytes(other.received_bytes),
      expected_sha256(std::move(other.expected_sha256)),
      file_stream(std::move(other.file_stream)),
      md_ctx(other.md_ctx),
      active(other.active) {
  other.md_ctx = nullptr;
  other.active = false;
}

UploadSession& UploadSession::operator=(UploadSession&& other) noexcept {
  if (this != &other) {
    if (file_stream.is_open()) file_stream.close();
    if (md_ctx) EVP_MD_CTX_free(md_ctx);

    upload_id = other.upload_id;
    filename = std::move(other.filename);
    target_path = std::move(other.target_path);
    expected_size = other.expected_size;
    received_bytes = other.received_bytes;
    expected_sha256 = std::move(other.expected_sha256);
    file_stream = std::move(other.file_stream);
    md_ctx = other.md_ctx;
    active = other.active;

    other.md_ctx = nullptr;
    other.active = false;
  }
  return *this;
}

UploadService::UploadService(FileService& file_service)
    : file_service_(file_service) {}

bool UploadService::beginUpload(uint64_t upload_id,
                                const std::string& remote_dir,
                                const std::string& filename, uint64_t file_size,
                                const std::string& expected_sha256,
                                std::string& out_error) {
  if (sessions_.find(upload_id) != sessions_.end()) {
    out_error = "Upload session already exists";
    return false;
  }

  if (filename.empty() || filename.find('/') != std::string::npos ||
      filename.find('\\') != std::string::npos) {
    out_error = "Invalid filename";
    return false;
  }

  stdfs::path dir_path;
  try {
    dir_path = file_service_.resolvePath(remote_dir);
  } catch (const std::exception& e) {
    out_error = std::string("Target path error: ") + e.what();
    return false;
  }

  std::error_code ec;
  if (!stdfs::exists(dir_path, ec)) {
    stdfs::create_directories(dir_path, ec);
  }

  stdfs::path target_file = dir_path / filename;

  UploadSession session;
  session.upload_id = upload_id;
  session.filename = filename;
  session.target_path = target_file;
  session.expected_size = file_size;
  session.received_bytes = 0;
  session.expected_sha256 = expected_sha256;

  session.file_stream.open(target_file, std::ios::binary | std::ios::trunc);
  if (!session.file_stream.is_open()) {
    out_error = "Could not open target file for writing";
    return false;
  }

  session.md_ctx = EVP_MD_CTX_new();
  if (!session.md_ctx ||
      EVP_DigestInit_ex(session.md_ctx, EVP_sha256(), nullptr) != 1) {
    out_error = "Failed to initialize digest context";
    return false;
  }

  session.active = true;
  sessions_[upload_id] = std::move(session);
  return true;
}

bool UploadService::writeChunk(uint64_t upload_id, uint64_t offset,
                               const uint8_t* data, size_t size,
                               std::string& out_error) {
  auto it = sessions_.find(upload_id);
  if (it == sessions_.end() || !it->second.active) {
    out_error = "No active upload session with specified ID";
    return false;
  }

  UploadSession& session = it->second;

  if (offset != session.received_bytes) {
    out_error = "Out-of-order chunk offset received";
    return false;
  }

  if (session.received_bytes + size > session.expected_size) {
    out_error = "Received bytes exceed expected file size";
    return false;
  }

  session.file_stream.write(reinterpret_cast<const char*>(data), size);
  if (!session.file_stream.good()) {
    out_error = "Filesystem write failure";
    return false;
  }

  if (EVP_DigestUpdate(session.md_ctx, data, size) != 1) {
    out_error = "Failed to update digest";
    return false;
  }

  session.received_bytes += size;
  return true;
}

bool UploadService::finishUpload(uint64_t upload_id, std::string& out_error) {
  auto it = sessions_.find(upload_id);
  if (it == sessions_.end() || !it->second.active) {
    out_error = "No active upload session with specified ID";
    return false;
  }

  UploadSession& session = it->second;
  session.file_stream.close();

  if (session.received_bytes != session.expected_size) {
    out_error = "Size mismatch: expected " +
                std::to_string(session.expected_size) + " but received " +
                std::to_string(session.received_bytes);
    abortUpload(upload_id);
    return false;
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length = 0;
  if (EVP_DigestFinal_ex(session.md_ctx, hash, &length) != 1) {
    out_error = "Failed to finalize SHA256 digest";
    abortUpload(upload_id);
    return false;
  }

  std::ostringstream oss;
  for (unsigned int i = 0; i < length; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }
  std::string computed_hash = oss.str();

  if (!session.expected_sha256.empty() &&
      CRYPTO_memcmp(computed_hash.data(), session.expected_sha256.data(),
                    computed_hash.size()) != 0) {
    out_error = "SHA256 checksum mismatch (corrupted upload)";
    abortUpload(upload_id);
    return false;
  }

  sessions_.erase(it);
  return true;
}

void UploadService::abortUpload(uint64_t upload_id) {
  auto it = sessions_.find(upload_id);
  if (it != sessions_.end()) {
    if (it->second.file_stream.is_open()) {
      it->second.file_stream.close();
    }
    std::error_code ec;
    stdfs::remove(it->second.target_path, ec);
    sessions_.erase(it);
  }
}

}  // namespace sailor::fs
