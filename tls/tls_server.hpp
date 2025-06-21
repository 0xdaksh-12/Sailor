#pragma once

#include <openssl/ssl.h>

#include <string>

class TlsServerContext {
 public:
  TlsServerContext(std::string cert_path, std::string key_path);
  ~TlsServerContext();

  TlsServerContext(const TlsServerContext&) = delete;
  TlsServerContext& operator=(const TlsServerContext&) = delete;

  bool initialize();
  SSL_CTX* get() const { return ctx_; }

 private:
  std::string cert_path_;
  std::string key_path_;
  SSL_CTX* ctx_{nullptr};
};
