#include "encode_caf.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <algorithm>
#include <sstream>
#include <vector>

namespace learning::audio::encode {
namespace {

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

bool make_opus_asbd(AudioStreamBasicDescription& out, std::string& error) {
  out = {};
  out.mFormatID = kAudioFormatOpus;
  out.mSampleRate = kSampleRateHz;
  out.mChannelsPerFrame = kChannels;
  UInt32 size = sizeof(out);
  const OSStatus status =
      AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, nullptr, &size,
                             &out);
  if (status != noErr) {
    std::ostringstream oss;
    oss << "AudioFormatGetProperty(FormatInfo, Opus) failed: " << status;
    error = oss.str();
    return false;
  }
  return true;
}

std::string osstatus_message(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " failed: " << status;
  return oss.str();
}

struct InputContext {
  const std::int16_t* data = nullptr;
  UInt32 frames_remaining = 0;
};

OSStatus input_proc(AudioConverterRef /*converter*/, UInt32* io_packets,
                    AudioBufferList* io_data,
                    AudioStreamPacketDescription** out_packet_desc,
                    void* user) {
  auto* ctx = static_cast<InputContext*>(user);
  if (out_packet_desc != nullptr) {
    *out_packet_desc = nullptr;
  }
  if (ctx->frames_remaining == 0) {
    *io_packets = 0;
    return noErr;
  }

  const UInt32 frames = std::min(*io_packets, ctx->frames_remaining);
  io_data->mNumberBuffers = 1;
  io_data->mBuffers[0].mNumberChannels = kChannels;
  io_data->mBuffers[0].mData =
      const_cast<std::int16_t*>(ctx->data);
  io_data->mBuffers[0].mDataByteSize =
      frames * static_cast<UInt32>(sizeof(std::int16_t)) * kChannels;
  ctx->data += frames * kChannels;
  ctx->frames_remaining -= frames;
  *io_packets = frames;
  return noErr;
}

}  // namespace

