#include "tcp_client.hpp"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

#include "common/hash.hpp"
#include "common/packet_builder.hpp"
#include "protocol/packet.hpp"
#include "protocol/packet_io.hpp"
#include "protocol/packet_type.hpp"

namespace stdfs = std::filesystem;

constexpr size_t UPLOAD_CHUNK_SIZE = 64 * 1024;  // 64 KB

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() { disconnect(); }

void TcpClient::disconnect() {
  transport_.reset();
  if (socket_fd_ >= 0) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
  state_ = ConnectionState::DISCONNECTED;
  session_id_ = 0;
}

bool TcpClient::connectTo(const std::string& host, int port) {
  if (!tls_context_.initialize()) {
    std::cerr << "Failed to initialize TLS client context" << std::endl;
    return false;
  }

  state_ = ConnectionState::CONNECTING;

  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    perror("socket");
    disconnect();
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
    perror("inet_pton");
    disconnect();
    return false;
  }

  if (connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
      0) {
    perror("connect");
    disconnect();
    return false;
  }

  state_ = ConnectionState::TLS_HANDSHAKE;

  SSL* ssl = SSL_new(tls_context_.get());
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    disconnect();
    return false;
  }

  SSL_set_fd(ssl, socket_fd_);

  if (SSL_connect(ssl) <= 0) {
    std::cerr << "TLS connection failed" << std::endl;
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    disconnect();
    return false;
  }

  transport_ =
      std::make_unique<TlsTransport>(socket_fd_, ssl, /*take_ownership=*/true);
  state_ = ConnectionState::CONNECTED_UNAUTHENTICATED;

  std::cout << "TLS connected (Cipher: " << SSL_get_cipher(ssl) << ")"
            << std::endl;
  return true;
}

bool TcpClient::login(const std::string& username,
                      const std::string& password) {
  if (!transport_ || state_ == ConnectionState::DISCONNECTED) {
    std::cerr << "Cannot login: not connected" << std::endl;
    return false;
  }

  state_ = ConnectionState::AUTHENTICATING;

  Packet req = PacketBuilder::authRequest(username, password);
  if (!PacketIO::sendPacket(*transport_, req)) {
    std::cerr << "Failed to send AUTH_REQUEST" << std::endl;
    disconnect();
    return false;
  }

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp)) {
    std::cerr << "Failed to receive AUTH_RESPONSE" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(resp.header.type) == PacketType::AUTH_RESPONSE) {
    bool ok = false;
    uint64_t sid = 0;
    std::string msg;

    if (!PacketBuilder::parseAuthResponse(resp, ok, sid, msg)) {
      std::cerr << "Malformed AUTH_RESPONSE" << std::endl;
      return false;
    }

    if (ok) {
      session_id_ = sid;
      state_ = ConnectionState::AUTHENTICATED;
      std::cout << "Login successful. Session ID: " << session_id_ << " ("
                << msg << ")" << std::endl;
      return true;
    } else {
      state_ = ConnectionState::CONNECTED_UNAUTHENTICATED;
      std::cerr << "Login failed: " << msg << std::endl;
      return false;
    }
  } else if (static_cast<PacketType>(resp.header.type) == PacketType::ERROR) {
    std::string err_msg;
    PacketBuilder::parseError(resp, err_msg);
    std::cerr << "Server Error: " << err_msg << std::endl;
    return false;
  }

  std::cerr << "Unexpected packet response: " << resp.header.type << std::endl;
  return false;
}

bool TcpClient::ping() {
  if (!transport_ || state_ == ConnectionState::DISCONNECTED) {
    std::cerr << "Cannot ping: not connected" << std::endl;
    return false;
  }

  Packet ping_pkt = PacketBuilder::ping();
  if (!PacketIO::sendPacket(*transport_, ping_pkt)) {
    std::cerr << "Failed to send PING" << std::endl;
    disconnect();
    return false;
  }

  Packet response;
  if (!PacketIO::receivePacket(*transport_, response)) {
    std::cerr << "Failed to receive response packet" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(response.header.type) == PacketType::PONG) {
    std::cout << "Received PONG" << std::endl;
    return true;
  } else {
    std::cout << "Received unexpected packet type: " << response.header.type
              << std::endl;
    return false;
  }
}

