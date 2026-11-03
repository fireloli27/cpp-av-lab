#include "capture_publish.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace cap = learning::video::capture;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_video_capture_" + stamp);
  fs::create_directories(root);
  return root;
}

std::vector<std::uint8_t> read_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

std::string read_text(const fs::path& path) {
  std::ifstream in(path);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Synthetic 4x2 NV12: distinct Y ramp + mid-gray chroma so PNG is non-empty.
std::vector<std::uint8_t> make_synth_nv12_4x2() {
  constexpr int w = 4;
  constexpr int h = 2;
  const std::size_t nbytes = cap::nv12_tight_bytes(w, h);
  std::vector<std::uint8_t> nv12(nbytes, 0);
  for (int i = 0; i < w * h; ++i) {
    nv12[static_cast<std::size_t>(i)] =
        static_cast<std::uint8_t>(40 + i * 20);
  }
  for (std::size_t i = static_cast<std::size_t>(w * h); i < nbytes; ++i) {
    nv12[i] = 128;
  }
  return nv12;
}

bool png_signature(const std::vector<std::uint8_t>& bytes) {
  static const std::uint8_t kSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A,
                                       0x0A};
  if (bytes.size() < 8) {
    return false;
  }
  for (int i = 0; i < 8; ++i) {
    if (bytes[static_cast<std::size_t>(i)] != kSig[i]) {
      return false;
    }
  }
  return true;
}

}  // namespace

TEST_CASE("nv12 tight byte count matches 720p contract",
          "[video][capture]") {
  // Literals from issue 07 / ADR-0006 (independent of helper constants).
  REQUIRE(cap::nv12_tight_bytes(1280, 720) == 1382400);
  REQUIRE(cap::nv12_tight_bytes(4, 2) == 4 * 2 + 2 * 2 * 1);  // 8 + 4 = 12
}

TEST_CASE("pack_nv12 strips row stride padding into tight layout",
          "[video][capture]") {
  constexpr int w = 4;
  constexpr int h = 2;
  constexpr std::size_t y_stride = 8;   // padded (visible row is 4)
  constexpr std::size_t uv_stride = 8;  // padded (visible UV row is 4)
  std::vector<std::uint8_t> y_plane(y_stride * h, 0xEE);
  std::vector<std::uint8_t> uv_plane(uv_stride * ((h + 1) / 2), 0xEE);
  for (int row = 0; row < h; ++row) {
    for (int col = 0; col < w; ++col) {
      y_plane[static_cast<std::size_t>(row) * y_stride +
              static_cast<std::size_t>(col)] =
          static_cast<std::uint8_t>(10 + row * w + col);
    }
  }
  for (int col = 0; col < 4; ++col) {
    uv_plane[static_cast<std::size_t>(col)] =
        static_cast<std::uint8_t>(100 + col);
  }

  std::vector<std::uint8_t> tight;
  std::string error;
  REQUIRE(cap::pack_nv12_planes_to_tight(y_plane.data(), y_stride,
                                         uv_plane.data(), uv_stride, w, h,
                                         tight, error));
  REQUIRE(error.empty());
  REQUIRE(tight.size() == 12);
  REQUIRE(tight[0] == 10);
  REQUIRE(tight[1] == 11);
  REQUIRE(tight[2] == 12);
  REQUIRE(tight[3] == 13);
  REQUIRE(tight[4] == 14);
  REQUIRE(tight[5] == 15);
  REQUIRE(tight[6] == 16);
  REQUIRE(tight[7] == 17);
  REQUIRE(tight[8] == 100);
  REQUIRE(tight[9] == 101);
  REQUIRE(tight[10] == 102);
  REQUIRE(tight[11] == 103);
  // Padding bytes must not leak into the tight payload.
  for (std::uint8_t b : tight) {
    REQUIRE(b != 0xEE);
  }
}

TEST_CASE("relative PTS zeros the first frame", "[video][capture]") {
  REQUIRE(cap::relative_pts_ns(5'000'000'000LL, 5'000'000'000LL) == 0);
  REQUIRE(cap::relative_pts_ns(5'033'333'333LL, 5'000'000'000LL) ==
          33'333'333LL);
}

