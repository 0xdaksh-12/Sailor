#include "download_service.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "common/hash.hpp"
#include "common/packet_builder.hpp"
#include "protocol/packet_io.hpp"

namespace stdfs = std::filesystem;
constexpr size_t DOWNLOAD_CHUNK_SIZE = 64 * 1024;  // 64 KB

namespace sailor::fs {

DownloadService::DownloadService(FileService& file_service)
    : file_service_(file_service) {}

bool DownloadService::prepareDownload(const std::string& remote_path,
                                      DownloadFileMetadata& out_meta,
                                      std::string& out_error) {
  if (remote_path.empty()) {
    out_error = "Remote path cannot be empty";
    return false;
  }

  stdfs::path target_path;
  try {
    target_path = file_service_.resolvePath(remote_path);
  } catch (const std::exception& e) {
    out_error = std::string("Invalid path: ") + e.what();
    return false;
  }

  std::error_code ec;
  if (!stdfs::exists(target_path, ec)) {
    out_error = "File not found";
    return false;
  }

  if (stdfs::is_directory(target_path, ec)) {
    out_error = "Target is a directory, not a file";
    return false;
  }

  uint64_t size = stdfs::file_size(target_path, ec);
  if (ec) {
    out_error = "Unable to read file metadata";
    return false;
  }

  std::string file_hash = sailor::common::sha256File(target_path.string());
  if (file_hash.empty() && size > 0) {
    out_error = "Failed to calculate file checksum";
    return false;
  }

  std::mt19937_64 rng(std::random_device{}());
  out_meta.download_id = rng();
  out_meta.file_size = size;
  out_meta.filename = target_path.filename().string();
  out_meta.sha256_hash = file_hash;
  out_meta.full_path = target_path;

  return true;
}

bool DownloadService::streamFile(ITransport& transport,
                                 const DownloadFileMetadata& meta,
                                 std::string& out_error) {
  // Send DOWNLOAD_BEGIN
  Packet begin_pkt = PacketBuilder::downloadBegin(
      meta.download_id, meta.file_size, meta.filename, meta.sha256_hash);
  if (!PacketIO::sendPacket(transport, begin_pkt)) {
    out_error = "Failed to send DOWNLOAD_BEGIN packet";
    return false;
  }

  // Stream 64KB Chunks
  std::ifstream file(meta.full_path, std::ios::binary);
  if (!file.is_open()) {
    out_error = "Failed to open file for streaming";
    return false;
  }

  std::vector<uint8_t> buffer(DOWNLOAD_CHUNK_SIZE);
  uint64_t offset = 0;

  while (file.good() && offset < meta.file_size) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    std::streamsize bytes_read = file.gcount();
    if (bytes_read <= 0) break;

    Packet chunk_pkt =
        PacketBuilder::downloadChunk(meta.download_id, offset, buffer.data(),
                                     static_cast<size_t>(bytes_read));
    if (!PacketIO::sendPacket(transport, chunk_pkt)) {
      out_error =
          "Failed to send DOWNLOAD_CHUNK at offset " + std::to_string(offset);
      return false;
    }

    offset += static_cast<uint64_t>(bytes_read);
  }

  // Send DOWNLOAD_END
  Packet end_pkt = PacketBuilder::downloadEnd(meta.download_id);
  if (!PacketIO::sendPacket(transport, end_pkt)) {
    out_error = "Failed to send DOWNLOAD_END packet";
    return false;
  }

  return true;
}

}  // namespace sailor::fs
