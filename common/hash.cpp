#include "hash.hpp"

#include <openssl/evp.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace sailor::common {

std::string sha256(const std::string& input) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length = 0;

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return "";

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx, input.data(), input.size()) != 1 ||
      EVP_DigestFinal_ex(ctx, hash, &length) != 1) {
    EVP_MD_CTX_free(ctx);
    return "";
  }
  EVP_MD_CTX_free(ctx);

  std::ostringstream oss;
  for (unsigned int i = 0; i < length; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }
  return oss.str();
}

std::string sha256File(const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open()) return "";

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return "";

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    return "";
  }

  std::vector<char> buffer(64 * 1024);
  while (file.good()) {
    file.read(buffer.data(), buffer.size());
    std::streamsize bytes = file.gcount();
    if (bytes > 0) {
      EVP_DigestUpdate(ctx, buffer.data(), static_cast<size_t>(bytes));
    }
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length = 0;
  EVP_DigestFinal_ex(ctx, hash, &length);
  EVP_MD_CTX_free(ctx);

  std::ostringstream oss;
  for (unsigned int i = 0; i < length; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }
  return oss.str();
}

}  // namespace sailor::common
