#pragma once

#include <openssl/ssl.h>

#include "itransport.hpp"

class TlsTransport : public ITransport {
 public:
  // Takes ownership of ssl handle if take_ownership is true
  TlsTransport(int fd, SSL* ssl, bool take_ownership = true);
  ~TlsTransport() override;

  TlsTransport(const TlsTransport&) = delete;
  TlsTransport& operator=(const TlsTransport&) = delete;

  bool sendAll(const uint8_t* data, size_t size) override;
  bool recvAll(uint8_t* data, size_t size) override;

  SSL* ssl() const { return ssl_; }
  int fd() const { return fd_; }

 private:
  int fd_;
  SSL* ssl_;
  bool take_ownership_;
};
