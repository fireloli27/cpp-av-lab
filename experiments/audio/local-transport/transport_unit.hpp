#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace learning::audio::local_transport {

// Transport Unit = uint32 big-endian length + Opus payload bytes.
// length = payload size only (does not include these 4 bytes). No seq/timestamp/SSRC.

std::vector<std::uint8_t> pack_transport_unit(
    const std::vector<std::uint8_t>& payload);

bool unpack_transport_unit(const std::vector<std::uint8_t>& unit,
                           std::vector<std::uint8_t>& payload,
                           std::string& error);

}  // namespace learning::audio::local_transport
