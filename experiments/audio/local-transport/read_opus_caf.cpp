#include "read_opus_caf.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <sstream>
#include <vector>

namespace learning::audio::local_transport {
namespace {

std::string osstatus_message(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " failed: " << status;
  return oss.str();
}

}  // namespace

bool read_opus_packets_from_caf(const std::filesystem::path& path,
                                std::vector<std::vector<std::uint8_t>>& packets,
                                std::string& error) {
  packets.clear();

  if (!std::filesystem::is_regular_file(path)) {
    error = "input CAF not found: " + path.string() +
            " (run learning_audio_encode first, or pass --input)";
    return false;
  }

  const std::string path_str = path.string();
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path_str.data()),
      static_cast<CFIndex>(path_str.size()), false);
  if (url == nullptr) {
    error = "CFURLCreateFromFileSystemRepresentation failed for " + path_str;
    return false;
  }

  AudioFileID file = nullptr;
  OSStatus status =
      AudioFileOpenURL(url, kAudioFileReadPermission, kAudioFileCAFType, &file);
  CFRelease(url);
  if (status != noErr || file == nullptr) {
    error = osstatus_message("AudioFileOpenURL", status);
    return false;
  }

  AudioStreamBasicDescription asbd{};
  UInt32 asbd_size = sizeof(asbd);
  status = AudioFileGetProperty(file, kAudioFilePropertyDataFormat, &asbd_size,
                                &asbd);
  if (status != noErr) {
    AudioFileClose(file);
    error = osstatus_message("AudioFileGetProperty(DataFormat)", status);
    return false;
  }
  if (asbd.mFormatID != kAudioFormatOpus) {
    AudioFileClose(file);
    std::ostringstream oss;
    oss << "CAF is not Opus (mFormatID=0x" << std::hex << asbd.mFormatID << ")";
    error = oss.str();
    return false;
  }

  UInt64 packet_count = 0;
  UInt32 prop_size = sizeof(packet_count);
  status = AudioFileGetProperty(file, kAudioFilePropertyAudioDataPacketCount,
                                &prop_size, &packet_count);
  if (status != noErr) {
    AudioFileClose(file);
    error =
        osstatus_message("AudioFileGetProperty(AudioDataPacketCount)", status);
    return false;
  }
  if (packet_count == 0) {
    AudioFileClose(file);
    error = "CAF contains zero Opus packets: " + path_str;
    return false;
  }

  UInt32 max_packet_size = 0;
  prop_size = sizeof(max_packet_size);
  status = AudioFileGetProperty(file, kAudioFilePropertyMaximumPacketSize,
                                &prop_size, &max_packet_size);
  if (status != noErr || max_packet_size == 0) {
    AudioFileClose(file);
    error =
        osstatus_message("AudioFileGetProperty(MaximumPacketSize)", status);
    return false;
  }

  constexpr UInt32 kPacketsPerRead = 32;
  std::vector<std::uint8_t> buf(
      static_cast<std::size_t>(max_packet_size) * kPacketsPerRead);
  std::vector<AudioStreamPacketDescription> descs(kPacketsPerRead);

  SInt64 packet_index = 0;
  while (static_cast<UInt64>(packet_index) < packet_count) {
    UInt32 num_packets = kPacketsPerRead;
    UInt32 num_bytes = static_cast<UInt32>(buf.size());
    status = AudioFileReadPacketData(file, false, &num_bytes, descs.data(),
                                     packet_index, &num_packets, buf.data());
    if (status != noErr) {
      AudioFileClose(file);
      error = osstatus_message("AudioFileReadPacketData", status);
      return false;
    }
    if (num_packets == 0) {
      break;
    }

    for (UInt32 i = 0; i < num_packets; ++i) {
      const auto& d = descs[i];
      if (d.mStartOffset + d.mDataByteSize > num_bytes) {
        AudioFileClose(file);
        error = "packet description out of range in AudioFileReadPacketData";
        return false;
      }
      const auto* start = buf.data() + d.mStartOffset;
      packets.emplace_back(start, start + d.mDataByteSize);
    }
    packet_index += num_packets;
  }

  status = AudioFileClose(file);
  if (status != noErr) {
    error = osstatus_message("AudioFileClose", status);
    return false;
  }

  if (packets.empty()) {
    error = "read zero Opus packets from " + path_str;
    return false;
  }

  error.clear();
  return true;
}

}  // namespace learning::audio::local_transport
