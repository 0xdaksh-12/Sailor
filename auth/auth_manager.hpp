#pragma once

#include <string>

#include "user_store.hpp"

namespace sailor::auth {

class AuthManager {
 public:
  explicit AuthManager(const std::string& db_path = "data/users.db");

  bool authenticate(const std::string& username,
                    const std::string& raw_password);

 private:
  UserStore user_store_;
};

}  // namespace sailor::auth