bool TcpClient::list(const std::string& path,
                     std::vector<sailor::fs::DirectoryEntry>& out_entries) {
  if (!transport_ || state_ != ConnectionState::AUTHENTICATED) {
    std::cerr << "Cannot list files: not authenticated" << std::endl;
    return false;
  }

  Packet req = PacketBuilder::listRequest(path);
  if (!PacketIO::sendPacket(*transport_, req)) {
    std::cerr << "Failed to send LIST packet" << std::endl;
    disconnect();
    return false;
  }

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp)) {
    std::cerr << "Failed to receive LIST response" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(resp.header.type) == PacketType::LIST_RESPONSE) {
    if (!PacketBuilder::parseListResponse(resp, out_entries)) {
      std::cerr << "Failed to parse LIST_RESPONSE" << std::endl;
      return false;
    }
    return true;
  } else if (static_cast<PacketType>(resp.header.type) == PacketType::ERROR) {
    std::string err_msg;
    PacketBuilder::parseError(resp, err_msg);
    std::cerr << "Server returned error on LIST: " << err_msg << std::endl;
    return false;
  }

  std::cerr << "Unexpected packet response: " << resp.header.type << std::endl;
  return false;
}

bool TcpClient::upload(const std::string& local_file_path,
                       const std::string& remote_dir,
                       ProgressCallback progress_cb) {
  if (!transport_ || state_ != ConnectionState::AUTHENTICATED) {
    std::cerr << "Cannot upload file: not authenticated" << std::endl;
    return false;
  }

  std::error_code ec;
  if (!stdfs::exists(local_file_path, ec) ||
      stdfs::is_directory(local_file_path, ec)) {
    std::cerr << "Local file does not exist or is a directory: "
              << local_file_path << std::endl;
    return false;
  }

  uint64_t file_size = stdfs::file_size(local_file_path, ec);
  std::string filename = stdfs::path(local_file_path).filename().string();

  std::cout << "Calculating SHA256 of " << filename << "..." << std::endl;
  std::string sha256_hash = sailor::common::sha256File(local_file_path);
  if (sha256_hash.empty()) {
    std::cerr << "Failed to calculate file SHA256" << std::endl;
    return false;
  }

  std::mt19937_64 rng(std::random_device{}());
  uint64_t upload_id = rng();

  // 1. Send UPLOAD_BEGIN
  Packet begin_pkt = PacketBuilder::uploadBegin(
      upload_id, file_size, remote_dir, filename, sha256_hash);
  if (!PacketIO::sendPacket(*transport_, begin_pkt)) {
    std::cerr << "Failed to send UPLOAD_BEGIN" << std::endl;
    disconnect();
    return false;
  }

  Packet begin_resp;
  if (!PacketIO::receivePacket(*transport_, begin_resp)) {
    std::cerr << "Failed to receive response to UPLOAD_BEGIN" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(begin_resp.header.type) != PacketType::SUCCESS) {
    std::string err;
    PacketBuilder::parseError(begin_resp, err);
    std::cerr << "Server rejected upload: " << err << std::endl;
    return false;
  }

  // 2. Stream UPLOAD_CHUNKs
  std::ifstream file(local_file_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open local file for reading" << std::endl;
    return false;
  }

  std::vector<uint8_t> buffer(UPLOAD_CHUNK_SIZE);
  uint64_t total_sent = 0;

  while (file.good() && total_sent < file_size) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    std::streamsize bytes_read = file.gcount();
    if (bytes_read <= 0) break;

    Packet chunk_pkt = PacketBuilder::uploadChunk(upload_id, total_sent,
                                                  buffer.data(), bytes_read);
    if (!PacketIO::sendPacket(*transport_, chunk_pkt)) {
      std::cerr << "Failed to send chunk at offset " << total_sent << std::endl;
      disconnect();
      return false;
    }

    total_sent += static_cast<uint64_t>(bytes_read);
    if (progress_cb) {
      progress_cb(total_sent, file_size);
    }
  }

  // 3. Send UPLOAD_END
  Packet end_pkt = PacketBuilder::uploadEnd(upload_id);
  if (!PacketIO::sendPacket(*transport_, end_pkt)) {
    std::cerr << "Failed to send UPLOAD_END" << std::endl;
    disconnect();
    return false;
  }

  Packet end_resp;
  if (!PacketIO::receivePacket(*transport_, end_resp)) {
    std::cerr << "Failed to receive UPLOAD_END verification response"
              << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(end_resp.header.type) == PacketType::SUCCESS) {
    std::string msg;
    PacketBuilder::parseSuccess(end_resp, msg);
    std::cout << "\nUpload completed successfully: " << msg << std::endl;
    return true;
  } else {
    std::string err;
    PacketBuilder::parseError(end_resp, err);
    std::cerr << "\nUpload failed during verification: " << err << std::endl;
    return false;
  }
}

