#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace learning::video::encode {

// Learning container for H.264 access units. Not a product container and not
// RTP. Layout (all multi-byte integers big-endian) is the SSOT in
// docs/learning/video/encode.notes.md.

inline constexpr char kVauMagic[4] = {'L', 'V', 'A', 'U'};
inline constexpr std::uint16_t kVauVersion = 1;
inline constexpr std::uint32_t kCodecAvc1 = 0x61766331u;  // 'avc1'
inline constexpr std::uint32_t kAccessUnitFlagKeyframe = 1u;
inline constexpr int kContractWidth = 1280;
inline constexpr int kContractHeight = 720;
inline constexpr std::uint32_t kMaxAccessUnits = 90;
inline constexpr std::size_t kMaxAccessUnitBytes = 4u * 1024u * 1024u;

struct ParameterSet {
  std::vector<std::uint8_t> nal;  // raw SPS/PPS NAL, no start code
};

struct AccessUnit {
  std::int64_t pts_ns = 0;
  bool keyframe = false;
  // AVCC: repeated uint32-be length + NAL bytes. Length size is 4.
  std::vector<std::uint8_t> payload;
};

struct VauFile {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t bitrate_bps = 0;
  std::vector<ParameterSet> parameter_sets;
  std::vector<AccessUnit> access_units;
};

bool write_vau(const std::filesystem::path& path, const VauFile& file,
               std::string& error);

bool read_vau(const std::filesystem::path& path, VauFile& file,
              std::string& error);

}  // namespace learning::video::encode
