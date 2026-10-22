#include "transport_unit.hpp"

#include <cstring>
#include <sstream>

namespace learning::audio::local_transport {

std::vector<std::uint8_t> pack_transport_unit(
    const std::vector<std::uint8_t>& payload) {
  const auto len = static_cast<std::uint32_t>(payload.size());
  std::vector<std::uint8_t> unit(4 + payload.size());
  unit[0] = static_cast<std::uint8_t>((len >> 24) & 0xff);
  unit[1] = static_cast<std::uint8_t>((len >> 16) & 0xff);
  unit[2] = static_cast<std::uint8_t>((len >> 8) & 0xff);
  unit[3] = static_cast<std::uint8_t>(len & 0xff);
  if (!payload.empty()) {
    std::memcpy(unit.data() + 4, payload.data(), payload.size());
  }
  return unit;
}

bool unpack_transport_unit(const std::vector<std::uint8_t>& unit,
                           std::vector<std::uint8_t>& payload,
                           std::string& error) {
  if (unit.size() < 4) {
    error = "transport unit shorter than 4-byte length prefix";
    return false;
  }
  const std::uint32_t len =
      (static_cast<std::uint32_t>(unit[0]) << 24) |
      (static_cast<std::uint32_t>(unit[1]) << 16) |
      (static_cast<std::uint32_t>(unit[2]) << 8) |
      static_cast<std::uint32_t>(unit[3]);
  if (unit.size() != 4 + static_cast<std::size_t>(len)) {
    std::ostringstream oss;
    oss << "transport unit length mismatch: prefix=" << len
        << " remaining=" << (unit.size() - 4);
    error = oss.str();
    return false;
  }
  payload.assign(unit.begin() + 4, unit.end());
  error.clear();
  return true;
}

}  // namespace learning::audio::local_transport
