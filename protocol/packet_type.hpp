#pragma once

#include <cstdint>

enum class PacketType : uint32_t {
  PING = 1,
  PONG = 2,

  CONNECT = 3,

  LIST = 4,
  LIST_RESPONSE = 5,

  UPLOAD_BEGIN = 6,
  UPLOAD_CHUNK = 7,
  UPLOAD_END = 8,

  DOWNLOAD_REQUEST = 9,
  DOWNLOAD_CHUNK = 10,

  DELETE_FILE = 11,
  RENAME_FILE = 12,
  MKDIR = 13,

  SUCCESS = 14,
  ERROR = 15
};
