#include "encode_vt.hpp"
#include "load_capture.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

namespace enc = learning::video::encode;
namespace fs = std::filesystem;

bool parse_args(int argc, char** argv, fs::path& input, std::string& error) {
  input = enc::kDefaultInputRelativePath;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--input") {
      if (i + 1 >= argc) {
        error = "--input requires a path";
        return false;
      }
      input = argv[++i];
    } else {
      error = "unknown argument: " + arg;
      return false;
    }
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  fs::path input;
  std::string error;
  if (!parse_args(argc, argv, input, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::vector<enc::RawFrame> frames;
  if (!enc::load_capture_directory(input, frames, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  enc::EncodeStats stats;
  if (!enc::encode_nv12_to_vau(".", frames, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "encoded"
            << " input=" << input.string()
            << " codec=" << enc::kCodecToken
            << " width=" << stats.width
            << " height=" << stats.height
            << " pixel_format=" << enc::kPixelFormatToken
            << " bitrate_bps=" << stats.bitrate_bps
            << " frames_encoded=" << stats.frames_encoded
            << " access_units=" << stats.access_units
            << " keyframes=" << stats.keyframes
            << " output_bytes=" << stats.output_bytes
            << " output=" << enc::kEncodeRelativePath
            << " manifest=" << enc::kManifestRelativePath << '\n';
  return 0;
}
