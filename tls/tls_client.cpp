#include "tls_client.hpp"

#include <openssl/err.h>

TlsClientContext::TlsClientContext() = default;

TlsClientContext::~TlsClientContext() {
  if (ctx_) {
    SSL_CTX_free(ctx_);
    ctx_ = nullptr;
  }
}

bool TlsClientContext::initialize() {
  const SSL_METHOD* method = TLS_client_method();
  ctx_ = SSL_CTX_new(method);
  if (!ctx_) {
    ERR_print_errors_fp(stderr);
    return false;
  }

  // Development mode: Skip peer verification for self-signed certificates
  SSL_CTX_set_verify(ctx_, SSL_VERIFY_NONE, nullptr);
  SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

  return true;
}
