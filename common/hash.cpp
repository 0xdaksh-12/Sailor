#include "hash.hpp"

#include <openssl/evp.h>

#include <iomanip>
#include <sstream>

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

}  // namespace sailor::common
