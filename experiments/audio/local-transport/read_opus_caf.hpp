#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::audio::local_transport {

inline constexpr const char* kDefaultInputRelativePath =
    "out/audio/encode/encode.caf";

// Reads Opus access units from a CAF via AudioToolbox (no third-party media lib).
// Rejects non-Opus / unreadable files. Does not decode to PCM.
bool read_opus_packets_from_caf(const std::filesystem::path& path,
                                std::vector<std::vector<std::uint8_t>>& packets,
                                std::string& error);

}  // namespace learning::audio::local_transport