bool TcpClient::download(const std::string& remote_file_path,
                         const std::string& local_dest_path,
                         ProgressCallback progress_cb) {
  if (!transport_ || state_ != ConnectionState::AUTHENTICATED) {
    std::cerr << "Cannot download file: not authenticated" << std::endl;
    return false;
  }

  if (remote_file_path.empty()) {
    std::cerr << "Remote file path cannot be empty" << std::endl;
    return false;
  }

  // 1. Send DOWNLOAD_REQUEST
  Packet req = PacketBuilder::downloadRequest(remote_file_path);
  if (!PacketIO::sendPacket(*transport_, req)) {
    std::cerr << "Failed to send DOWNLOAD_REQUEST" << std::endl;
    disconnect();
    return false;
  }

  // 2. Receive DOWNLOAD_BEGIN or ERROR
  Packet begin_pkt;
  if (!PacketIO::receivePacket(*transport_, begin_pkt)) {
    std::cerr << "Failed to receive response to DOWNLOAD_REQUEST" << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(begin_pkt.header.type) == PacketType::ERROR) {
    std::string err;
    PacketBuilder::parseError(begin_pkt, err);
    std::cerr << "Download rejected by server: " << err << std::endl;
    return false;
  }

  if (static_cast<PacketType>(begin_pkt.header.type) !=
      PacketType::DOWNLOAD_BEGIN) {
    std::cerr << "Unexpected packet received: " << begin_pkt.header.type
              << std::endl;
    return false;
  }

  uint64_t download_id = 0, file_size = 0;
  std::string filename, expected_sha256;
  if (!PacketBuilder::parseDownloadBegin(begin_pkt, download_id, file_size,
                                         filename, expected_sha256)) {
    std::cerr << "Malformed DOWNLOAD_BEGIN packet" << std::endl;
    return false;
  }

  // Resolve local destination path
  stdfs::path target_path(local_dest_path);
  std::error_code ec;
  if (stdfs::is_directory(target_path, ec) ||
      (!local_dest_path.empty() && (local_dest_path.back() == '/' ||
                                    local_dest_path.back() == '\\'))) {
    if (!stdfs::exists(target_path, ec)) {
      stdfs::create_directories(target_path, ec);
    }
    target_path /= filename;
  } else if (target_path.has_parent_path() &&
             !stdfs::exists(target_path.parent_path(), ec)) {
    stdfs::create_directories(target_path.parent_path(), ec);
  }

  std::cout << "Downloading '" << filename << "' (" << file_size
            << " bytes) -> " << target_path.string() << std::endl;

  std::ofstream outfile(target_path, std::ios::binary | std::ios::trunc);
  if (!outfile.is_open()) {
    std::cerr << "Failed to open local destination file for writing: "
              << target_path << std::endl;
    return false;
  }

  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if (!mdctx || EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
    if (mdctx) EVP_MD_CTX_free(mdctx);
    outfile.close();
    stdfs::remove(target_path, ec);
    std::cerr << "Failed to initialize hash context" << std::endl;
    return false;
  }

  uint64_t received_bytes = 0;
  bool download_complete = false;

  while (true) {
    Packet chunk_pkt;
    if (!PacketIO::receivePacket(*transport_, chunk_pkt)) {
      std::cerr << "Connection lost while receiving download stream"
                << std::endl;
      disconnect();
      break;
    }

    auto ptype = static_cast<PacketType>(chunk_pkt.header.type);

    if (ptype == PacketType::DOWNLOAD_CHUNK) {
      uint64_t chunk_id = 0, offset = 0;
      const uint8_t* chunk_data = nullptr;
      size_t chunk_size = 0;

      if (!PacketBuilder::parseDownloadChunk(chunk_pkt, chunk_id, offset,
                                             chunk_data, chunk_size)) {
        std::cerr << "Malformed DOWNLOAD_CHUNK packet" << std::endl;
        break;
      }

      if (chunk_id != download_id) {
        std::cerr << "Mismatched download ID: expected " << download_id
                  << ", got " << chunk_id << std::endl;
        break;
      }

      if (offset != received_bytes) {
        std::cerr << "Chunk offset mismatch: expected " << received_bytes
                  << ", got " << offset << std::endl;
        break;
      }

      if (chunk_size > 0) {
        outfile.write(reinterpret_cast<const char*>(chunk_data), chunk_size);
        if (!outfile.good()) {
          std::cerr << "Disk write error at offset " << offset << std::endl;
          break;
        }

        EVP_DigestUpdate(mdctx, chunk_data, chunk_size);
        received_bytes += chunk_size;

        if (progress_cb) {
          progress_cb(received_bytes, file_size);
        }
      }
    } else if (ptype == PacketType::DOWNLOAD_END) {
      uint64_t end_id = 0;
      if (!PacketBuilder::parseDownloadEnd(chunk_pkt, end_id) ||
          end_id != download_id) {
        std::cerr << "Invalid DOWNLOAD_END packet" << std::endl;
        break;
      }
      download_complete = true;
      break;
    } else if (ptype == PacketType::ERROR) {
      std::string err_msg;
      PacketBuilder::parseError(chunk_pkt, err_msg);
      std::cerr << "\nServer aborted download: " << err_msg << std::endl;
      break;
    } else {
      std::cerr << "\nUnexpected packet type during download: "
                << static_cast<uint32_t>(ptype) << std::endl;
      break;
    }
  }

  outfile.flush();
  outfile.close();

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;
  EVP_DigestFinal_ex(mdctx, hash, &hash_len);
  EVP_MD_CTX_free(mdctx);

  if (!download_complete) {
    stdfs::remove(target_path, ec);
    std::cerr << "\nDownload failed before completion." << std::endl;
    return false;
  }

  if (received_bytes != file_size) {
    stdfs::remove(target_path, ec);
    std::cerr << "\nSize mismatch: received " << received_bytes
              << " bytes, expected " << file_size << " bytes." << std::endl;
    return false;
  }

  std::ostringstream oss;
  for (unsigned int i = 0; i < hash_len; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }
  std::string calculated_hash = oss.str();

  if (calculated_hash != expected_sha256) {
    stdfs::remove(target_path, ec);
    std::cerr << "\nChecksum verification FAILED!"
              << "\n  Expected: " << expected_sha256
              << "\n  Got:      " << calculated_hash << std::endl;
    return false;
  }

  std::cout << "\nDownload verified and completed successfully."
            << "\n  SHA256: " << calculated_hash << std::endl;
  return true;
}

