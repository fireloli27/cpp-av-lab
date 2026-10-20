#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::audio::capture {

inline constexpr const char* kCaptureRelativePath = "out/audio/capture/capture.wav";
inline constexpr std::uint32_t kSampleRateHz = 48000;
inline constexpr std::uint16_t kChannels = 1;
inline constexpr std::uint16_t kBitsPerSample = 16;

// Writes a PCM WAV (48 kHz / mono / s16le) under `root` at kCaptureRelativePath.
// `pcm` is interleaved little-endian int16 samples (mono ⇒ one sample per frame).
bool write_capture_wav(const std::filesystem::path& root,
                       const std::vector<std::int16_t>& pcm, std::string& error);

}  // namespace learning::audio::capture
