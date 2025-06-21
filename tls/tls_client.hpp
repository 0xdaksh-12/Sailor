#pragma once

#include <openssl/ssl.h>

class TlsClientContext {
 public:
  TlsClientContext();
  ~TlsClientContext();

  TlsClientContext(const TlsClientContext&) = delete;
  TlsClientContext& operator=(const TlsClientContext&) = delete;

  bool initialize();
  SSL_CTX* get() const { return ctx_; }

 private:
  SSL_CTX* ctx_{nullptr};
};
