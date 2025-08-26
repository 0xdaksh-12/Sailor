#include <cstdlib>
#include <cstring>
#include <memory>

#include "sailor.h"
#include "sailor_client_internal.hpp"
#include "tls/tls_init.hpp"

static std::unique_ptr<sailor::sdk::InternalClient> g_client = nullptr;
static SailorProgressCallback g_progress_cb = nullptr;
static SailorLogCallback g_log_cb = nullptr;

static sailor::sdk::InternalClient& getClient() {
  if (!g_client) {
    initializeTls();
    g_client = std::make_unique<sailor::sdk::InternalClient>();
    if (g_progress_cb) g_client->setProgressCallback(g_progress_cb);
    if (g_log_cb) {
      g_client->setLogCallback([](const std::string& msg) {
        if (g_log_cb) g_log_cb(msg.c_str());
      });
    }
  }
  return *g_client;
}

extern "C" {

void sailor_set_progress_callback(SailorProgressCallback callback) {
  g_progress_cb = callback;
  if (g_client) g_client->setProgressCallback(callback);
}

void sailor_set_log_callback(SailorLogCallback callback) {
  g_log_cb = callback;
  if (g_client) {
    g_client->setLogCallback([](const std::string& msg) {
      if (g_log_cb) g_log_cb(msg.c_str());
    });
  }
}

int32_t sailor_connect(const char* host, uint16_t port, const char* username,
                       const char* password) {
  if (!host || !username || !password) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(
      getClient().connect(host, port, username, password));
}

int32_t sailor_disconnect(void) {
  if (!g_client) return SAILOR_OK;
  return static_cast<int32_t>(g_client->disconnect());
}

int32_t sailor_is_connected(void) {
  return (g_client && g_client->isConnected()) ? 1 : 0;
}

SailorListResult* sailor_list(const char* path) {
  if (!path) return nullptr;

  std::vector<sailor::fs::DirectoryEntry> entries;
  SailorStatus st = getClient().list(path, entries);
  if (st != SAILOR_OK) return nullptr;

  auto* result =
      static_cast<SailorListResult*>(std::malloc(sizeof(SailorListResult)));
  if (!result) return nullptr;

  result->count = entries.size();
  result->entries = nullptr;

  if (result->count > 0) {
    result->entries = static_cast<SailorEntry*>(
        std::malloc(sizeof(SailorEntry) * entries.size()));

    if (!result->entries) {
      std::free(result);
      return nullptr;
    }

    for (size_t i = 0; i < entries.size(); ++i) {
      result->entries[i].name = strdup(entries[i].name.c_str());
      result->entries[i].is_directory = entries[i].is_directory ? 1 : 0;
      result->entries[i].size = entries[i].size;
      result->entries[i].modified_time = entries[i].modified_time;
    }
  }

  return result;
}

void sailor_free_list(SailorListResult* result) {
  if (!result) return;
  if (result->entries) {
    for (uint64_t i = 0; i < result->count; ++i) {
      if (result->entries[i].name) {
        std::free(const_cast<char*>(result->entries[i].name));
      }
    }
    std::free(result->entries);
  }
  std::free(result);
}

int32_t sailor_mkdir(const char* path) {
  if (!path) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(getClient().mkdir(path));
}

int32_t sailor_rmdir(const char* path) {
  if (!path) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(getClient().rmdir(path));
}

int32_t sailor_upload(const char* local_path, const char* remote_dir) {
  if (!local_path || !remote_dir) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(getClient().upload(local_path, remote_dir));
}

int32_t sailor_download(const char* remote_path, const char* local_path) {
  if (!remote_path || !local_path) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(getClient().download(remote_path, local_path));
}

int32_t sailor_delete(const char* remote_path) {
  if (!remote_path) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(getClient().deletePath(remote_path));
}

int32_t sailor_rename(const char* source, const char* destination) {
  if (!source || !destination) return SAILOR_INVALID_PATH;
  return static_cast<int32_t>(getClient().rename(source, destination));
}

}  // extern "C"
