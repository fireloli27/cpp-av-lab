#include "encode_vt.hpp"
#include "load_capture.hpp"
#include "vau.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace enc = learning::video::encode;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_video_encode_" + stamp);
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

enc::VauFile sample_vau() {
  enc::VauFile file;
  file.width = 1280;
  file.height = 720;
  file.bitrate_bps = enc::kBitrateBps;
  enc::ParameterSet sps;
  sps.nal = {0x67, 0x42, 0x00, 0x1e};
  enc::ParameterSet pps;
  pps.nal = {0x68, 0xce, 0x38, 0x80};
  file.parameter_sets = {sps, pps};
  enc::AccessUnit idr;
  idr.pts_ns = 0;
  idr.keyframe = true;
  idr.payload = {0x00, 0x00, 0x00, 0x02, 0x65, 0x88};
  enc::AccessUnit pframe;
  pframe.pts_ns = 33333333;
  pframe.keyframe = false;
  pframe.payload = {0x00, 0x00, 0x00, 0x02, 0x41, 0x9a};
  file.access_units = {idr, pframe};
  return file;
}

void write_capture_dir(const fs::path& dir, const std::vector<enc::RawFrame>& frames) {
  fs::create_directories(dir / "frames");
  std::ofstream manifest(dir / "manifest.csv");
  manifest << "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,"
              "bytes\n";
  const auto first = frames.front().pts_ns;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    char name[64];
    std::snprintf(name, sizeof(name), "frame-%06zu.nv12", i);
    std::ofstream frame(dir / "frames" / name, std::ios::binary);
    frame.write(reinterpret_cast<const char*>(frames[i].nv12.data()),
                static_cast<std::streamsize>(frames[i].nv12.size()));
    manifest << i << ",frames/" << name << ",1280,720,420v," << frames[i].pts_ns
             << ',' << (frames[i].pts_ns - first) << ',' << frames[i].nv12.size()
             << '\n';
  }
}

}  // namespace

TEST_CASE("vau writer roundtrips H.264 access units", "[video][encode]") {
  const fs::path root = make_temp_root();
  const fs::path path = root / "sample.vau";
  const enc::VauFile original = sample_vau();
  std::string error;
  REQUIRE(enc::write_vau(path, original, error));
  REQUIRE(error.empty());

  enc::VauFile loaded;
  REQUIRE(enc::read_vau(path, loaded, error));
  REQUIRE(error.empty());
  REQUIRE(loaded.width == 1280);
  REQUIRE(loaded.height == 720);
  REQUIRE(loaded.bitrate_bps == enc::kBitrateBps);
  REQUIRE(loaded.parameter_sets.size() == 2);
  REQUIRE(loaded.parameter_sets[0].nal == original.parameter_sets[0].nal);
  REQUIRE(loaded.parameter_sets[1].nal == original.parameter_sets[1].nal);
  REQUIRE(loaded.access_units.size() == 2);
  REQUIRE(loaded.access_units[0].keyframe);
  REQUIRE_FALSE(loaded.access_units[1].keyframe);
  REQUIRE(loaded.access_units[0].pts_ns == 0);
  REQUIRE(loaded.access_units[1].pts_ns == 33333333);
  REQUIRE(loaded.access_units[0].payload == original.access_units[0].payload);
  REQUIRE(loaded.access_units[1].payload == original.access_units[1].payload);
  fs::remove_all(root);
}

TEST_CASE("vau reader rejects a bad magic and trailing bytes", "[video][encode]") {
  const fs::path root = make_temp_root();
  const fs::path path = root / "sample.vau";
  std::string error;
  REQUIRE(enc::write_vau(path, sample_vau(), error));

  {
    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
    char bad = 'X';
    file.seekp(0);
    file.write(&bad, 1);
  }
  enc::VauFile loaded;
  REQUIRE_FALSE(enc::read_vau(path, loaded, error));
  REQUIRE(error.find("LVAU") != std::string::npos);

  REQUIRE(enc::write_vau(path, sample_vau(), error));
  {
    std::ofstream extra(path, std::ios::binary | std::ios::app);
    extra.put('Z');
  }
  REQUIRE_FALSE(enc::read_vau(path, loaded, error));
  REQUIRE(error.find("trailing") != std::string::npos);
  fs::remove_all(root);
}

TEST_CASE("capture directory loader enforces the NV12 contract", "[video][encode]") {
  const fs::path root = make_temp_root();
  enc::RawFrame frame;
  frame.nv12 = solid_nv12(40);
  frame.pts_ns = 1000;
  write_capture_dir(root, {frame});

  std::vector<enc::RawFrame> loaded;
  std::string error;
  REQUIRE(enc::load_capture_directory(root, loaded, error));
  REQUIRE(error.empty());
  REQUIRE(loaded.size() == 1);
  REQUIRE(loaded[0].pts_ns == 1000);
  REQUIRE(loaded[0].nv12.size() == 1382400);
  REQUIRE(loaded[0].nv12[0] == 40);

  std::ofstream manifest(root / "manifest.csv");
  manifest << "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,"
              "bytes\n";
  manifest << "0,frames/frame-000000.nv12,640,480,420v,1000,0,1382400\n";
  REQUIRE_FALSE(enc::load_capture_directory(root, loaded, error));
  REQUIRE(error.find("1280x720") != std::string::npos);
  fs::remove_all(root);
}

TEST_CASE("encoder writes a non-empty H.264 VAU from synthetic NV12",
          "[video][encode]") {
  const fs::path root = make_temp_root();
  std::vector<enc::RawFrame> frames(2);
  frames[0].nv12 = solid_nv12(48);
  frames[0].pts_ns = 0;
  frames[1].nv12 = solid_nv12(200);
  frames[1].pts_ns = 33333333;

  enc::EncodeStats stats;
  std::string error;
  REQUIRE(enc::encode_nv12_to_vau(root, frames, stats, error));
  REQUIRE(error.empty());
  REQUIRE(stats.frames_encoded == 2);
  REQUIRE(stats.access_units == 2);
  REQUIRE(stats.keyframes >= 1);
  REQUIRE(stats.width == 1280);
  REQUIRE(stats.height == 720);
  REQUIRE(stats.bitrate_bps == 2000000);
  REQUIRE(stats.output_bytes > 0);

  const fs::path vau_path = root / enc::kEncodeRelativePath;
  REQUIRE(fs::is_regular_file(vau_path));
  REQUIRE(fs::file_size(vau_path) == stats.output_bytes);
  REQUIRE(fs::is_regular_file(root / enc::kManifestRelativePath));

  enc::VauFile loaded;
  REQUIRE(enc::read_vau(vau_path, loaded, error));
  REQUIRE(loaded.access_units.size() == 2);
  REQUIRE(loaded.access_units[0].keyframe);
  REQUIRE(loaded.access_units[0].pts_ns == 0);
  REQUIRE(loaded.access_units[1].pts_ns == 33333333);
  bool saw_sps = false;
  bool saw_pps = false;
  for (const auto& set : loaded.parameter_sets) {
    REQUIRE_FALSE(set.nal.empty());
    const int nal_type = set.nal[0] & 0x1f;
    if (nal_type == 7) {
      saw_sps = true;
    }
    if (nal_type == 8) {
      saw_pps = true;
    }
  }
  REQUIRE(saw_sps);
  REQUIRE(saw_pps);

  std::ifstream manifest(root / enc::kManifestRelativePath);
  std::string header;
  std::getline(manifest, header);
  REQUIRE(header == "au_index,pts_ns,is_keyframe,bytes");
  fs::remove_all(root);
}
