#include "tls_transport.hpp"

#include <openssl/err.h>
#include <unistd.h>

TlsTransport::TlsTransport(int fd, SSL* ssl, bool take_ownership)
    : fd_(fd), ssl_(ssl), take_ownership_(take_ownership) {}

TlsTransport::~TlsTransport() {
  if (take_ownership_ && ssl_) {
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    ssl_ = nullptr;
  }
}

bool TlsTransport::sendAll(const uint8_t* data, size_t size) {
  if (!ssl_) return false;

  size_t total = 0;
  while (total < size) {
    int sent = SSL_write(ssl_, data + total, static_cast<int>(size - total));
    if (sent <= 0) {
      return false;
    }
    total += static_cast<size_t>(sent);
  }
  return true;
}

bool TlsTransport::recvAll(uint8_t* data, size_t size) {
  if (!ssl_) return false;

  size_t total = 0;
  while (total < size) {
    int received = SSL_read(ssl_, data + total, static_cast<int>(size - total));
    if (received <= 0) {
      return false;
    }
    total += static_cast<size_t>(received);
  }
  return true;
}
