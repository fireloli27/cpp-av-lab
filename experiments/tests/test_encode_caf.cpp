#include "encode_caf.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace enc = learning::audio::encode;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_audio_encode_" + stamp);
  fs::create_directories(root);
  return root;
}

// ~1 s of 440 Hz tone at contract rate (enough for several Opus packets).
std::vector<std::int16_t> make_tone_pcm() {
  constexpr double kFreqHz = 440.0;
  const std::size_t n = enc::kSampleRateHz;
  std::vector<std::int16_t> pcm(n);
  for (std::size_t i = 0; i < n; ++i) {
    const double t = static_cast<double>(i) / enc::kSampleRateHz;
    const double s = std::sin(2.0 * 3.14159265358979323846 * kFreqHz * t);
    pcm[i] = static_cast<std::int16_t>(s * 16000.0);
  }
  return pcm;
}

}  // namespace

TEST_CASE("encode caf writer emits non-empty Opus CAF from PCM",
          "[audio][encode]") {
  const fs::path root = make_temp_root();
  const auto pcm = make_tone_pcm();
  enc::EncodeStats stats;
  std::string error;

  REQUIRE(enc::write_encode_caf(root, pcm, stats, error));
  REQUIRE(error.empty());

  const fs::path out_path = root / enc::kEncodeRelativePath;
  REQUIRE(fs::is_regular_file(out_path));
  REQUIRE(stats.output_bytes > 0);
  REQUIRE(stats.packet_count > 0);
  REQUIRE(stats.duration_s > 0.9);
  REQUIRE(stats.frames_per_packet > 0);
  REQUIRE(fs::file_size(out_path) == stats.output_bytes);

  const std::string path_str = out_path.string();
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path_str.data()),
      static_cast<CFIndex>(path_str.size()), false);
  REQUIRE(url != nullptr);

  AudioFileID file = nullptr;
  const OSStatus open_status =
      AudioFileOpenURL(url, kAudioFileReadPermission, kAudioFileCAFType, &file);
  CFRelease(url);
  REQUIRE(open_status == noErr);
  REQUIRE(file != nullptr);

  AudioStreamBasicDescription asbd{};
  UInt32 asbd_size = sizeof(asbd);
  const OSStatus fmt_status = AudioFileGetProperty(
      file, kAudioFilePropertyDataFormat, &asbd_size, &asbd);
  AudioFileClose(file);

  REQUIRE(fmt_status == noErr);
  // Literal matches grilled codec (independent of encode header constants).
  REQUIRE(asbd.mFormatID == 'opus');
  REQUIRE(asbd.mSampleRate == 48000);
  REQUIRE(asbd.mChannelsPerFrame == 1);

  fs::remove_all(root);
}
