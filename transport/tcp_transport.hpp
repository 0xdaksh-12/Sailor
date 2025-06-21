#pragma once

#include "itransport.hpp"

class TcpTransport : public ITransport {
 public:
  explicit TcpTransport(int fd);
  ~TcpTransport() override = default;

  bool sendAll(const uint8_t* data, size_t size) override;
  bool recvAll(uint8_t* data, size_t size) override;

  int fd() const { return fd_; }

 private:
  int fd_;
};
