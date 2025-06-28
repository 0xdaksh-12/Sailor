#pragma once

#include <string>
#include <unordered_map>

namespace sailor::auth {

class UserStore {
 public:
  explicit UserStore(const std::string& db_path = "data/users.db");

  bool load();
  bool userExists(const std::string& username) const;
  std::string getPasswordHash(const std::string& username) const;
  void addUser(const std::string& username, const std::string& password_hash);

 private:
  std::string db_path_;
  std::unordered_map<std::string, std::string> users_;
};

}  // namespace sailor::auth
