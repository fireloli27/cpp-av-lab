#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace learning::audio::decode {

inline constexpr const char* kDecodeRelativePath = "out/audio/decode/decode.wav";
inline constexpr const char* kDefaultInputRelativePath =
    "out/audio/encode/encode.caf";
inline constexpr std::uint32_t kSampleRateHz = 48000;
inline constexpr std::uint16_t kChannels = 1;
inline constexpr std::uint16_t kBitsPerSample = 16;

struct DecodeStats {
  std::uint64_t packets = 0;
  std::uint64_t pcm_frames = 0;
  std::uint32_t frames_per_packet = 0;
};

// Decodes an Opus CAF (48 kHz / mono access units) to a PCM WAV under `root`
// at kDecodeRelativePath. Uses AudioToolbox AudioFileReadPacketData +
// AudioConverter (one access unit per converter pull). Does not trim CAF priming.
bool write_decode_wav(const std::filesystem::path& root,
                      const std::filesystem::path& input_caf, DecodeStats& stats,
                      std::string& error);

}  // namespace learning::audio::decode
