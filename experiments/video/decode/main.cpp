#include "decode_vau.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

namespace dec = learning::video::decode;
namespace fs = std::filesystem;

bool parse_args(int argc, char** argv, fs::path& input, std::string& error) {
  input = dec::kDefaultInputRelativePath;
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

  dec::DecodeStats stats;
  if (!dec::decode_vau_to_nv12(".", input, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "decoded"
            << " input=" << input.string()
            << " frames_decoded=" << stats.frames_decoded
            << " width=" << stats.width
            << " height=" << stats.height
            << " pixel_format=" << dec::kPixelFormatToken
            << " frame_bytes=" << stats.frame_bytes
            << " manifest=" << dec::kManifestRelativePath
            << " preview=" << dec::kPreviewRelativePath << '\n';
  return 0;
}