bool write_encode_caf(const std::filesystem::path& root,
                      const std::vector<std::int16_t>& pcm, EncodeStats& stats,
                      std::string& error) {
  namespace fs = std::filesystem;
  stats = {};

  if (pcm.empty()) {
    error = "no PCM to encode";
    return false;
  }

  const fs::path out_path = root / kEncodeRelativePath;
  std::error_code ec;
  fs::create_directories(out_path.parent_path(), ec);
  if (ec) {
    error = "failed to create " + out_path.parent_path().string() + ": " +
            ec.message();
    return false;
  }

  const AudioStreamBasicDescription in_asbd = make_pcm_asbd();
  AudioStreamBasicDescription opus_asbd{};
  if (!make_opus_asbd(opus_asbd, error)) {
    return false;
  }

  AudioConverterRef converter = nullptr;
  OSStatus status = AudioConverterNew(&in_asbd, &opus_asbd, &converter);
  if (status != noErr || converter == nullptr) {
    error = osstatus_message("AudioConverterNew", status);
    return false;
  }

  UInt32 bit_rate = kBitRateBps;
  status = AudioConverterSetProperty(converter, kAudioConverterEncodeBitRate,
                                     sizeof(bit_rate), &bit_rate);
  if (status != noErr) {
    AudioConverterDispose(converter);
    error = osstatus_message("AudioConverterSetProperty(EncodeBitRate)", status);
    return false;
  }

  // Refresh opus ASBD from converter (frames/packet, etc.).
  UInt32 asbd_size = sizeof(opus_asbd);
  status = AudioConverterGetProperty(converter,
                                     kAudioConverterCurrentOutputStreamDescription,
                                     &asbd_size, &opus_asbd);
  if (status != noErr) {
    AudioConverterDispose(converter);
    error = osstatus_message(
        "AudioConverterGetProperty(CurrentOutputStreamDescription)", status);
    return false;
  }

  const std::string path_str = out_path.string();
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path_str.data()),
      static_cast<CFIndex>(path_str.size()), false);
  if (url == nullptr) {
    AudioConverterDispose(converter);
    error = "CFURLCreateFromFileSystemRepresentation failed for " + path_str;
    return false;
  }

  AudioFileID file = nullptr;
  status = AudioFileCreateWithURL(
      url, kAudioFileCAFType, &opus_asbd,
      kAudioFileFlags_EraseFile | kAudioFileFlags_DontPageAlignAudioData, &file);
  CFRelease(url);
  if (status != noErr || file == nullptr) {
    AudioConverterDispose(converter);
    error = osstatus_message("AudioFileCreateWithURL", status);
    return false;
  }

  UInt32 cookie_size = 0;
  Boolean writable = false;
  status = AudioConverterGetPropertyInfo(
      converter, kAudioConverterCompressionMagicCookie, &cookie_size, &writable);
  if (status == noErr && cookie_size > 0) {
    std::vector<std::uint8_t> cookie(cookie_size);
    status = AudioConverterGetProperty(converter,
                                       kAudioConverterCompressionMagicCookie,
                                       &cookie_size, cookie.data());
    if (status != noErr) {
      AudioFileClose(file);
      AudioConverterDispose(converter);
      error = osstatus_message(
          "AudioConverterGetProperty(CompressionMagicCookie)", status);
      return false;
    }
    status = AudioFileSetProperty(file, kAudioFilePropertyMagicCookieData,
                                  cookie_size, cookie.data());
    if (status != noErr) {
      AudioFileClose(file);
      AudioConverterDispose(converter);
      error =
          osstatus_message("AudioFileSetProperty(MagicCookieData)", status);
      return false;
    }
  }

  UInt32 max_packet_bytes = 0;
  UInt32 prop_size = sizeof(max_packet_bytes);
  status = AudioConverterGetProperty(
      converter, kAudioConverterPropertyMaximumOutputPacketSize, &prop_size,
      &max_packet_bytes);
  if (status != noErr || max_packet_bytes == 0) {
    AudioFileClose(file);
    AudioConverterDispose(converter);
    error = osstatus_message(
        "AudioConverterGetProperty(MaximumOutputPacketSize)", status);
    return false;
  }

  constexpr UInt32 kPacketsPerPull = 32;
  std::vector<std::uint8_t> packet_buf(
      static_cast<std::size_t>(max_packet_bytes) * kPacketsPerPull);
  std::vector<AudioStreamPacketDescription> packet_descs(kPacketsPerPull);

  InputContext input_ctx;
  input_ctx.data = pcm.data();
  input_ctx.frames_remaining = static_cast<UInt32>(pcm.size() / kChannels);

  UInt64 total_packets = 0;
  SInt64 packet_index = 0;

  while (true) {
    AudioBufferList abl{};
    abl.mNumberBuffers = 1;
    abl.mBuffers[0].mNumberChannels = kChannels;
    abl.mBuffers[0].mData = packet_buf.data();
    abl.mBuffers[0].mDataByteSize =
        static_cast<UInt32>(packet_buf.size());

    UInt32 packets = kPacketsPerPull;
    status = AudioConverterFillComplexBuffer(converter, input_proc, &input_ctx,
                                             &packets, &abl, packet_descs.data());
    if (status != noErr) {
      AudioFileClose(file);
      AudioConverterDispose(converter);
      error = osstatus_message("AudioConverterFillComplexBuffer", status);
      return false;
    }
    if (packets == 0) {
      break;
    }

    status = AudioFileWritePackets(file, false, abl.mBuffers[0].mDataByteSize,
                                   packet_descs.data(), packet_index, &packets,
                                   packet_buf.data());
    if (status != noErr) {
      AudioFileClose(file);
      AudioConverterDispose(converter);
      error = osstatus_message("AudioFileWritePackets", status);
      return false;
    }
    packet_index += packets;
    total_packets += packets;
  }

  AudioConverterDispose(converter);

  status = AudioFileOptimize(file);
  if (status != noErr) {
    AudioFileClose(file);
    error = osstatus_message("AudioFileOptimize", status);
    return false;
  }

  status = AudioFileClose(file);
  if (status != noErr) {
    error = osstatus_message("AudioFileClose", status);
    return false;
  }

  if (!fs::is_regular_file(out_path)) {
    error = "encode wrote no file at " + out_path.string();
    return false;
  }

  stats.packet_count = total_packets;
  stats.output_bytes = static_cast<std::uint64_t>(fs::file_size(out_path));
  stats.duration_s = static_cast<double>(pcm.size()) /
                     static_cast<double>(kSampleRateHz * kChannels);
  stats.frames_per_packet = opus_asbd.mFramesPerPacket;

  error.clear();
  return true;
}

}  // namespace learning::audio::encode
