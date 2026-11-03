#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace learning::video::local_transport {

// One Transport Unit is one H.264 access unit. A video access unit can exceed
// a single UDP datagram, so the learning experiment fragments that same unit.
// This is not RTP: no timestamp, no SSRC. au_index is only a reassembly key.
// Wire layout (big-endian):
//   uint32 au_length | uint16 frag_index | uint16 frag_count | uint32 au_index
//   | fragment bytes
// SSOT: docs/learning/video/local-transport.notes.md

inline constexpr std::size_t kFragPayloadMax = 1200;
inline constexpr std::size_t kDatagramHeaderBytes = 12;
inline constexpr std::size_t kMaxAccessUnitBytes = 4u * 1024u * 1024u;

bool split_access_unit(std::uint32_t au_index,
                       const std::vector<std::uint8_t>& payload,
                       std::vector<std::vector<std::uint8_t>>& datagrams,
                       std::string& error);

// Rebuild one access unit from the datagrams of that unit (any arrival order).
// expected_au_index must match every fragment.
bool reassemble_access_unit(
    const std::vector<std::vector<std::uint8_t>>& datagrams,
    std::uint32_t expected_au_index, std::vector<std::uint8_t>& payload,
    std::string& error);

}  // namespace learning::video::local_transport
