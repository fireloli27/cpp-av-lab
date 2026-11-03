#include "render_frames.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rend = learning::video::render;

namespace {

void write_one_frame(const fs::path& dir, std::uint8_t y) {
  fs::create_directories(dir / "frames");
  std::vector<char> bytes(rend::kFrameBytes, 0);
  const std::size_t y_bytes =
      static_cast<std::size_t>(rend::kWidth) * rend::kHeight;
  for (std::size_t i = 0; i < y_bytes; ++i) {
    bytes[i] = static_cast<char>(y);
  }
  std::ofstream frame(dir / "frames/frame-000000.nv12", std::ios::binary);
  frame.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  std::ofstream manifest(dir / "manifest.csv");
  manifest << "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,"
              "bytes\n";
  manifest << "0,frames/frame-000000.nv12,1280,720,420v,50,0,1382400\n";
}

}  // namespace

TEST_CASE("presentation delays follow relative PTS and clamp long gaps",
          "[video][render]") {
  std::vector<std::int64_t> delays;
  std::string error;
  REQUIRE(rend::presentation_delays_ns({0, 33333333, 66666666}, delays, error));
  REQUIRE(error.empty());
  REQUIRE(delays.size() == 3);
  REQUIRE(delays[0] == 0);
  REQUIRE(delays[1] == 33333333);
  REQUIRE(delays[2] == 33333333);

  REQUIRE(rend::presentation_delays_ns({0, 500000000}, delays, error));
  REQUIRE(delays[1] == rend::kMaxGapNs);

  REQUIRE_FALSE(rend::presentation_delays_ns({0, 0}, delays, error));
  REQUIRE_FALSE(rend::presentation_delays_ns({}, delays, error));
}

TEST_CASE("render loader accepts a decode-layout directory and rejects format drift",
          "[video][render]") {
  const fs::path dir =
      fs::temp_directory_path() / "learning_video_render_loader";
  fs::remove_all(dir);
  write_one_frame(dir, 90);

  std::vector<rend::RenderFrame> frames;
  std::string error;
  REQUIRE(rend::load_frame_directory(dir, frames, error));
  REQUIRE(frames.size() == 1);
  REQUIRE(frames[0].pts_ns == 50);
  REQUIRE(frames[0].relative_pts_ns == 0);
  REQUIRE(frames[0].nv12.size() == 1382400);
  REQUIRE(frames[0].nv12[0] == 90);

  {
    std::ofstream manifest(dir / "manifest.csv");
    manifest << "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,"
                "bytes\n";
    manifest << "0,frames/frame-000000.nv12,1280,720,BGRA,50,0,1382400\n";
  }
  REQUIRE_FALSE(rend::load_frame_directory(dir, frames, error));
  REQUIRE(error.find("1280x720") != std::string::npos);
  fs::remove_all(dir);
}
