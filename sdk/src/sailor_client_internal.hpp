#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/connection_state.hpp"
#include "filesystem/directory_entry.hpp"
#include "sailor.h"
#include "tls/tls_client.hpp"
#include "transport/tls_transport.hpp"

namespace sailor::sdk {

class InternalClient {
 public:
  using ProgressCallback = std::function<void(uint64_t, uint64_t)>;
  using LogCallback = std::function<void(const std::string&)>;

  InternalClient();
  ~InternalClient();

  void setProgressCallback(ProgressCallback cb);
  void setLogCallback(LogCallback cb);

  SailorStatus connect(const std::string& host, uint16_t port,
                       const std::string& user, const std::string& pass);
  SailorStatus disconnect();
  bool isConnected() const;

  SailorStatus list(const std::string& path,
                    std::vector<sailor::fs::DirectoryEntry>& out_entries);
  SailorStatus upload(const std::string& local_file,
                      const std::string& remote_dir);
  SailorStatus download(const std::string& remote_file,
                        const std::string& local_dest);
  SailorStatus deletePath(const std::string& remote_path);
  SailorStatus rename(const std::string& source,
                      const std::string& destination);
  SailorStatus mkdir(const std::string& path);
  SailorStatus rmdir(const std::string& path);

 private:
  void log(const std::string& msg);

  int socket_fd_{-1};
  uint64_t session_id_{0};
  ConnectionState state_{ConnectionState::DISCONNECTED};
  TlsClientContext tls_context_;
  std::unique_ptr<TlsTransport> transport_{nullptr};

  ProgressCallback progress_cb_{nullptr};
  LogCallback log_cb_{nullptr};
  mutable std::mutex mtx_;
};

}  // namespace sailor::sdk
