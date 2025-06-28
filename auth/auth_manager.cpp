#include "auth_manager.hpp"

#include "common/hash.hpp"

namespace sailor::auth {

AuthManager::AuthManager(const std::string& db_path) : user_store_(db_path) {
  user_store_.load();
}

bool AuthManager::authenticate(const std::string& username,
                               const std::string& raw_password) {
  if (!user_store_.userExists(username)) {
    return false;
  }

  std::string stored_hash = user_store_.getPasswordHash(username);
  std::string incoming_hash = common::sha256(raw_password);

  return !stored_hash.empty() && (stored_hash == incoming_hash);
}

}  // namespace sailor::auth
