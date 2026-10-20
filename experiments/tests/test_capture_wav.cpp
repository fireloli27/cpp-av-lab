#include "capture_wav.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace cap = learning::audio::capture;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_audio_capture_" + stamp);
  fs::create_directories(root);
  return root;
}

std::vector<std::uint8_t> read_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

std::uint16_t le16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

std::uint32_t le32(const std::uint8_t* p) {
  return static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) |
                                   (p[3] << 24));
}

}  // namespace

TEST_CASE("capture wav writer emits 48k mono s16le RIFF WAVE",
          "[audio][capture]") {
  const fs::path root = make_temp_root();
  // Two frames of known PCM (not silence): 0x1234, 0x7FFF
  const std::vector<std::int16_t> pcm{0x1234, 0x7FFF};
  std::string error;

  REQUIRE(cap::write_capture_wav(root, pcm, error));
  REQUIRE(error.empty());

  const fs::path out_path = root / cap::kCaptureRelativePath;
  REQUIRE(fs::is_regular_file(out_path));

  const auto bytes = read_bytes(out_path);
  // 12 (RIFF) + 8+16 (fmt) + 8+4 (data) = 44 header + 4 payload
  REQUIRE(bytes.size() == 44 + pcm.size() * sizeof(std::int16_t));

  REQUIRE(std::memcmp(bytes.data(), "RIFF", 4) == 0);
  REQUIRE(le32(bytes.data() + 4) == bytes.size() - 8);
  REQUIRE(std::memcmp(bytes.data() + 8, "WAVE", 4) == 0);

  REQUIRE(std::memcmp(bytes.data() + 12, "fmt ", 4) == 0);
  REQUIRE(le32(bytes.data() + 16) == 16);  // PCM fmt chunk size
  REQUIRE(le16(bytes.data() + 20) == 1);   // PCM format
  REQUIRE(le16(bytes.data() + 22) == 1);   // mono
  // Literals match the grilled PCM contract (independent of header constants).
  REQUIRE(le32(bytes.data() + 24) == 48000);
  REQUIRE(le32(bytes.data() + 28) == 96000);  // byte rate
  REQUIRE(le16(bytes.data() + 32) == 2);      // block align
  REQUIRE(le16(bytes.data() + 34) == 16);     // bits

  REQUIRE(std::memcmp(bytes.data() + 36, "data", 4) == 0);
  REQUIRE(le32(bytes.data() + 40) == 4);
  REQUIRE(bytes[44] == 0x34);
  REQUIRE(bytes[45] == 0x12);
  REQUIRE(bytes[46] == 0xFF);
  REQUIRE(bytes[47] == 0x7F);

  fs::remove_all(root);
}
