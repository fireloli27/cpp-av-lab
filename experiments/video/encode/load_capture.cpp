#include "load_capture.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace learning::video::encode {
namespace {

constexpr const char* kManifestHeader =
    "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,bytes";

std::string strip_cr(std::string s) {
  if (!s.empty() && s.back() == '\r') {
    s.pop_back();
  }
  return s;
}

bool parse_i64(const std::string& text, std::int64_t& out) {
  if (text.empty()) {
    return false;
  }
  char* end = nullptr;
  errno = 0;
  const long long value = std::strtoll(text.c_str(), &end, 10);
  if (errno != 0 || end != text.c_str() + text.size()) {
    return false;
  }
  out = static_cast<std::int64_t>(value);
  return true;
}

bool parse_u64(const std::string& text, std::uint64_t& out) {
  if (text.empty() || text[0] == '-') {
    return false;
  }
  char* end = nullptr;
  errno = 0;
  const unsigned long long value = std::strtoull(text.c_str(), &end, 10);
  if (errno != 0 || end != text.c_str() + text.size()) {
    return false;
  }
  out = static_cast<std::uint64_t>(value);
  return true;
}

std::vector<std::string> split_csv(const std::string& line) {
  std::vector<std::string> fields;
  std::string cur;
  for (char c : line) {
    if (c == ',') {
      fields.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  fields.push_back(cur);
  return fields;
}

std::vector<std::uint8_t> read_file(const std::filesystem::path& path,
                                    std::string& error) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "failed to open frame: " + path.string();
    return {};
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
  if (!in.eof() && in.fail()) {
    error = "failed while reading frame: " + path.string();
    return {};
  }
  error.clear();
  return bytes;
}

}  // namespace

bool load_capture_directory(const std::filesystem::path& dir,
                            std::vector<RawFrame>& frames, std::string& error) {
  frames.clear();
  const std::filesystem::path manifest = dir / "manifest.csv";
  std::ifstream in(manifest);
  if (!in) {
    error = "failed to open " + manifest.string() +
            " (run learning_video_capture first, or pass --input)";
    return false;
  }

  std::string header;
  if (!std::getline(in, header)) {
    error = "empty manifest: " + manifest.string();
    return false;
  }
  header = strip_cr(header);
  if (header != kManifestHeader) {
    error = "manifest header mismatch: " + manifest.string();
    return false;
  }

  std::int64_t prev_pts = -1;
  std::int64_t first_pts = 0;
  std::string line;
  while (std::getline(in, line)) {
    line = strip_cr(line);
    if (line.empty()) {
      continue;
    }
    if (frames.size() >= static_cast<std::size_t>(kMaxFrames)) {
      error = "manifest has more than 90 frames";
      return false;
    }
    const auto fields = split_csv(line);
    if (fields.size() != 8) {
      error = "manifest row does not have 8 fields";
      return false;
    }
    std::uint64_t index = 0;
    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t pts = 0;
    std::int64_t relative = 0;
    std::uint64_t nbytes = 0;
    if (!parse_u64(fields[0], index) || !parse_i64(fields[2], width) ||
        !parse_i64(fields[3], height) || !parse_i64(fields[5], pts) ||
        !parse_i64(fields[6], relative) || !parse_u64(fields[7], nbytes)) {
      error = "manifest row has a non-integer field";
      return false;
    }
    if (index != frames.size()) {
      error = "manifest frame_index is not sequential from 0";
      return false;
    }
    if (width != kWidth || height != kHeight ||
        fields[4] != kPixelFormatToken || nbytes != kFrameBytes) {
      error =
          "manifest frame is not 1280x720 420v tight NV12 (1382400 bytes)";
      return false;
    }
    char expected_name[64];
    std::snprintf(expected_name, sizeof(expected_name),
                  "frames/frame-%06zu.nv12", frames.size());
    if (fields[1] != expected_name) {
      error = "manifest file column is not the expected frame path";
      return false;
    }
    if (pts < 0 || (frames.size() > 0 && pts <= prev_pts)) {
      error = "manifest pts_ns must be >= 0 and strictly increasing";
      return false;
    }
    if (frames.empty()) {
      first_pts = pts;
    }
    if (relative != pts - first_pts) {
      error = "manifest relative_pts_ns is not pts_ns minus the first PTS";
      return false;
    }
    prev_pts = pts;

    std::string read_error;
    auto nv12 = read_file(dir / fields[1], read_error);
    if (!read_error.empty()) {
      error = read_error;
      return false;
    }
    if (nv12.size() != kFrameBytes) {
      error = "frame file size is not 1382400: " + fields[1];
      return false;
    }
    RawFrame frame;
    frame.nv12 = std::move(nv12);
    frame.pts_ns = pts;
    frames.push_back(std::move(frame));
  }

  if (frames.empty()) {
    error = "manifest has no frames: " + manifest.string();
    return false;
  }
  error.clear();
  return true;
}

}  // namespace learning::video::encode