bool TcpClient::deleteFile(const std::string& remote_path,
                           std::string& out_message) {
  if (!transport_ || state_ != ConnectionState::AUTHENTICATED) {
    out_message = "Cannot delete: not authenticated";
    std::cerr << out_message << std::endl;
    return false;
  }

  if (remote_path.empty()) {
    out_message = "Remote path cannot be empty";
    std::cerr << out_message << std::endl;
    return false;
  }

  Packet req = PacketBuilder::deleteRequest(remote_path);
  if (!PacketIO::sendPacket(*transport_, req)) {
    out_message = "Failed to send DELETE_REQUEST";
    std::cerr << out_message << std::endl;
    disconnect();
    return false;
  }

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp)) {
    out_message = "Failed to receive DELETE_RESPONSE";
    std::cerr << out_message << std::endl;
    disconnect();
    return false;
  }

  if (static_cast<PacketType>(resp.header.type) ==
      PacketType::DELETE_RESPONSE) {
    bool success = false;
    if (!PacketBuilder::parseDeleteResponse(resp, success, out_message)) {
      out_message = "Malformed DELETE_RESPONSE packet";
      std::cerr << out_message << std::endl;
      return false;
    }
    return success;
  } else if (static_cast<PacketType>(resp.header.type) == PacketType::ERROR) {
    PacketBuilder::parseError(resp, out_message);
    return false;
  }

  out_message =
      "Unexpected packet response: " + std::to_string(resp.header.type);
  std::cerr << out_message << std::endl;
  return false;
}
