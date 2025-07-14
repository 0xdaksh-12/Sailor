#pragma once

#include <string>

namespace sailor::common {

std::string sha256(const std::string& input);
std::string sha256File(const std::string& filepath);

}  // namespace sailor::common
