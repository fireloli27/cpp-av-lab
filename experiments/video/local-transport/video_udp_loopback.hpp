#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace learning::video::local_transport {

struct LoopbackStats {
  std::uint64_t units_sent = 0;
  std::uint64_t units_recv = 0;
  std::uint64_t payload_bytes_sent = 0;
  std::uint64_t payload_bytes_recv = 0;
  std::uint64_t fragments_sent = 0;
  std::uint64_t fragments_recv = 0;
  int bound_port = 0;
};

// Single-process POSIX UDP on 127.0.0.1. Binds an ephemeral port, sends every
// access unit as fragmented datagrams to self, receives those datagrams, and
// checks the reassembled payload is byte-identical. No ICE/TURN/SRTP.
bool udp_loopback_roundtrip(
    const std::vector<std::vector<std::uint8_t>>& access_units,
    LoopbackStats& stats, std::string& error);

}  // namespace learning::video::local_transport
