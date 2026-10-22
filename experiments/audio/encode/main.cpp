#include "encode_caf.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace enc = learning::audio::encode;
namespace fs = std::filesystem;

std::uint16_t read_le16(const unsigned char* p) {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

std::uint32_t read_le32(const unsigned char* p) {
  return static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) |
                                   (p[3] << 24));
}

bool parse_args(int argc, char** argv, fs::path& input, std::string& error) {
  input = enc::kDefaultInputRelativePath;
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

// Reads a RIFF/WAVE PCM file; rejects anything other than 48 kHz mono s16le.
bool read_contract_pcm_wav(const fs::path& path, std::vector<std::int16_t>& pcm,
                           std::string& error) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "failed to open input WAV: " + path.string() +
            " (run learning_audio_capture first, or pass --input)";
    return false;
  }

  unsigned char header[12];
  if (!in.read(reinterpret_cast<char*>(header), 12)) {
    error = "WAV too short (missing RIFF header): " + path.string();
    return false;
  }
  if (std::memcmp(header, "RIFF", 4) != 0 ||
      std::memcmp(header + 8, "WAVE", 4) != 0) {
    error = "not a RIFF/WAVE file: " + path.string();
    return false;
  }

  bool got_fmt = false;
  bool got_data = false;
  std::uint16_t audio_format = 0;
  std::uint16_t channels = 0;
  std::uint32_t sample_rate = 0;
  std::uint16_t bits = 0;
  std::vector<unsigned char> data_bytes;

  while (in && !(got_fmt && got_data)) {
    unsigned char chunk_hdr[8];
    if (!in.read(reinterpret_cast<char*>(chunk_hdr), 8)) {
      break;
    }
    const std::uint32_t chunk_size = read_le32(chunk_hdr + 4);
    if (std::memcmp(chunk_hdr, "fmt ", 4) == 0) {
      if (chunk_size < 16) {
        error = "fmt chunk too small";
        return false;
      }
      std::vector<unsigned char> fmt(chunk_size);
      if (!in.read(reinterpret_cast<char*>(fmt.data()),
                   static_cast<std::streamsize>(chunk_size))) {
        error = "failed reading fmt chunk";
        return false;
      }
      audio_format = read_le16(fmt.data());
      channels = read_le16(fmt.data() + 2);
      sample_rate = read_le32(fmt.data() + 4);
      bits = read_le16(fmt.data() + 14);
      got_fmt = true;
      if (chunk_size % 2 == 1) {
        in.ignore(1);
      }
    } else if (std::memcmp(chunk_hdr, "data", 4) == 0) {
      data_bytes.resize(chunk_size);
      if (chunk_size > 0 &&
          !in.read(reinterpret_cast<char*>(data_bytes.data()),
                   static_cast<std::streamsize>(chunk_size))) {
        error = "failed reading data chunk";
        return false;
      }
      got_data = true;
      if (chunk_size % 2 == 1) {
        in.ignore(1);
      }
    } else {
      in.seekg(static_cast<std::streamoff>(chunk_size), std::ios::cur);
      if (chunk_size % 2 == 1) {
        in.ignore(1);
      }
    }
  }

  if (!got_fmt || !got_data) {
    error = "WAV missing fmt or data chunk: " + path.string();
    return false;
  }

  if (audio_format != 1 || channels != enc::kChannels ||
      sample_rate != enc::kSampleRateHz || bits != enc::kBitsPerSample) {
    error = "WAV must be 48 kHz mono s16le PCM (got format=" +
            std::to_string(audio_format) +
            " rate=" + std::to_string(sample_rate) +
            " ch=" + std::to_string(channels) + " bits=" + std::to_string(bits) +
            ")";
    return false;
  }

  if (data_bytes.size() % sizeof(std::int16_t) != 0) {
    error = "data chunk size is not a multiple of 2 (s16le)";
    return false;
  }

  pcm.resize(data_bytes.size() / sizeof(std::int16_t));
  if (!pcm.empty()) {
    std::memcpy(pcm.data(), data_bytes.data(), data_bytes.size());
  }
  if (pcm.empty()) {
    error = "WAV data chunk is empty";
    return false;
  }

  error.clear();
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
  if (!read_contract_pcm_wav(input, pcm, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  enc::EncodeStats stats;
  if (!enc::write_encode_caf(".", pcm, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "encoded " << input.string() << " -> " << enc::kEncodeRelativePath
            << " rate=" << enc::kSampleRateHz << " channels=" << enc::kChannels
            << " bits=" << enc::kBitsPerSample << " bitrate_bps=" << enc::kBitRateBps
            << " duration_s≈" << stats.duration_s
            << " packets=" << stats.packet_count
            << " frames_per_packet=" << stats.frames_per_packet
            << " output_bytes=" << stats.output_bytes << '\n';
  return 0;
}
