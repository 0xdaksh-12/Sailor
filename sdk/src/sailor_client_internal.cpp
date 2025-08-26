#include "sailor_client_internal.hpp"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <random>

#include "common/hash.hpp"
#include "common/packet_builder.hpp"
#include "protocol/packet.hpp"
#include "protocol/packet_io.hpp"
#include "protocol/packet_type.hpp"

namespace stdfs = std::filesystem;
constexpr size_t CHUNK_SIZE = 64 * 1024;

namespace sailor::sdk {

InternalClient::InternalClient() = default;

InternalClient::~InternalClient() { disconnect(); }

void InternalClient::setProgressCallback(ProgressCallback cb) {
  std::lock_guard<std::mutex> lock(mtx_);
  progress_cb_ = std::move(cb);
}

void InternalClient::setLogCallback(LogCallback cb) {
  std::lock_guard<std::mutex> lock(mtx_);
  log_cb_ = std::move(cb);
}

void InternalClient::log(const std::string& msg) {
  if (log_cb_) log_cb_(msg);
}

bool InternalClient::isConnected() const {
  std::lock_guard<std::mutex> lock(mtx_);
  return state_ == ConnectionState::AUTHENTICATED && transport_ != nullptr;
}

SailorStatus InternalClient::disconnect() {
  std::lock_guard<std::mutex> lock(mtx_);
  transport_.reset();
  if (socket_fd_ >= 0) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
  state_ = ConnectionState::DISCONNECTED;
  session_id_ = 0;
  log("Disconnected");
  return SAILOR_OK;
}

SailorStatus InternalClient::connect(const std::string& host, uint16_t port,
                                     const std::string& user,
                                     const std::string& pass) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (!tls_context_.initialize()) {
    log("Failed to initialize TLS context");
    return SAILOR_INTERNAL_ERROR;
  }

  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    log("Socket creation failed");
    return SAILOR_CONNECTION_FAILED;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
    close(socket_fd_);
    socket_fd_ = -1;
    log("Invalid IP address");
    return SAILOR_CONNECTION_FAILED;
  }

  if (::connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
      0) {
    close(socket_fd_);
    socket_fd_ = -1;
    log("TCP connection failed");
    return SAILOR_CONNECTION_FAILED;
  }

  SSL* ssl = SSL_new(tls_context_.get());
  if (!ssl) {
    close(socket_fd_);
    socket_fd_ = -1;
    return SAILOR_INTERNAL_ERROR;
  }
  SSL_set_fd(ssl, socket_fd_);

  if (SSL_connect(ssl) <= 0) {
    SSL_free(ssl);
    close(socket_fd_);
    socket_fd_ = -1;
    log("TLS handshake failed");
    return SAILOR_CONNECTION_FAILED;
  }

  transport_ = std::make_unique<TlsTransport>(socket_fd_, ssl, true);
  state_ = ConnectionState::CONNECTED_UNAUTHENTICATED;
  log("TLS handshake complete");

  // Authentication
  Packet req = PacketBuilder::authRequest(user, pass);
  if (!PacketIO::sendPacket(*transport_, req)) {
    disconnect();
    return SAILOR_CONNECTION_FAILED;
  }

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp)) {
    disconnect();
    return SAILOR_CONNECTION_FAILED;
  }

  bool ok = false;
  uint64_t sid = 0;
  std::string msg;
  if (PacketBuilder::parseAuthResponse(resp, ok, sid, msg) && ok) {
    session_id_ = sid;
    state_ = ConnectionState::AUTHENTICATED;
    log("Authenticated successfully as " + user);
    return SAILOR_OK;
  }

  log("Authentication failed: " + msg);
  return SAILOR_AUTH_FAILED;
}

SailorStatus InternalClient::list(
    const std::string& path,
    std::vector<sailor::fs::DirectoryEntry>& out_entries) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_) {
    return SAILOR_NOT_CONNECTED;
  }

  Packet req = PacketBuilder::listRequest(path);
  if (!PacketIO::sendPacket(*transport_, req)) return SAILOR_CONNECTION_FAILED;

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp))
    return SAILOR_CONNECTION_FAILED;

  if (static_cast<PacketType>(resp.header.type) == PacketType::LIST_RESPONSE) {
    if (PacketBuilder::parseListResponse(resp, out_entries)) return SAILOR_OK;
    return SAILOR_INTERNAL_ERROR;
  }

  return SAILOR_OPERATION_FAILED;
}

