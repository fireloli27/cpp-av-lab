#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace learning::video::decode {

inline constexpr int kWidth = 1280;
inline constexpr int kHeight = 720;
inline constexpr const char* kPixelFormatToken = "420v";
inline constexpr std::size_t kFrameBytes = 1382400;
inline constexpr const char* kDefaultInputRelativePath =
    "out/video/encode/encode.vau";
inline constexpr const char* kManifestRelativePath =
    "out/video/decode/manifest.csv";
inline constexpr const char* kPreviewRelativePath = "out/video/decode/preview.png";

struct DecodeStats {
  int width = kWidth;
  int height = kHeight;
  std::uint32_t frames_decoded = 0;
  std::uint32_t frame_bytes = static_cast<std::uint32_t>(kFrameBytes);
};

// Decodes a learning VAU (H.264 access units + SPS/PPS) to tight NV12 frames
// under root/out/video/decode/. Uses VTDecompressionSession, one access unit at
// a time. Does not re-read UDP. Output layout matches the capture directory so
// render can consume it. Lossy: pixels are not required to match the source.
bool decode_vau_to_nv12(const std::filesystem::path& root,
                        const std::filesystem::path& vau_path,
                        DecodeStats& stats, std::string& error);

}  // namespace learning::video::decode
