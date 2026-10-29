#include "play_queue.hpp"
#include "render_pcm.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace ren = learning::audio::render;
namespace fs = std::filesystem;

bool parse_args(int argc, char** argv, fs::path& input, std::string& error) {
  input = ren::kDefaultInputRelativePath;
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

  std::vector<std::int16_t> pcm;
  if (!ren::load_render_wav(input, pcm, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "rendering " << input.string() << " pcm_frames=" << pcm.size()
            << " duration_s≈"
            << static_cast<double>(pcm.size()) / ren::kSampleRateHz << '\n';

  ren::RenderStats stats;
  if (!ren::play_pcm(pcm, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "rendered " << input.string()
            << " pcm_frames=" << stats.pcm_frames
            << " frames_rendered=" << stats.frames_rendered
            << " buffers=" << stats.buffers
            << " buffer_frames=" << stats.buffer_frames << '\n';
  return 0;
}