SailorStatus InternalClient::upload(const std::string& local_file,
                                    const std::string& remote_dir) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_) {
    return SAILOR_NOT_CONNECTED;
  }

  std::error_code ec;
  if (!stdfs::exists(local_file, ec) || stdfs::is_directory(local_file, ec)) {
    return SAILOR_FILE_NOT_FOUND;
  }

  uint64_t file_size = stdfs::file_size(local_file, ec);
  std::string filename = stdfs::path(local_file).filename().string();
  std::string sha256_hash = sailor::common::sha256File(local_file);
  if (sha256_hash.empty() && file_size > 0) return SAILOR_INTERNAL_ERROR;

  std::mt19937_64 rng(std::random_device{}());
  uint64_t upload_id = rng();

  Packet begin_pkt = PacketBuilder::uploadBegin(
      upload_id, file_size, remote_dir, filename, sha256_hash);
  if (!PacketIO::sendPacket(*transport_, begin_pkt))
    return SAILOR_CONNECTION_FAILED;

  Packet begin_resp;
  if (!PacketIO::receivePacket(*transport_, begin_resp) ||
      static_cast<PacketType>(begin_resp.header.type) != PacketType::SUCCESS) {
    return SAILOR_OPERATION_FAILED;
  }

  std::ifstream file(local_file, std::ios::binary);
  if (!file.is_open()) return SAILOR_FILE_NOT_FOUND;

  std::vector<uint8_t> buffer(CHUNK_SIZE);
  uint64_t total_sent = 0;

  while (file.good() && total_sent < file_size) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    std::streamsize bytes_read = file.gcount();
    if (bytes_read <= 0) break;

    Packet chunk_pkt = PacketBuilder::uploadChunk(upload_id, total_sent,
                                                  buffer.data(), bytes_read);
    if (!PacketIO::sendPacket(*transport_, chunk_pkt))
      return SAILOR_CONNECTION_FAILED;

    total_sent += static_cast<uint64_t>(bytes_read);
    if (progress_cb_) progress_cb_(total_sent, file_size);
  }

  Packet end_pkt = PacketBuilder::uploadEnd(upload_id);
  if (!PacketIO::sendPacket(*transport_, end_pkt))
    return SAILOR_CONNECTION_FAILED;

  Packet end_resp;
  if (!PacketIO::receivePacket(*transport_, end_resp) ||
      static_cast<PacketType>(end_resp.header.type) != PacketType::SUCCESS) {
    return SAILOR_INTERNAL_ERROR;
  }

  return SAILOR_OK;
}

