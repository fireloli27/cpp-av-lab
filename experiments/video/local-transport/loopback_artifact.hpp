#pragma once

#include "udp_loopback.hpp"

#include <filesystem>
#include <string>

namespace learning::video::local_transport {

inline constexpr const char* kLoopbackOkRelativePath =
    "out/video/local-transport/loopback-ok.txt";
inline constexpr const char* kDefaultInputRelativePath =
    "out/video/encode/encode.vau";

// Writes the success artifact under `root`. Contents are the SSOT in
// docs/learning/video/local-transport.notes.md.
bool write_loopback_ok_artifact(const std::filesystem::path& root,
                                const LoopbackStats& stats, std::string& error);

}  // namespace learning::video::local_transport
