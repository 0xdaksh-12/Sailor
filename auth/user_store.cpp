#include "user_store.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

static std::string resolveDbPath(const std::string& path) {
  if (path.empty()) return path;
  if (fs::exists(path)) return path;
  if (fs::exists("../" + path)) return "../" + path;
  try {
    fs::path exe_dir = fs::canonical("/proc/self/exe").parent_path();
    if (fs::exists(exe_dir / path)) {
      return (exe_dir / path).string();
    }
    if (fs::exists(exe_dir.parent_path() / path)) {
      return (exe_dir.parent_path() / path).string();
    }
  } catch (...) {
    // Fallback to original path
  }
  return path;
}

namespace sailor::auth {

UserStore::UserStore(const std::string& db_path) : db_path_(db_path) {}

bool UserStore::load() {
  std::string resolved = resolveDbPath(db_path_);
  std::ifstream file(resolved, std::ios::binary);
  if (!file.is_open()) {
    return false;  // Fall back to in-memory default
  }

  char magic[16] = {0};
  file.read(magic, 15);
  file.close();

  // If SQLite database binary file
  if (std::string(magic, 15) == "SQLite format 3") {
    std::string cmd = "sqlite3 \"" + resolved +
                      "\" \"SELECT username, password_hash FROM users;\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
      char buffer[512];
      while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
          line.pop_back();
        }
        size_t pipe_pos = line.find('|');
        if (pipe_pos != std::string::npos) {
          std::string u = line.substr(0, pipe_pos);
          std::string h = line.substr(pipe_pos + 1);
          users_[u] = h;
        }
      }
      pclose(pipe);
      return true;
    }
    return false;
  }

  // Plain text fallback format: <username>:<hash>
  std::ifstream text_file(resolved);
  std::string line;
  while (std::getline(text_file, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::istringstream iss(line);
    std::string username, hash;
    if (std::getline(iss, username, ':') && std::getline(iss, hash)) {
      if (!hash.empty() && hash.back() == '\r') {
        hash.pop_back();
      }
      users_[username] = hash;
    }
  }
  return true;
}

bool UserStore::userExists(const std::string& username) const {
  return users_.find(username) != users_.end();
}

std::string UserStore::getPasswordHash(const std::string& username) const {
  auto it = users_.find(username);
  if (it != users_.end()) {
    return it->second;
  }
  return "";
}

void UserStore::addUser(const std::string& username,
                        const std::string& password_hash) {
  users_[username] = password_hash;
}

}  // namespace sailor::auth
