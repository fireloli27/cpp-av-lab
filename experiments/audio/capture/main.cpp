#include "capture_wav.hpp"

#include <AudioToolbox/AudioToolbox.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr double kDefaultSeconds = 3.0;
constexpr double kMinSeconds = 0.0;   // exclusive lower bound checked as > 0
constexpr double kMaxSeconds = 60.0;

struct RecorderState {
  std::mutex mu;
  std::vector<std::int16_t> pcm;
  std::atomic<bool> failed{false};
  char error[256]{};
};

void set_error(RecorderState* state, const char* msg) {
  state->failed = true;
  std::snprintf(state->error, sizeof(state->error), "%s", msg);
}

void input_callback(void* user_data, AudioQueueRef queue,
                    AudioQueueBufferRef buffer,
                    const AudioTimeStamp* /*start_time*/,
                    UInt32 /*num_packets*/,
                    const AudioStreamPacketDescription* /*packet_descs*/) {
  auto* state = static_cast<RecorderState*>(user_data);
  if (state->failed.load()) {
    return;
  }

  const auto bytes = static_cast<std::size_t>(buffer->mAudioDataByteSize);
  if (bytes > 0) {
    if (bytes % sizeof(std::int16_t) != 0) {
      set_error(state, "AudioQueue delivered a non-s16le buffer size");
      return;
    }
    const auto* samples = static_cast<const std::int16_t*>(buffer->mAudioData);
    const auto count = bytes / sizeof(std::int16_t);
    std::lock_guard<std::mutex> lock(state->mu);
    state->pcm.insert(state->pcm.end(), samples, samples + count);
  }

  if (!state->failed.load()) {
    AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
  }
}

bool parse_seconds(int argc, char** argv, double& seconds, std::string& error) {
  seconds = kDefaultSeconds;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--seconds") {
      if (i + 1 >= argc) {
        error = "--seconds requires a value";
        return false;
      }
      char* end = nullptr;
      const double v = std::strtod(argv[++i], &end);
      if (end == argv[i] || *end != '\0') {
        error = "--seconds must be a number";
        return false;
      }
      if (!(v > kMinSeconds && v <= kMaxSeconds)) {
        error = "--seconds must be in (0, 60]";
        return false;
      }
      seconds = v;
    } else {
      error = "unknown argument: " + arg;
      return false;
    }
  }
  return true;
}

AudioStreamBasicDescription make_asbd() {
  AudioStreamBasicDescription asbd{};
  asbd.mSampleRate = learning::audio::capture::kSampleRateHz;
  asbd.mFormatID = kAudioFormatLinearPCM;
  // Native endian on Apple Silicon / Intel Mac = little-endian ⇒ s16le.
  asbd.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
                      kLinearPCMFormatFlagIsPacked |
                      kAudioFormatFlagsNativeEndian;
  asbd.mFramesPerPacket = 1;
  asbd.mChannelsPerFrame = learning::audio::capture::kChannels;
  asbd.mBitsPerChannel = learning::audio::capture::kBitsPerSample;
  asbd.mBytesPerFrame =
      (asbd.mBitsPerChannel / 8) * asbd.mChannelsPerFrame;
  asbd.mBytesPerPacket = asbd.mBytesPerFrame;
  return asbd;
}

}  // namespace

int main(int argc, char** argv) {
  double seconds = kDefaultSeconds;
  std::string error;
  if (!parse_seconds(argc, argv, seconds, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  RecorderState state;
  AudioStreamBasicDescription asbd = make_asbd();
  AudioQueueRef queue = nullptr;
  // NULL run loop ⇒ Audio Queue Services invokes the callback on its own thread.
  OSStatus status =
      AudioQueueNewInput(&asbd, input_callback, &state, nullptr, nullptr, 0,
                         &queue);
  if (status != noErr || queue == nullptr) {
    std::cerr << "AudioQueueNewInput failed: " << status
              << " (check Terminal microphone permission)\n";
    return 1;
  }

  AudioStreamBasicDescription actual{};
  UInt32 actual_size = sizeof(actual);
  status = AudioQueueGetProperty(queue, kAudioQueueProperty_StreamDescription,
                                 &actual, &actual_size);
  if (status != noErr) {
    std::cerr << "AudioQueueGetProperty(StreamDescription) failed: " << status
              << '\n';
    AudioQueueDispose(queue, true);
    return 1;
  }
  if (actual.mSampleRate != asbd.mSampleRate ||
      actual.mChannelsPerFrame != asbd.mChannelsPerFrame ||
      actual.mBitsPerChannel != asbd.mBitsPerChannel ||
      actual.mFormatID != kAudioFormatLinearPCM) {
    std::cerr << "AudioQueue did not honor 48 kHz mono s16le ASBD "
                 "(got rate="
              << actual.mSampleRate << " ch=" << actual.mChannelsPerFrame
              << " bits=" << actual.mBitsPerChannel << ")\n";
    AudioQueueDispose(queue, true);
    return 1;
  }

  constexpr int kNumBuffers = 3;
  constexpr UInt32 kBufferBytes = 4096;
  AudioQueueBufferRef buffers[kNumBuffers]{};
  for (int i = 0; i < kNumBuffers; ++i) {
    status = AudioQueueAllocateBuffer(queue, kBufferBytes, &buffers[i]);
    if (status != noErr) {
      std::cerr << "AudioQueueAllocateBuffer failed: " << status << '\n';
      AudioQueueDispose(queue, true);
      return 1;
    }
    status = AudioQueueEnqueueBuffer(queue, buffers[i], 0, nullptr);
    if (status != noErr) {
      std::cerr << "AudioQueueEnqueueBuffer failed: " << status << '\n';
      AudioQueueDispose(queue, true);
      return 1;
    }
  }

  status = AudioQueueStart(queue, nullptr);
  if (status != noErr) {
    std::cerr << "AudioQueueStart failed: " << status
              << " (check Terminal microphone permission / default input device)\n";
    AudioQueueDispose(queue, true);
    return 1;
  }

  std::cout << "recording " << seconds << "s at "
            << learning::audio::capture::kSampleRateHz << " Hz mono s16le\n";
  std::this_thread::sleep_for(
      std::chrono::duration<double>(seconds));

  AudioQueueStop(queue, true);
  AudioQueueDispose(queue, true);

  if (state.failed.load()) {
    std::cerr << state.error << '\n';
    return 1;
  }

  std::vector<std::int16_t> pcm;
  {
    std::lock_guard<std::mutex> lock(state.mu);
    pcm.swap(state.pcm);
  }

  if (pcm.empty()) {
    std::cerr << "no PCM captured (empty buffers)\n";
    return 1;
  }

  if (!learning::audio::capture::write_capture_wav(".", pcm, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  const double captured_s =
      static_cast<double>(pcm.size()) /
      static_cast<double>(learning::audio::capture::kSampleRateHz);
  std::cout << "wrote " << learning::audio::capture::kCaptureRelativePath
            << " samples=" << pcm.size() << " duration_s≈" << captured_s
            << " rate=" << learning::audio::capture::kSampleRateHz
            << " channels=" << learning::audio::capture::kChannels
            << " bits=" << learning::audio::capture::kBitsPerSample << '\n';
  return 0;
}
