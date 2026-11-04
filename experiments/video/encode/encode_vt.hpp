#pragma once

#include "load_capture.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::video::encode {

inline constexpr const char* kCodecToken = "H264";
inline constexpr const char* kEncodeRelativePath = "out/video/encode/encode.vau";
inline constexpr const char* kManifestRelativePath =
    "out/video/encode/manifest.csv";
inline constexpr const char* kDefaultInputRelativePath = "out/video/capture";
inline constexpr std::uint32_t kBitrateBps = 2000000;
inline constexpr int kFrameRate = 30;
inline constexpr int kMaxKeyFrameInterval = 30;

struct EncodeStats {
  int width = kWidth;
  int height = kHeight;
  std::uint32_t bitrate_bps = kBitrateBps;
  std::uint32_t frames_encoded = 0;
  std::uint32_t access_units = 0;
  std::uint32_t keyframes = 0;
  std::uint64_t output_bytes = 0;
};

// Encodes tight NV12 420v frames (1280x720, strictly increasing pts_ns) to
// H.264 Baseline with VTCompressionSession and publishes encode.vau + manifest
// under root/out/video/encode/. One Raw Video Frame becomes one access unit.
// AllowFrameReordering is off. Bitrate is fixed; no CLI override.
bool encode_nv12_to_vau(const std::filesystem::path& root,
                        const std::vector<RawFrame>& frames, EncodeStats& stats,
                        std::string& error);

}  // namespace learning::video::encode
