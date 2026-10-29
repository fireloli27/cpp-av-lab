#include "decode_wav.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <fstream>
#include <sstream>
#include <vector>

namespace learning::audio::decode {
namespace {

std::string osstatus_message(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " failed: " << status;
  return oss.str();
}

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

bool write_pcm_wav(const std::filesystem::path& path,
                   const std::vector<std::int16_t>& pcm, std::string& error) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  if (ec) {
    error = "failed to create " + path.parent_path().string() + ": " +
            ec.message();
    return false;
  }

  const std::uint32_t data_bytes =
      static_cast<std::uint32_t>(pcm.size() * sizeof(std::int16_t));
  const std::uint32_t riff_size = 36 + data_bytes;
  const std::uint32_t byte_rate =
      kSampleRateHz * kChannels * (kBitsPerSample / 8);
  const std::uint16_t block_align =
      static_cast<std::uint16_t>(kChannels * (kBitsPerSample / 8));

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    error = "failed to open " + path.string();
    return false;
  }

  out.write("RIFF", 4);
  write_le32(out, riff_size);
  out.write("WAVE", 4);
  out.write("fmt ", 4);
  write_le32(out, 16);
  write_le16(out, 1);
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
    error = "failed while writing " + path.string();
    return false;
  }
  out.close();
  if (out.fail()) {
    error = "failed while closing " + path.string();
    return false;
  }
  return true;
}

AudioStreamBasicDescription make_pcm_asbd() {
  AudioStreamBasicDescription asbd{};
  asbd.mSampleRate = kSampleRateHz;
  asbd.mFormatID = kAudioFormatLinearPCM;
  asbd.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
                      kLinearPCMFormatFlagIsPacked |
                      kAudioFormatFlagsNativeEndian;
  asbd.mFramesPerPacket = 1;
  asbd.mChannelsPerFrame = kChannels;
  asbd.mBitsPerChannel = kBitsPerSample;
  asbd.mBytesPerFrame =
      (asbd.mBitsPerChannel / 8) * asbd.mChannelsPerFrame;
  asbd.mBytesPerPacket = asbd.mBytesPerFrame;
  return asbd;
}

struct OpusCaf {
  AudioStreamBasicDescription asbd{};
  std::vector<std::uint8_t> cookie;
  std::vector<std::vector<std::uint8_t>> packets;
};

bool read_opus_caf(const std::filesystem::path& path, OpusCaf& caf,
                   std::string& error) {
  caf = {};
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

  UInt32 asbd_size = sizeof(caf.asbd);
  status = AudioFileGetProperty(file, kAudioFilePropertyDataFormat, &asbd_size,
                                &caf.asbd);
  if (status != noErr) {
    AudioFileClose(file);
    error = osstatus_message("AudioFileGetProperty(DataFormat)", status);
    return false;
  }
  if (caf.asbd.mFormatID != kAudioFormatOpus) {
    AudioFileClose(file);
    std::ostringstream oss;
    oss << "CAF is not Opus (mFormatID=0x" << std::hex << caf.asbd.mFormatID
        << ")";
    error = oss.str();
    return false;
  }
  const auto rate =
      static_cast<std::uint32_t>(caf.asbd.mSampleRate + 0.5);
  if (rate != kSampleRateHz || caf.asbd.mChannelsPerFrame != kChannels) {
    AudioFileClose(file);
    std::ostringstream oss;
    oss << "Opus CAF must be " << kSampleRateHz << " Hz mono (got rate=" << rate
        << " ch=" << caf.asbd.mChannelsPerFrame << ")";
    error = oss.str();
    return false;
  }
  if (caf.asbd.mFramesPerPacket == 0) {
    AudioFileClose(file);
    error = "Opus CAF frames_per_packet is 0";
    return false;
  }

  UInt32 cookie_size = 0;
  status = AudioFileGetPropertyInfo(file, kAudioFilePropertyMagicCookieData,
                                    &cookie_size, nullptr);
  if (status != noErr || cookie_size == 0) {
    AudioFileClose(file);
    error = "CAF has no Opus magic cookie";
    return false;
  }
  caf.cookie.resize(cookie_size);
  status = AudioFileGetProperty(file, kAudioFilePropertyMagicCookieData,
                                &cookie_size, caf.cookie.data());
  if (status != noErr) {
    AudioFileClose(file);
    error = osstatus_message("AudioFileGetProperty(MagicCookieData)", status);
    return false;
  }
  caf.cookie.resize(cookie_size);

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
      caf.packets.emplace_back(start, start + d.mDataByteSize);
    }
    packet_index += num_packets;
  }

  status = AudioFileClose(file);
  if (status != noErr) {
    error = osstatus_message("AudioFileClose", status);
    return false;
  }
  if (caf.packets.empty()) {
    error = "read zero Opus packets from " + path_str;
    return false;
  }
  return true;
}

