#include "render_pcm.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace ren = learning::audio::render;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_audio_render_" + stamp);
  fs::create_directories(root);
  return root;
}

void put_le16(std::ofstream& out, std::uint16_t v) {
  const char b[2] = {static_cast<char>(v & 0xff),
                     static_cast<char>((v >> 8) & 0xff)};
  out.write(b, 2);
}

void put_le32(std::ofstream& out, std::uint32_t v) {
  const char b[4] = {static_cast<char>(v & 0xff),
                     static_cast<char>((v >> 8) & 0xff),
                     static_cast<char>((v >> 16) & 0xff),
                     static_cast<char>((v >> 24) & 0xff)};
  out.write(b, 4);
}

struct WavSpec {
  std::uint32_t rate = 48000;
  std::uint16_t channels = 1;
  std::uint16_t bits = 16;
  bool list_chunk = false;
};

// Hand-written PCM WAV so the test does not depend on any stage's writer.
fs::path write_wav(const fs::path& root, const std::string& name,
                   const WavSpec& spec, const std::vector<std::uint8_t>& data) {
  const fs::path path = root / name;
  std::ofstream out(path, std::ios::binary);
  const std::uint16_t block_align =
      static_cast<std::uint16_t>(spec.channels * (spec.bits / 8));
  const std::string list_body("INFOISFT\x04\x00\x00\x00test", 16);
  const std::uint32_t list_bytes =
      spec.list_chunk ? 8 + static_cast<std::uint32_t>(list_body.size()) : 0;
  const auto data_bytes = static_cast<std::uint32_t>(data.size());

  out.write("RIFF", 4);
  put_le32(out, 4 + (8 + 16) + list_bytes + (8 + data_bytes));
  out.write("WAVE", 4);
  out.write("fmt ", 4);
  put_le32(out, 16);
  put_le16(out, 1);
  put_le16(out, spec.channels);
  put_le32(out, spec.rate);
  put_le32(out, spec.rate * block_align);
  put_le16(out, block_align);
  put_le16(out, spec.bits);
  if (spec.list_chunk) {
    out.write("LIST", 4);
    put_le32(out, static_cast<std::uint32_t>(list_body.size()));
    out.write(list_body.data(), static_cast<std::streamsize>(list_body.size()));
  }
  out.write("data", 4);
  put_le32(out, data_bytes);
  out.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
  return path;
}

}  // namespace

TEST_CASE("render loads 48k mono s16le WAV samples in order",
          "[audio][render]") {
  const fs::path root = make_temp_root();
  WavSpec spec;
  spec.list_chunk = true;
  // Little-endian bytes for samples 0x1234, -2 (0xFFFE), 0x7FFF.
  const fs::path wav =
      write_wav(root, "ok.wav", spec, {0x34, 0x12, 0xFE, 0xFF, 0xFF, 0x7F});

  std::vector<std::int16_t> pcm;
  std::string error;
  REQUIRE(ren::load_render_wav(wav, pcm, error));
  REQUIRE(error.empty());
  REQUIRE(pcm == std::vector<std::int16_t>{0x1234, -2, 0x7FFF});

  fs::remove_all(root);
}

TEST_CASE("pcm cursor hands out buffer-sized chunks until all frames rendered",
          "[audio][render]") {
  std::vector<std::int16_t> pcm(2500);
  for (std::size_t i = 0; i < pcm.size(); ++i) {
    pcm[i] = static_cast<std::int16_t>(i);
  }
  ren::PcmCursor cursor(pcm);
  std::vector<std::int16_t> buffer(960, -1);

  REQUIRE(cursor.fill(buffer.data(), buffer.size()) == 960);
  REQUIRE(buffer.front() == 0);
  REQUIRE(buffer.back() == 959);
  REQUIRE(cursor.fill(buffer.data(), buffer.size()) == 960);
  REQUIRE(buffer.front() == 960);
  REQUIRE_FALSE(cursor.done());

  REQUIRE(cursor.fill(buffer.data(), buffer.size()) == 580);
  REQUIRE(buffer.front() == 1920);
  REQUIRE(buffer[579] == 2499);
  REQUIRE(cursor.done());
  REQUIRE(cursor.frames_consumed() == 2500);

  REQUIRE(cursor.fill(buffer.data(), buffer.size()) == 0);
  REQUIRE(cursor.frames_consumed() == 2500);
}

TEST_CASE("render rejects WAV outside the 48k mono s16le contract",
          "[audio][render]") {
  const fs::path root = make_temp_root();
  const std::vector<std::uint8_t> four_bytes{1, 0, 2, 0};

  WavSpec rate_44k;
  rate_44k.rate = 44100;
  WavSpec stereo;
  stereo.channels = 2;
  WavSpec eight_bit;
  eight_bit.bits = 8;

  std::vector<std::int16_t> pcm;
  std::string error;
  REQUIRE_FALSE(ren::load_render_wav(
      write_wav(root, "44k.wav", rate_44k, four_bytes), pcm, error));
  REQUIRE_FALSE(error.empty());
  REQUIRE_FALSE(ren::load_render_wav(
      write_wav(root, "stereo.wav", stereo, four_bytes), pcm, error));
  REQUIRE_FALSE(ren::load_render_wav(
      write_wav(root, "8bit.wav", eight_bit, four_bytes), pcm, error));
  REQUIRE_FALSE(
      ren::load_render_wav(write_wav(root, "empty.wav", WavSpec{}, {}), pcm,
                           error));
  REQUIRE_FALSE(ren::load_render_wav(root / "missing.wav", pcm, error));

  fs::remove_all(root);
}
