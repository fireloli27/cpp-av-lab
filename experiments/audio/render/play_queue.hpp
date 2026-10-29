#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace learning::audio::render {

struct RenderStats {
  std::uint64_t pcm_frames = 0;
  std::uint64_t frames_rendered = 0;
  std::uint32_t buffers = 0;
  std::uint32_t buffer_frames = 0;
};

// Plays 48 kHz / mono / s16le PCM on the default output device with
// AudioQueueNewOutput (kNumBuffers x kBufferFrames). Blocks until the queue has
// drained (AudioQueueStop non-immediate, then kAudioQueueProperty_IsRunning == 0).
bool play_pcm(const std::vector<std::int16_t>& pcm, RenderStats& stats,
              std::string& error);

}  // namespace learning::audio::render