SailorStatus InternalClient::download(const std::string& remote_file,
                                      const std::string& local_dest) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_) {
    return SAILOR_NOT_CONNECTED;
  }

  Packet req = PacketBuilder::downloadRequest(remote_file);
  if (!PacketIO::sendPacket(*transport_, req)) return SAILOR_CONNECTION_FAILED;

  Packet begin_pkt;
  if (!PacketIO::receivePacket(*transport_, begin_pkt) ||
      static_cast<PacketType>(begin_pkt.header.type) !=
          PacketType::DOWNLOAD_BEGIN) {
    return SAILOR_FILE_NOT_FOUND;
  }

  uint64_t download_id = 0, file_size = 0;
  std::string filename, expected_sha256;
  if (!PacketBuilder::parseDownloadBegin(begin_pkt, download_id, file_size,
                                         filename, expected_sha256)) {
    return SAILOR_INTERNAL_ERROR;
  }

  stdfs::path target_path(local_dest);
  std::error_code ec;
  if (stdfs::is_directory(target_path, ec) || local_dest.back() == '/' ||
      local_dest.back() == '\\') {
    stdfs::create_directories(target_path, ec);
    target_path /= filename;
  } else if (target_path.has_parent_path()) {
    stdfs::create_directories(target_path.parent_path(), ec);
  }

  std::ofstream outfile(target_path, std::ios::binary | std::ios::trunc);
  if (!outfile.is_open()) return SAILOR_PERMISSION_DENIED;

  uint64_t received_bytes = 0;
  bool ok = false;

  while (true) {
    Packet chunk_pkt;
    if (!PacketIO::receivePacket(*transport_, chunk_pkt)) break;

    auto ptype = static_cast<PacketType>(chunk_pkt.header.type);
    if (ptype == PacketType::DOWNLOAD_CHUNK) {
      uint64_t chunk_id = 0, offset = 0;
      const uint8_t* chunk_data = nullptr;
      size_t chunk_size = 0;

      if (!PacketBuilder::parseDownloadChunk(chunk_pkt, chunk_id, offset,
                                             chunk_data, chunk_size) ||
          chunk_id != download_id || offset != received_bytes) {
        break;
      }

      if (chunk_size > 0) {
        outfile.write(reinterpret_cast<const char*>(chunk_data), chunk_size);
        received_bytes += chunk_size;
        if (progress_cb_) progress_cb_(received_bytes, file_size);
      }
    } else if (ptype == PacketType::DOWNLOAD_END) {
      ok = (received_bytes == file_size);
      break;
    } else {
      break;
    }
  }

  outfile.close();
  if (!ok) {
    stdfs::remove(target_path, ec);
    return SAILOR_OPERATION_FAILED;
  }

  std::string actual_hash = sailor::common::sha256File(target_path.string());
  if (actual_hash != expected_sha256) {
    stdfs::remove(target_path, ec);
    return SAILOR_INTERNAL_ERROR;
  }

  return SAILOR_OK;
}

SailorStatus InternalClient::deletePath(const std::string& remote_path) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_)
    return SAILOR_NOT_CONNECTED;

  Packet req = PacketBuilder::deleteRequest(remote_path);
  if (!PacketIO::sendPacket(*transport_, req)) return SAILOR_CONNECTION_FAILED;

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp))
    return SAILOR_CONNECTION_FAILED;

  bool ok = false;
  std::string msg;
  if (PacketBuilder::parseDeleteResponse(resp, ok, msg) && ok) return SAILOR_OK;
  return SAILOR_OPERATION_FAILED;
}

SailorStatus InternalClient::rename(const std::string& source,
                                    const std::string& destination) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_)
    return SAILOR_NOT_CONNECTED;

  Packet req = PacketBuilder::renameRequest(source, destination);
  if (!PacketIO::sendPacket(*transport_, req)) return SAILOR_CONNECTION_FAILED;

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp))
    return SAILOR_CONNECTION_FAILED;

  bool ok = false;
  std::string msg;
  if (PacketBuilder::parseRenameResponse(resp, ok, msg) && ok) return SAILOR_OK;
  return SAILOR_OPERATION_FAILED;
}

SailorStatus InternalClient::mkdir(const std::string& path) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_)
    return SAILOR_NOT_CONNECTED;

  Packet req = PacketBuilder::mkdirRequest(path);
  if (!PacketIO::sendPacket(*transport_, req)) return SAILOR_CONNECTION_FAILED;

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp))
    return SAILOR_CONNECTION_FAILED;

  bool ok = false;
  std::string msg;
  if (PacketBuilder::parseMkdirResponse(resp, ok, msg) && ok) return SAILOR_OK;
  return SAILOR_OPERATION_FAILED;
}

SailorStatus InternalClient::rmdir(const std::string& path) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (state_ != ConnectionState::AUTHENTICATED || !transport_)
    return SAILOR_NOT_CONNECTED;

  Packet req = PacketBuilder::rmdirRequest(path);
  if (!PacketIO::sendPacket(*transport_, req)) return SAILOR_CONNECTION_FAILED;

  Packet resp;
  if (!PacketIO::receivePacket(*transport_, resp))
    return SAILOR_CONNECTION_FAILED;

  bool ok = false;
  std::string msg;
  if (PacketBuilder::parseRmdirResponse(resp, ok, msg) && ok) return SAILOR_OK;
  return SAILOR_OPERATION_FAILED;
}

}  // namespace sailor::sdk
