#include "vau.hpp"

#include <fstream>
#include <sstream>

namespace learning::video::encode {
namespace {

void append_be16(std::vector<std::uint8_t>& out, std::uint16_t v) {
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void append_be32(std::vector<std::uint8_t>& out, std::uint32_t v) {
  out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void append_be64(std::vector<std::uint8_t>& out, std::uint64_t v) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xff));
  }
}

bool read_exact(std::istream& in, std::uint8_t* dst, std::size_t n,
                std::string& error) {
  if (n == 0) {
    return true;
  }
  in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
  if (!in) {
    error = "truncated VAU";
    return false;
  }
  return true;
}

std::uint16_t load_be16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(p[0]) << 8) |
                                   p[1]);
}

std::uint32_t load_be32(const std::uint8_t* p) {
  return (static_cast<std::uint32_t>(p[0]) << 24) |
         (static_cast<std::uint32_t>(p[1]) << 16) |
         (static_cast<std::uint32_t>(p[2]) << 8) |
         static_cast<std::uint32_t>(p[3]);
}

std::uint64_t load_be64(const std::uint8_t* p) {
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v = (v << 8) | p[i];
  }
  return v;
}

bool validate_for_write(const VauFile& file, std::string& error) {
  if (file.width != static_cast<std::uint32_t>(kContractWidth) ||
      file.height != static_cast<std::uint32_t>(kContractHeight)) {
    error = "VAU width/height must be 1280x720";
    return false;
  }
  if (file.access_units.empty() ||
      file.access_units.size() > kMaxAccessUnits) {
    error = "VAU access unit count must be in [1, 90]";
    return false;
  }
  if (file.parameter_sets.empty() || file.parameter_sets.size() > 8) {
    error = "VAU parameter set count must be in [1, 8]";
    return false;
  }
  for (const auto& set : file.parameter_sets) {
    if (set.nal.empty() || set.nal.size() > 2048) {
      error = "VAU parameter set NAL size must be in [1, 2048]";
      return false;
    }
  }
  std::int64_t prev_pts = -1;
  for (std::size_t i = 0; i < file.access_units.size(); ++i) {
    const auto& au = file.access_units[i];
    if (au.pts_ns < 0) {
      error = "VAU pts_ns must be >= 0";
      return false;
    }
    if (i > 0 && au.pts_ns <= prev_pts) {
      error = "VAU pts_ns must be strictly increasing";
      return false;
    }
    prev_pts = au.pts_ns;
    if (au.payload.empty() || au.payload.size() > kMaxAccessUnitBytes) {
      error = "VAU access unit payload size out of range";
      return false;
    }
  }
  if (!file.access_units.front().keyframe) {
    error = "VAU first access unit must be a keyframe";
    return false;
  }
  return true;
}

}  // namespace

bool write_vau(const std::filesystem::path& path, const VauFile& file,
               std::string& error) {
  if (!validate_for_write(file, error)) {
    return false;
  }

  std::vector<std::uint8_t> bytes;
  bytes.reserve(64 + file.access_units.size() * 64);
  bytes.insert(bytes.end(), kVauMagic, kVauMagic + 4);
  append_be16(bytes, kVauVersion);
  append_be16(bytes, 0);
  append_be32(bytes, kCodecAvc1);
  append_be32(bytes, file.width);
  append_be32(bytes, file.height);
  append_be32(bytes, file.bitrate_bps);
  append_be32(bytes, static_cast<std::uint32_t>(file.access_units.size()));
  append_be32(bytes, static_cast<std::uint32_t>(file.parameter_sets.size()));
  for (const auto& set : file.parameter_sets) {
    append_be16(bytes, static_cast<std::uint16_t>(set.nal.size()));
    bytes.insert(bytes.end(), set.nal.begin(), set.nal.end());
  }
  for (const auto& au : file.access_units) {
    append_be64(bytes, static_cast<std::uint64_t>(au.pts_ns));
    append_be32(bytes, au.keyframe ? kAccessUnitFlagKeyframe : 0u);
    append_be32(bytes, static_cast<std::uint32_t>(au.payload.size()));
    bytes.insert(bytes.end(), au.payload.begin(), au.payload.end());
  }

  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    error = "failed to create " + path.parent_path().string() + ": " +
            ec.message();
    return false;
  }

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    error = "failed to open " + path.string();
    return false;
  }
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  if (!out.good()) {
    error = "failed while writing " + path.string();
    return false;
  }
  out.close();
  if (!out) {
    error = "failed while closing " + path.string();
    return false;
  }
  error.clear();
  return true;
}

