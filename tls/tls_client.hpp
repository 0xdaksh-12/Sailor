#pragma once

#include <openssl/ssl.h>

#include <string>

class TlsClientContext {
 public:
  TlsClientContext(std::string ca_cert_path = "certs/cert.pem",
                   std::string client_cert_path = "certs/client-cert.pem",
                   std::string client_key_path = "certs/client-key.pem");
  ~TlsClientContext();

  TlsClientContext(const TlsClientContext&) = delete;
  TlsClientContext& operator=(const TlsClientContext&) = delete;

  bool initialize();
  SSL_CTX* get() const { return ctx_; }

 private:
  std::string ca_cert_path_;
  std::string client_cert_path_;
  std::string client_key_path_;
  SSL_CTX* ctx_{nullptr};
};
