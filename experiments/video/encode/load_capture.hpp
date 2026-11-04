#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::video::encode {

inline constexpr int kWidth = 1280;
inline constexpr int kHeight = 720;
inline constexpr const char* kPixelFormatToken = "420v";
inline constexpr std::size_t kFrameBytes = 1382400;
inline constexpr int kMaxFrames = 90;

struct RawFrame {
  std::vector<std::uint8_t> nv12;
  std::int64_t pts_ns = 0;
};

// Reads a capture-layout directory (manifest.csv + frames/*.nv12).
// Contract matches docs/learning/video/capture.notes.md: 1280x720 tight NV12
// 420v, sequential frame_index from 0, strictly increasing pts_ns, at most 90.
bool load_capture_directory(const std::filesystem::path& dir,
                            std::vector<RawFrame>& frames, std::string& error);

}  // namespace learning::video::encode