bool read_vau(const std::filesystem::path& path, VauFile& file,
              std::string& error) {
  file = {};
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "failed to open VAU: " + path.string();
    return false;
  }

  std::uint8_t header[32];
  if (!read_exact(in, header, sizeof(header), error)) {
    return false;
  }
  if (header[0] != kVauMagic[0] || header[1] != kVauMagic[1] ||
      header[2] != kVauMagic[2] || header[3] != kVauMagic[3]) {
    error = "not an LVAU file: " + path.string();
    return false;
  }
  const std::uint16_t version = load_be16(header + 4);
  const std::uint16_t reserved = load_be16(header + 6);
  const std::uint32_t codec = load_be32(header + 8);
  if (version != kVauVersion || reserved != 0) {
    error = "unsupported VAU version";
    return false;
  }
  if (codec != kCodecAvc1) {
    std::ostringstream oss;
    oss << "VAU codec fourcc is not avc1: 0x" << std::hex << codec;
    error = oss.str();
    return false;
  }
  file.width = load_be32(header + 12);
  file.height = load_be32(header + 16);
  file.bitrate_bps = load_be32(header + 20);
  const std::uint32_t au_count = load_be32(header + 24);
  const std::uint32_t ps_count = load_be32(header + 28);
  if (file.width != static_cast<std::uint32_t>(kContractWidth) ||
      file.height != static_cast<std::uint32_t>(kContractHeight)) {
    error = "VAU width/height must be 1280x720";
    return false;
  }
  if (au_count == 0 || au_count > kMaxAccessUnits) {
    error = "VAU access unit count out of range";
    return false;
  }
  if (ps_count == 0 || ps_count > 8) {
    error = "VAU parameter set count out of range";
    return false;
  }

  file.parameter_sets.resize(ps_count);
  for (std::uint32_t i = 0; i < ps_count; ++i) {
    std::uint8_t len_buf[2];
    if (!read_exact(in, len_buf, 2, error)) {
      return false;
    }
    const std::uint16_t nal_len = load_be16(len_buf);
    if (nal_len == 0 || nal_len > 2048) {
      error = "VAU parameter set NAL size out of range";
      return false;
    }
    file.parameter_sets[i].nal.resize(nal_len);
    if (!read_exact(in, file.parameter_sets[i].nal.data(), nal_len, error)) {
      return false;
    }
  }

  file.access_units.resize(au_count);
  std::int64_t prev_pts = -1;
  for (std::uint32_t i = 0; i < au_count; ++i) {
    std::uint8_t au_hdr[16];
    if (!read_exact(in, au_hdr, sizeof(au_hdr), error)) {
      return false;
    }
    const std::uint64_t pts_u = load_be64(au_hdr);
    if (pts_u > static_cast<std::uint64_t>(INT64_MAX)) {
      error = "VAU pts_ns out of range";
      return false;
    }
    const auto pts = static_cast<std::int64_t>(pts_u);
    if (i > 0 && pts <= prev_pts) {
      error = "VAU pts_ns must be strictly increasing";
      return false;
    }
    prev_pts = pts;
    const std::uint32_t flags = load_be32(au_hdr + 8);
    if ((flags & ~kAccessUnitFlagKeyframe) != 0) {
      error = "VAU access unit has unknown flags";
      return false;
    }
    const std::uint32_t payload_bytes = load_be32(au_hdr + 12);
    if (payload_bytes == 0 ||
        payload_bytes > static_cast<std::uint32_t>(kMaxAccessUnitBytes)) {
      error = "VAU access unit payload size out of range";
      return false;
    }
    auto& au = file.access_units[i];
    au.pts_ns = pts;
    au.keyframe = (flags & kAccessUnitFlagKeyframe) != 0;
    au.payload.resize(payload_bytes);
    if (!read_exact(in, au.payload.data(), payload_bytes, error)) {
      return false;
    }
  }
  if (!file.access_units.front().keyframe) {
    error = "VAU first access unit must be a keyframe";
    return false;
  }

  char extra = 0;
  in.read(&extra, 1);
  if (in.gcount() != 0) {
    error = "VAU has trailing bytes";
    return false;
  }

  error.clear();
  return true;
}

}  // namespace learning::video::encode
