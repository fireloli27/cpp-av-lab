#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::audio::render {

inline constexpr const char* kDefaultInputRelativePath =
    "out/audio/decode/decode.wav";
inline constexpr std::uint32_t kSampleRateHz = 48000;
inline constexpr std::uint16_t kChannels = 1;
inline constexpr std::uint16_t kBitsPerSample = 16;
inline constexpr std::uint32_t kNumBuffers = 3;
inline constexpr std::uint32_t kBufferFrames = 960;  // 20 ms at 48 kHz

// Reads a 48 kHz / mono / s16le PCM WAV via AudioToolbox AudioFileOpenURL +
// AudioFileReadPacketData. Any other format, or zero frames, fails.
bool load_render_wav(const std::filesystem::path& input,
                     std::vector<std::int16_t>& pcm, std::string& error);

// Read position into loaded PCM. The AudioQueue output callback pulls the next
// buffer's worth of frames from here; the last chunk may be short.
class PcmCursor {
 public:
  explicit PcmCursor(const std::vector<std::int16_t>& pcm);

  // Copies up to `max_frames` frames into `dst`; returns frames copied (0 at end).
  std::size_t fill(std::int16_t* dst, std::size_t max_frames);
  bool done() const;
  std::uint64_t frames_consumed() const;

 private:
  const std::vector<std::int16_t>& pcm_;
  std::size_t position_ = 0;
};

}  // namespace learning::audio::render
