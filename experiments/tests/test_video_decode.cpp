#include "decode_vau.hpp"
#include "encode_vt.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace enc = learning::video::encode;
namespace dec = learning::video::decode;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_video_decode_" + stamp);
  fs::create_directories(root);
  return root;
}

std::vector<std::uint8_t> solid_nv12(std::uint8_t y) {
  std::vector<std::uint8_t> nv12(enc::kFrameBytes, 128);
  const std::size_t y_bytes =
      static_cast<std::size_t>(enc::kWidth) * enc::kHeight;
  for (std::size_t i = 0; i < y_bytes; ++i) {
    nv12[i] = y;
  }
  return nv12;
}

std::vector<std::uint8_t> read_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

bool near_luma(const std::vector<std::uint8_t>& nv12, std::uint64_t target,
               std::uint64_t tolerance) {
  constexpr std::uint64_t kY = 1280ull * 720ull;
  if (nv12.size() < kY) {
    return false;
  }
  std::uint64_t sum = 0;
  for (std::uint64_t i = 0; i < kY; ++i) {
    sum += nv12[static_cast<std::size_t>(i)];
  }
  const std::uint64_t goal = target * kY;
  const std::uint64_t diff = sum > goal ? sum - goal : goal - sum;
  return diff < tolerance * kY;
}

}  // namespace

TEST_CASE("decoder restores tight NV12 frames from a synthetic H.264 VAU",
          "[video][decode]") {
  const fs::path root = make_temp_root();
  std::vector<enc::RawFrame> frames(2);
  frames[0].nv12 = solid_nv12(48);
  frames[0].pts_ns = 10;
  frames[1].nv12 = solid_nv12(200);
  frames[1].pts_ns = 33333343;

  enc::EncodeStats encoded;
  std::string error;
  const bool encoded_ok =
      enc::encode_nv12_to_vau(root, frames, encoded, error);
  INFO(error);
  REQUIRE(encoded_ok);

  dec::DecodeStats stats;
  const bool decoded_ok = dec::decode_vau_to_nv12(
      root, root / enc::kEncodeRelativePath, stats, error);
  INFO(error);
  REQUIRE(decoded_ok);
  REQUIRE(stats.frames_decoded == 2);
  REQUIRE(stats.width == 1280);
  REQUIRE(stats.height == 720);
  REQUIRE(stats.frame_bytes == 1382400);

  const auto frame0 =
      read_bytes(root / "out/video/decode/frames/frame-000000.nv12");
  const auto frame1 =
      read_bytes(root / "out/video/decode/frames/frame-000001.nv12");
  REQUIRE(frame0.size() == 1382400);
  REQUIRE(frame1.size() == 1382400);
  REQUIRE(near_luma(frame0, 48, 40));
  REQUIRE(near_luma(frame1, 200, 40));

  const auto png = read_bytes(root / dec::kPreviewRelativePath);
  REQUIRE(png.size() > 8);
  REQUIRE(png[0] == 0x89);
  REQUIRE(png[1] == 'P');
  REQUIRE(png[2] == 'N');
  REQUIRE(png[3] == 'G');

  std::ifstream manifest(root / dec::kManifestRelativePath);
  std::string header;
  std::getline(manifest, header);
  REQUIRE(header ==
          "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,bytes");
  std::string row0;
  std::getline(manifest, row0);
  REQUIRE(row0.find("0,frames/frame-000000.nv12,1280,720,420v,10,0,1382400") ==
          0);
  fs::remove_all(root);
}
