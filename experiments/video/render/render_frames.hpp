#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::video::render {

inline constexpr int kWidth = 1280;
inline constexpr int kHeight = 720;
inline constexpr const char* kPixelFormatToken = "420v";
inline constexpr std::size_t kFrameBytes = 1382400;
inline constexpr int kMaxFrames = 90;
inline constexpr const char* kDefaultInputRelativePath = "out/video/decode";
// Gaps larger than this are clamped so a timestamp jump cannot stall the window.
inline constexpr std::int64_t kMaxGapNs = 200000000;

struct RenderFrame {
  std::vector<std::uint8_t> nv12;
  std::int64_t pts_ns = 0;
  std::int64_t relative_pts_ns = 0;
};

// Reads a capture-layout directory (the decode output, or an equivalent
// capture directory). 1280x720 tight NV12 420v, sequential names, relative PTS
// equals pts minus the first PTS.
bool load_frame_directory(const std::filesystem::path& dir,
                          std::vector<RenderFrame>& frames, std::string& error);

// delays_ns[0] is 0. Later entries are the wait before showing that frame:
// relative_pts delta, clamped to kMaxGapNs. A non-positive delta fails.
bool presentation_delays_ns(const std::vector<std::int64_t>& relative_pts_ns,
                            std::vector<std::int64_t>& delays_ns,
                            std::string& error);

}  // namespace learning::video::render
