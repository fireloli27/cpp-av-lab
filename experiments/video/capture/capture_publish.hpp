#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::video::capture {

inline constexpr int kWidth = 1280;
inline constexpr int kHeight = 720;
inline constexpr const char* kPixelFormatToken = "420v";
inline constexpr const char* kPublishRelativeDir = "out/video/capture";
inline constexpr const char* kStagingRelativeDir = "out/video/capture.staging";
inline constexpr int kDefaultFrameCount = 30;
inline constexpr int kMinFrameCount = 1;
inline constexpr int kMaxFrameCount = 90;

// Tight NV12: Y (width*height) then interleaved CbCr
// (2 * ceil(width/2) * ceil(height/2)).
std::size_t nv12_tight_bytes(int width, int height);

// Copy biplanar NV12 with possible row padding into a tight buffer.
// y_stride / uv_stride are bytes-per-row of the source planes.
bool pack_nv12_planes_to_tight(const std::uint8_t* y_plane, std::size_t y_stride,
                               const std::uint8_t* uv_plane,
                               std::size_t uv_stride, int width, int height,
                               std::vector<std::uint8_t>& out,
                               std::string& error);

inline std::int64_t relative_pts_ns(std::int64_t pts_ns,
                                   std::int64_t first_pts_ns) {
  return pts_ns - first_pts_ns;
}

struct CapturedFrame {
  std::vector<std::uint8_t> nv12;  // tight-packed
  std::int64_t pts_ns = 0;
};

// Writes frames/manifest/preview under staging, then replaces publish dir.
// On failure: cleans staging and leaves any prior publish directory intact.
// PNG preview is derived from frames[0] via Core Image + ImageIO.
bool publish_capture_artifacts(const std::filesystem::path& root,
                               const std::vector<CapturedFrame>& frames,
                               int width, int height, std::string& error);

// Core Image + ImageIO: tight NV12 → PNG (for tests and publish).
bool write_preview_png_from_nv12(const std::filesystem::path& png_path,
                                 const std::uint8_t* nv12, std::size_t nbytes,
                                 int width, int height, std::string& error);

}  // namespace learning::video::capture
