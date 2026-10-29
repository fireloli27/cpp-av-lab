#include "decode_wav.hpp"
#include "encode_caf.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace enc = learning::audio::encode;
namespace dec = learning::audio::decode;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_audio_decode_" + stamp);
  fs::create_directories(root);
  return root;
}

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

std::uint16_t read_le16(const unsigned char* p) {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

std::uint32_t read_le32(const unsigned char* p) {
  return static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) |
                                   (p[3] << 24));
}

struct WavPcm {
  std::uint16_t audio_format = 0;
  std::uint16_t channels = 0;
  std::uint32_t sample_rate = 0;
  std::uint16_t bits = 0;
  std::vector<std::int16_t> samples;
};

bool read_wav_pcm(const fs::path& path, WavPcm& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  unsigned char header[12];
  if (!in.read(reinterpret_cast<char*>(header), 12)) {
    return false;
  }
  if (std::memcmp(header, "RIFF", 4) != 0 ||
      std::memcmp(header + 8, "WAVE", 4) != 0) {
    return false;
  }

  bool got_fmt = false;
  bool got_data = false;
  std::vector<unsigned char> data_bytes;
  while (in && !(got_fmt && got_data)) {
    unsigned char chunk_hdr[8];
    if (!in.read(reinterpret_cast<char*>(chunk_hdr), 8)) {
      break;
    }
    const std::uint32_t chunk_size = read_le32(chunk_hdr + 4);
    if (std::memcmp(chunk_hdr, "fmt ", 4) == 0) {
      if (chunk_size < 16) {
        return false;
      }
      std::vector<unsigned char> fmt(chunk_size);
      if (!in.read(reinterpret_cast<char*>(fmt.data()),
                   static_cast<std::streamsize>(chunk_size))) {
        return false;
      }
      out.audio_format = read_le16(fmt.data());
      out.channels = read_le16(fmt.data() + 2);
      out.sample_rate = read_le32(fmt.data() + 4);
      out.bits = read_le16(fmt.data() + 14);
      got_fmt = true;
      if (chunk_size % 2 == 1) {
        in.ignore(1);
      }
    } else if (std::memcmp(chunk_hdr, "data", 4) == 0) {
      data_bytes.resize(chunk_size);
      if (chunk_size > 0 &&
          !in.read(reinterpret_cast<char*>(data_bytes.data()),
                   static_cast<std::streamsize>(chunk_size))) {
        return false;
      }
      got_data = true;
    } else {
      in.seekg(static_cast<std::streamoff>(chunk_size), std::ios::cur);
      if (chunk_size % 2 == 1) {
        in.ignore(1);
      }
    }
  }
  if (!got_fmt || !got_data || data_bytes.size() % 2 != 0) {
    return false;
  }
  out.samples.resize(data_bytes.size() / sizeof(std::int16_t));
  if (!out.samples.empty()) {
    std::memcpy(out.samples.data(), data_bytes.data(), data_bytes.size());
  }
  return true;
}

bool caf_packet_timeline(const fs::path& path, std::uint64_t& packet_count,
                         std::uint32_t& frames_per_packet) {
  const std::string path_str = path.string();
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path_str.data()),
      static_cast<CFIndex>(path_str.size()), false);
  if (url == nullptr) {
    return false;
  }
  AudioFileID file = nullptr;
  const OSStatus open_status =
      AudioFileOpenURL(url, kAudioFileReadPermission, kAudioFileCAFType, &file);
  CFRelease(url);
  if (open_status != noErr || file == nullptr) {
    return false;
  }

  AudioStreamBasicDescription asbd{};
  UInt32 asbd_size = sizeof(asbd);
  const OSStatus fmt_status = AudioFileGetProperty(
      file, kAudioFilePropertyDataFormat, &asbd_size, &asbd);
  UInt64 count = 0;
  UInt32 count_size = sizeof(count);
  const OSStatus count_status = AudioFileGetProperty(
      file, kAudioFilePropertyAudioDataPacketCount, &count_size, &count);
  AudioFileClose(file);
  if (fmt_status != noErr || count_status != noErr || asbd.mFormatID != 'opus') {
    return false;
  }
  packet_count = count;
  frames_per_packet = asbd.mFramesPerPacket;
  return packet_count > 0 && frames_per_packet > 0;
}

}  // namespace

TEST_CASE("decode wav writer emits 48k mono s16le PCM from Opus CAF",
          "[audio][decode]") {
  const fs::path root = make_temp_root();
  enc::EncodeStats encode_stats;
  std::string error;
  REQUIRE(enc::write_encode_caf(root, make_tone_pcm(), encode_stats, error));

  const fs::path caf_path = root / enc::kEncodeRelativePath;
  dec::DecodeStats stats;
  REQUIRE(dec::write_decode_wav(root, caf_path, stats, error));
  REQUIRE(error.empty());

  const fs::path wav_path = root / dec::kDecodeRelativePath;
  WavPcm wav;
  REQUIRE(read_wav_pcm(wav_path, wav));
  REQUIRE(wav.audio_format == 1);
  REQUIRE(wav.sample_rate == 48000);
  REQUIRE(wav.channels == 1);
  REQUIRE(wav.bits == 16);

  const bool any_nonzero =
      std::any_of(wav.samples.begin(), wav.samples.end(),
                  [](std::int16_t s) { return s != 0; });
  REQUIRE(any_nonzero);

  std::uint64_t packet_count = 0;
  std::uint32_t frames_per_packet = 0;
  REQUIRE(caf_packet_timeline(caf_path, packet_count, frames_per_packet));
  const std::uint64_t source_frames = packet_count * frames_per_packet;
  const std::uint64_t pcm_frames = wav.samples.size();
  REQUIRE(pcm_frames > 0);
  REQUIRE(pcm_frames >= source_frames / 2);
  REQUIRE(pcm_frames <= source_frames * 2);
  REQUIRE(stats.packets == packet_count);
  REQUIRE(stats.pcm_frames == pcm_frames);
  REQUIRE(stats.frames_per_packet == frames_per_packet);

  fs::remove_all(root);
}
