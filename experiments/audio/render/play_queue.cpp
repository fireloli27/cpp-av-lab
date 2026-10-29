#include "play_queue.hpp"

#include "render_pcm.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <chrono>
#include <sstream>

namespace learning::audio::render {
namespace {

std::string osstatus_message(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " failed: " << status;
  return oss.str();
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
  asbd.mBytesPerFrame = (asbd.mBitsPerChannel / 8) * asbd.mChannelsPerFrame;
  asbd.mBytesPerPacket = asbd.mBytesPerFrame;
  return asbd;
}

// Callbacks run on this thread's run loop, so no locking is needed.
struct PlayerState {
  explicit PlayerState(const std::vector<std::int16_t>& pcm) : cursor(pcm) {}
  PcmCursor cursor;
  std::uint64_t frames_enqueued = 0;
  bool stop_requested = false;
  OSStatus enqueue_status = noErr;
};

void request_stop(PlayerState* state, AudioQueueRef queue) {
  if (!state->stop_requested) {
    state->stop_requested = true;
    AudioQueueStop(queue, false);
  }
}

// Returns false when nothing was enqueued (cursor exhausted or enqueue failed).
bool refill(PlayerState* state, AudioQueueRef queue, AudioQueueBufferRef buffer) {
  const std::size_t frames = state->cursor.fill(
      static_cast<std::int16_t*>(buffer->mAudioData), kBufferFrames);
  if (frames == 0) {
    return false;
  }
  buffer->mAudioDataByteSize =
      static_cast<UInt32>(frames * sizeof(std::int16_t));
  const OSStatus status = AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
  if (status != noErr) {
    state->enqueue_status = status;
    return false;
  }
  state->frames_enqueued += frames;
  return true;
}

void output_callback(void* user_data, AudioQueueRef queue,
                     AudioQueueBufferRef buffer) {
  auto* state = static_cast<PlayerState*>(user_data);
  if (state->stop_requested) {
    return;
  }
  if (!refill(state, queue, buffer)) {
    request_stop(state, queue);
  }
}

bool queue_is_running(AudioQueueRef queue) {
  UInt32 running = 0;
  UInt32 size = sizeof(running);
  if (AudioQueueGetProperty(queue, kAudioQueueProperty_IsRunning, &running,
                            &size) != noErr) {
    return false;
  }
  return running != 0;
}

}  // namespace

bool play_pcm(const std::vector<std::int16_t>& pcm, RenderStats& stats,
              std::string& error) {
  stats = {};
  stats.pcm_frames = pcm.size();
  stats.buffers = kNumBuffers;
  stats.buffer_frames = kBufferFrames;
  if (pcm.empty()) {
    error = "no PCM frames to render";
    return false;
  }

  PlayerState state(pcm);
  const AudioStreamBasicDescription asbd = make_pcm_asbd();
  AudioQueueRef queue = nullptr;
  OSStatus status =
      AudioQueueNewOutput(&asbd, output_callback, &state, CFRunLoopGetCurrent(),
                          kCFRunLoopCommonModes, 0, &queue);
  if (status != noErr || queue == nullptr) {
    error = osstatus_message("AudioQueueNewOutput", status) +
            " (check default output device)";
    return false;
  }

  const UInt32 buffer_bytes = kBufferFrames * sizeof(std::int16_t);
  for (std::uint32_t i = 0; i < kNumBuffers; ++i) {
    AudioQueueBufferRef buffer = nullptr;
    status = AudioQueueAllocateBuffer(queue, buffer_bytes, &buffer);
    if (status != noErr) {
      AudioQueueDispose(queue, true);
      error = osstatus_message("AudioQueueAllocateBuffer", status);
      return false;
    }
    if (!refill(&state, queue, buffer)) {
      break;
    }
  }
  if (state.enqueue_status != noErr) {
    AudioQueueDispose(queue, true);
    error = osstatus_message("AudioQueueEnqueueBuffer", state.enqueue_status);
    return false;
  }

  status = AudioQueueStart(queue, nullptr);
  if (status != noErr) {
    AudioQueueDispose(queue, true);
    error = osstatus_message("AudioQueueStart", status) +
            " (check default output device)";
    return false;
  }
  if (state.cursor.done()) {
    request_stop(&state, queue);
  }

  // Generous deadline so a wedged device fails instead of hanging the CLI.
  const auto deadline =
      std::chrono::steady_clock::now() +
      std::chrono::milliseconds(pcm.size() * 1000 / kSampleRateHz + 5000);
  // IsRunning flips to 1 asynchronously after AudioQueueStart; a short input can
  // request stop before that, so wait to see it start before waiting for 0.
  bool seen_running = false;
  while (true) {
    const bool running = queue_is_running(queue);
    seen_running = seen_running || running;
    if (state.stop_requested && seen_running && !running) {
      break;
    }
    if (std::chrono::steady_clock::now() > deadline) {
      AudioQueueDispose(queue, true);
      error = "playback did not drain before deadline";
      return false;
    }
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
  }
  AudioQueueDispose(queue, true);

  if (state.enqueue_status != noErr) {
    error = osstatus_message("AudioQueueEnqueueBuffer", state.enqueue_status);
    return false;
  }
  stats.frames_rendered = state.frames_enqueued;
  if (stats.frames_rendered != stats.pcm_frames) {
    std::ostringstream oss;
    oss << "frames_rendered=" << stats.frames_rendered
        << " != pcm_frames=" << stats.pcm_frames;
    error = oss.str();
    return false;
  }
  error.clear();
  return true;
}

}  // namespace learning::audio::render
