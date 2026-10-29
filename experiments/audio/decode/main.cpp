#include "decode_wav.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

namespace dec = learning::audio::decode;
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
  if (!dec::write_decode_wav(".", input, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "decoded " << input.string() << " -> " << dec::kDecodeRelativePath
            << " packets=" << stats.packets
            << " pcm_frames=" << stats.pcm_frames
            << " frames_per_packet=" << stats.frames_per_packet << '\n';
  return 0;
}