struct PacketFeed {
  const std::vector<std::vector<std::uint8_t>>* packets = nullptr;
  std::size_t index = 0;
  AudioStreamPacketDescription desc{};
};

OSStatus feed_one_packet(AudioConverterRef /*converter*/, UInt32* io_packets,
                         AudioBufferList* io_data,
                         AudioStreamPacketDescription** out_packet_desc,
                         void* user) {
  auto* feed = static_cast<PacketFeed*>(user);
  if (feed->packets == nullptr || feed->index >= feed->packets->size() ||
      *io_packets == 0) {
    *io_packets = 0;
    return noErr;
  }

  const auto& packet = (*feed->packets)[feed->index];
  io_data->mNumberBuffers = 1;
  io_data->mBuffers[0].mNumberChannels = kChannels;
  io_data->mBuffers[0].mData =
      const_cast<std::uint8_t*>(packet.data());
  io_data->mBuffers[0].mDataByteSize = static_cast<UInt32>(packet.size());
  feed->desc.mStartOffset = 0;
  feed->desc.mVariableFramesInPacket = 0;
  feed->desc.mDataByteSize = static_cast<UInt32>(packet.size());
  if (out_packet_desc != nullptr) {
    *out_packet_desc = &feed->desc;
  }
  *io_packets = 1;
  feed->index += 1;
  return noErr;
}

bool pcm_in_window(std::uint64_t pcm_frames, std::uint64_t source_frames,
                   std::string& error) {
  if (pcm_frames == 0 || pcm_frames < source_frames / 2 ||
      pcm_frames > source_frames * 2) {
    std::ostringstream oss;
    oss << "decoded pcm_frames=" << pcm_frames
        << " outside window of source_frames=" << source_frames
        << " (1/2..2x, no priming trim)";
    error = oss.str();
    return false;
  }
  return true;
}

}  // namespace

bool write_decode_wav(const std::filesystem::path& root,
                      const std::filesystem::path& input_caf, DecodeStats& stats,
                      std::string& error) {
  stats = {};
  OpusCaf caf;
  if (!read_opus_caf(input_caf, caf, error)) {
    return false;
  }

  const AudioStreamBasicDescription pcm_asbd = make_pcm_asbd();
  AudioConverterRef converter = nullptr;
  OSStatus status = AudioConverterNew(&caf.asbd, &pcm_asbd, &converter);
  if (status != noErr || converter == nullptr) {
    error = osstatus_message("AudioConverterNew", status);
    return false;
  }

  status = AudioConverterSetProperty(converter,
                                     kAudioConverterDecompressionMagicCookie,
                                     static_cast<UInt32>(caf.cookie.size()),
                                     caf.cookie.data());
  if (status != noErr) {
    AudioConverterDispose(converter);
    error = osstatus_message(
        "AudioConverterSetProperty(DecompressionMagicCookie)", status);
    return false;
  }

  PacketFeed feed;
  feed.packets = &caf.packets;
  const std::uint64_t source_frames =
      static_cast<std::uint64_t>(caf.packets.size()) *
      caf.asbd.mFramesPerPacket;
  constexpr UInt32 kFramesPerPull = 4096;
  std::vector<std::int16_t> pull(kFramesPerPull);
  std::vector<std::int16_t> pcm;
  pcm.reserve(static_cast<std::size_t>(source_frames));

  while (true) {
    AudioBufferList abl{};
    abl.mNumberBuffers = 1;
    abl.mBuffers[0].mNumberChannels = kChannels;
    abl.mBuffers[0].mData = pull.data();
    abl.mBuffers[0].mDataByteSize =
        static_cast<UInt32>(pull.size() * sizeof(std::int16_t));
    UInt32 frames = kFramesPerPull;
    status = AudioConverterFillComplexBuffer(converter, feed_one_packet, &feed,
                                             &frames, &abl, nullptr);
    if (status != noErr) {
      AudioConverterDispose(converter);
      error = osstatus_message("AudioConverterFillComplexBuffer", status);
      return false;
    }
    if (frames == 0) {
      break;
    }
    pcm.insert(pcm.end(), pull.begin(),
               pull.begin() + static_cast<std::ptrdiff_t>(frames));
    if (pcm.size() > source_frames * 2) {
      break;
    }
  }
  AudioConverterDispose(converter);

  if (feed.index != caf.packets.size()) {
    std::ostringstream oss;
    oss << "converter stopped after " << feed.index << " of "
        << caf.packets.size() << " access units";
    error = oss.str();
    return false;
  }

  if (!pcm_in_window(pcm.size(), source_frames, error)) {
    return false;
  }

  const auto out_path = root / kDecodeRelativePath;
  if (!write_pcm_wav(out_path, pcm, error)) {
    return false;
  }

  stats.packets = caf.packets.size();
  stats.pcm_frames = pcm.size();
  stats.frames_per_packet = caf.asbd.mFramesPerPacket;
  error.clear();
  return true;
}

}  // namespace learning::audio::decode
