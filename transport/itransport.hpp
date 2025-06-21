#pragma once

#include <cstddef>
#include <cstdint>

class ITransport {
 public:
  virtual ~ITransport() = default;

  virtual bool sendAll(const uint8_t* data, size_t size) = 0;
  virtual bool recvAll(uint8_t* data, size_t size) = 0;
};
