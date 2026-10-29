#include "render_pcm.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <algorithm>
#include <cstring>
#include <sstream>

namespace learning::audio::render {
namespace {

std::string osstatus_message(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " failed: " << status;
  return oss.str();
}

bool is_contract_pcm(const AudioStreamBasicDescription& asbd) {
  const auto rate = static_cast<std::uint32_t>(asbd.mSampleRate + 0.5);
  return asbd.mFormatID == kAudioFormatLinearPCM &&
         (asbd.mFormatFlags & kLinearPCMFormatFlagIsSignedInteger) != 0 &&
         (asbd.mFormatFlags & kLinearPCMFormatFlagIsFloat) == 0 &&
         (asbd.mFormatFlags & kLinearPCMFormatFlagIsBigEndian) == 0 &&
         rate == kSampleRateHz && asbd.mChannelsPerFrame == kChannels &&
         asbd.mBitsPerChannel == kBitsPerSample;
}

}  // namespace

bool load_render_wav(const std::filesystem::path& input,
                     std::vector<std::int16_t>& pcm, std::string& error) {
  pcm.clear();
  if (!std::filesystem::is_regular_file(input)) {
    error = "input WAV not found: " + input.string() +
            " (run learning_audio_decode first, or pass --input)";
    return false;
  }

  const std::string path_str = input.string();
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path_str.data()),
      static_cast<CFIndex>(path_str.size()), false);
  if (url == nullptr) {
    error = "CFURLCreateFromFileSystemRepresentation failed for " + path_str;
    return false;
  }

  AudioFileID file = nullptr;
  OSStatus status =
      AudioFileOpenURL(url, kAudioFileReadPermission, kAudioFileWAVEType, &file);
  CFRelease(url);
  if (status != noErr || file == nullptr) {
    error = osstatus_message("AudioFileOpenURL", status);
    return false;
  }

  AudioStreamBasicDescription asbd{};
  UInt32 prop_size = sizeof(asbd);
  status =
      AudioFileGetProperty(file, kAudioFilePropertyDataFormat, &prop_size, &asbd);
  if (status != noErr) {
    AudioFileClose(file);
    error = osstatus_message("AudioFileGetProperty(DataFormat)", status);
    return false;
  }
  if (!is_contract_pcm(asbd)) {
    AudioFileClose(file);
    std::ostringstream oss;
    oss << "WAV must be " << kSampleRateHz << " Hz mono s16le PCM (got rate="
        << asbd.mSampleRate << " ch=" << asbd.mChannelsPerFrame
        << " bits=" << asbd.mBitsPerChannel << ")";
    error = oss.str();
    return false;
  }

  UInt64 frame_count = 0;
  prop_size = sizeof(frame_count);
  status = AudioFileGetProperty(file, kAudioFilePropertyAudioDataPacketCount,
                                &prop_size, &frame_count);
  if (status != noErr) {
    AudioFileClose(file);
    error =
        osstatus_message("AudioFileGetProperty(AudioDataPacketCount)", status);
    return false;
  }
  if (frame_count == 0) {
    AudioFileClose(file);
    error = "WAV contains zero PCM frames: " + path_str;
    return false;
  }

  pcm.resize(static_cast<std::size_t>(frame_count));
  UInt32 num_frames = static_cast<UInt32>(frame_count);
  UInt32 num_bytes = num_frames * sizeof(std::int16_t);
  status = AudioFileReadPacketData(file, false, &num_bytes, nullptr, 0,
                                   &num_frames, pcm.data());
  AudioFileClose(file);
  if (status != noErr) {
    pcm.clear();
    error = osstatus_message("AudioFileReadPacketData", status);
    return false;
  }
  if (num_frames != frame_count) {
    pcm.clear();
    std::ostringstream oss;
    oss << "read " << num_frames << " of " << frame_count << " PCM frames";
    error = oss.str();
    return false;
  }

  error.clear();
  return true;
}

PcmCursor::PcmCursor(const std::vector<std::int16_t>& pcm) : pcm_(pcm) {}

std::size_t PcmCursor::fill(std::int16_t* dst, std::size_t max_frames) {
  const std::size_t frames = std::min(max_frames, pcm_.size() - position_);
  if (frames > 0) {
    std::memcpy(dst, pcm_.data() + position_, frames * sizeof(std::int16_t));
    position_ += frames;
  }
  return frames;
}

bool PcmCursor::done() const { return position_ == pcm_.size(); }

std::uint64_t PcmCursor::frames_consumed() const { return position_; }

}  // namespace learning::audio::render
