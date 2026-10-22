#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace learning::audio::local_transport {

struct LoopbackStats {
  std::uint64_t units_sent = 0;
  std::uint64_t units_recv = 0;
  std::uint64_t payload_bytes_sent = 0;
  std::uint64_t payload_bytes_recv = 0;
  // Ephemeral port chosen by bind(..., 0); recorded for notes / artifact.
  int bound_port = 0;
};

// Single-process POSIX UDP on 127.0.0.1: bind ephemeral port, send each payload
// as a Transport Unit to self, recv and verify byte-identical payloads in order.
bool udp_loopback_roundtrip(
    const std::vector<std::vector<std::uint8_t>>& payloads, LoopbackStats& stats,
    std::string& error);

}  // namespace learning::audio::local_transport
