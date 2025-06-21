#pragma once

#include <openssl/err.h>
#include <openssl/ssl.h>

inline void initializeTls() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
#else
  OPENSSL_init_ssl(
      OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS,
      nullptr);
#endif
}

inline void cleanupTls() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  ERR_free_strings();
  EVP_cleanup();
#endif
}
