#pragma once

#include "udp_loopback.hpp"

#include <filesystem>
#include <string>

namespace learning::audio::local_transport {

inline constexpr const char* kLoopbackOkRelativePath =
    "out/audio/local-transport/loopback-ok.txt";

// Writes success artifact under `root` (cwd = repo root layout). Contents SSOT
// lives in docs/learning/audio/local-transport.notes.md.
bool write_loopback_ok_artifact(const std::filesystem::path& root,
                                const LoopbackStats& stats, std::string& error);

}  // namespace learning::audio::local_transport
