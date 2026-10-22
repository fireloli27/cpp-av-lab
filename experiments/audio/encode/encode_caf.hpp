#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::audio::encode {

inline constexpr const char* kEncodeRelativePath = "out/audio/encode/encode.caf";
inline constexpr const char* kDefaultInputRelativePath =
    "out/audio/capture/capture.wav";
inline constexpr std::uint32_t kSampleRateHz = 48000;
inline constexpr std::uint16_t kChannels = 1;
inline constexpr std::uint16_t kBitsPerSample = 16;
inline constexpr std::uint32_t kBitRateBps = 64000;

struct EncodeStats {
  std::uint64_t packet_count = 0;
  std::uint64_t output_bytes = 0;
  double duration_s = 0.0;
  std::uint32_t frames_per_packet = 0;
};

// Encodes PCM (48 kHz / mono / s16le) to Opus in a CAF under `root` at
// kEncodeRelativePath. Uses AudioToolbox AudioConverter + AudioFileWritePackets.
bool write_encode_caf(const std::filesystem::path& root,
                      const std::vector<std::int16_t>& pcm, EncodeStats& stats,
                      std::string& error);

}  // namespace learning::audio::encode
