#pragma once

#include <cstdint>
#include <string>

namespace sailor::server {

struct Session {
  uint64_t id{0};
  std::string username{};
  bool authenticated{false};
};

}  // namespace sailor::server