TEST_CASE("publish writes tight frames, CSV, PNG via staging",
          "[video][capture]") {
  const fs::path root = make_temp_root();
  constexpr int w = 4;
  constexpr int h = 2;

  // Seed a prior publish with a distinct first byte so replace is observable.
  {
    std::vector<cap::CapturedFrame> prior(1);
    prior[0].nv12 = make_synth_nv12_4x2();
    prior[0].nv12[0] = 1;
    prior[0].pts_ns = 1'000'000'000LL;
    std::string err;
    REQUIRE(cap::publish_capture_artifacts(root, prior, w, h, err));
  }
  const fs::path publish = root / cap::kPublishRelativeDir;
  const fs::path prior_frame0 = publish / "frames" / "frame-000000.nv12";
  REQUIRE(fs::is_regular_file(prior_frame0));
  const auto prior_bytes = read_bytes(prior_frame0);

  std::vector<cap::CapturedFrame> frames(2);
  frames[0].nv12 = make_synth_nv12_4x2();
  frames[0].pts_ns = 9'000'000'000LL;
  frames[1].nv12 = make_synth_nv12_4x2();
  frames[1].nv12[0] = 255;  // distinguish second frame
  frames[1].pts_ns = 9'033'333'333LL;

  std::string error;
  REQUIRE(cap::publish_capture_artifacts(root, frames, w, h, error));
  REQUIRE(error.empty());

  REQUIRE_FALSE(fs::exists(root / cap::kStagingRelativeDir));
  REQUIRE(fs::is_regular_file(publish / "frames" / "frame-000000.nv12"));
  REQUIRE(fs::is_regular_file(publish / "frames" / "frame-000001.nv12"));
  REQUIRE(fs::is_regular_file(publish / "manifest.csv"));
  REQUIRE(fs::is_regular_file(publish / "preview.png"));

  const auto f0 = read_bytes(publish / "frames" / "frame-000000.nv12");
  const auto f1 = read_bytes(publish / "frames" / "frame-000001.nv12");
  REQUIRE(f0.size() == 12);
  REQUIRE(f1.size() == 12);
  REQUIRE(f0 == frames[0].nv12);
  REQUIRE(f1 == frames[1].nv12);
  REQUIRE(f0 != prior_bytes);  // replaced prior publish

  const std::string csv = read_text(publish / "manifest.csv");
  REQUIRE(csv.find("frame_index,file,width,height,pixel_format,pts_ns,"
                    "relative_pts_ns,bytes") != std::string::npos);
  REQUIRE(csv.find("0,frames/frame-000000.nv12,4,2,420v,9000000000,0,12") !=
          std::string::npos);
  REQUIRE(csv.find(
              "1,frames/frame-000001.nv12,4,2,420v,9033333333,33333333,12") !=
          std::string::npos);

  const auto png = read_bytes(publish / "preview.png");
  REQUIRE(png_signature(png));
  REQUIRE(png.size() > 32);

  fs::remove_all(root);
}

TEST_CASE("failed publish leaves prior capture directory intact",
          "[video][capture]") {
  const fs::path root = make_temp_root();
  constexpr int w = 4;
  constexpr int h = 2;

  std::vector<cap::CapturedFrame> prior(1);
  prior[0].nv12 = make_synth_nv12_4x2();
  prior[0].pts_ns = 100;
  std::string err;
  REQUIRE(cap::publish_capture_artifacts(root, prior, w, h, err));
  const auto prior_bytes =
      read_bytes(root / cap::kPublishRelativeDir / "frames" /
                 "frame-000000.nv12");

  // Wrong byte count → publish must fail without touching prior output.
  std::vector<cap::CapturedFrame> bad(1);
  bad[0].nv12 = {1, 2, 3};  // not tight 4x2
  bad[0].pts_ns = 200;
  REQUIRE_FALSE(cap::publish_capture_artifacts(root, bad, w, h, err));
  REQUIRE_FALSE(err.empty());
  REQUIRE_FALSE(fs::exists(root / cap::kStagingRelativeDir));
  REQUIRE(read_bytes(root / cap::kPublishRelativeDir / "frames" /
                     "frame-000000.nv12") == prior_bytes);

  fs::remove_all(root);
}
