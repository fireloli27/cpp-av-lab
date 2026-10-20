#include "capture_wav.hpp"

#include <fstream>

namespace learning::audio::capture {
namespace {

void write_le16(std::ostream& out, std::uint16_t v) {
  const unsigned char b[2] = {static_cast<unsigned char>(v & 0xff),
                              static_cast<unsigned char>((v >> 8) & 0xff)};
  out.write(reinterpret_cast<const char*>(b), 2);
}

void write_le32(std::ostream& out, std::uint32_t v) {
  const unsigned char b[4] = {
      static_cast<unsigned char>(v & 0xff),
      static_cast<unsigned char>((v >> 8) & 0xff),
      static_cast<unsigned char>((v >> 16) & 0xff),
      static_cast<unsigned char>((v >> 24) & 0xff)};
  out.write(reinterpret_cast<const char*>(b), 4);
}

}  // namespace

bool write_capture_wav(const std::filesystem::path& root,
                       const std::vector<std::int16_t>& pcm, std::string& error) {
  namespace fs = std::filesystem;

  const fs::path out_path = root / kCaptureRelativePath;
  std::error_code ec;
  fs::create_directories(out_path.parent_path(), ec);
  if (ec) {
    error = "failed to create " + out_path.parent_path().string() + ": " +
            ec.message();
    return false;
  }

  const std::uint32_t data_bytes =
      static_cast<std::uint32_t>(pcm.size() * sizeof(std::int16_t));
  const std::uint32_t riff_size = 36 + data_bytes;  // after "RIFF" size field
  const std::uint32_t byte_rate =
      kSampleRateHz * kChannels * (kBitsPerSample / 8);
  const std::uint16_t block_align =
      static_cast<std::uint16_t>(kChannels * (kBitsPerSample / 8));

  std::ofstream out(out_path, std::ios::binary);
  if (!out) {
    error = "failed to open " + out_path.string();
    return false;
  }

  out.write("RIFF", 4);
  write_le32(out, riff_size);
  out.write("WAVE", 4);
  out.write("fmt ", 4);
  write_le32(out, 16);
  write_le16(out, 1);  // PCM
  write_le16(out, kChannels);
  write_le32(out, kSampleRateHz);
  write_le32(out, byte_rate);
  write_le16(out, block_align);
  write_le16(out, kBitsPerSample);
  out.write("data", 4);
  write_le32(out, data_bytes);
  if (!pcm.empty()) {
    out.write(reinterpret_cast<const char*>(pcm.data()),
              static_cast<std::streamsize>(data_bytes));
  }
  if (!out.good()) {
    error = "failed while writing " + out_path.string();
    return false;
  }
  out.close();
  if (out.fail()) {
    error = "failed while closing " + out_path.string();
    return false;
  }

  error.clear();
  return true;
}

}  // namespace learning::audio::capture
