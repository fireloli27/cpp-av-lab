#include "transport_unit.hpp"

#include <algorithm>

namespace learning::video::local_transport {
namespace {

void append_be16(std::vector<std::uint8_t>& out, std::uint16_t v) {
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void append_be32(std::vector<std::uint8_t>& out, std::uint32_t v) {
  out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

std::uint16_t load_be16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(p[0]) << 8) |
                                   p[1]);
}

std::uint32_t load_be32(const std::uint8_t* p) {
  return (static_cast<std::uint32_t>(p[0]) << 24) |
         (static_cast<std::uint32_t>(p[1]) << 16) |
         (static_cast<std::uint32_t>(p[2]) << 8) |
         static_cast<std::uint32_t>(p[3]);
}

struct ParsedFragment {
  std::uint32_t au_length = 0;
  std::uint16_t frag_index = 0;
  std::uint16_t frag_count = 0;
  std::uint32_t au_index = 0;
  std::vector<std::uint8_t> payload;
};

bool parse_datagram(const std::vector<std::uint8_t>& datagram,
                    ParsedFragment& out, std::string& error) {
  if (datagram.size() < kDatagramHeaderBytes) {
    error = "datagram shorter than transport header";
    return false;
  }
  if (datagram.size() > kDatagramHeaderBytes + kFragPayloadMax) {
    error = "datagram fragment exceeds 1200 payload bytes";
    return false;
  }
  out.au_length = load_be32(datagram.data());
  out.frag_index = load_be16(datagram.data() + 4);
  out.frag_count = load_be16(datagram.data() + 6);
  out.au_index = load_be32(datagram.data() + 8);
  if (out.au_length == 0 || out.au_length > kMaxAccessUnitBytes) {
    error = "au_length out of range";
    return false;
  }
  if (out.frag_count == 0 || out.frag_index >= out.frag_count) {
    error = "fragment index out of range";
    return false;
  }
  const std::size_t payload_size = datagram.size() - kDatagramHeaderBytes;
  if (payload_size == 0) {
    error = "fragment payload is empty";
    return false;
  }
  out.payload.assign(datagram.begin() + static_cast<std::ptrdiff_t>(kDatagramHeaderBytes),
                     datagram.end());
  return true;
}

std::size_t expected_frag_size(std::uint32_t au_length, std::uint16_t frag_index,
                               std::uint16_t frag_count) {
  if (frag_index + 1 == frag_count) {
    return static_cast<std::size_t>(au_length) -
           kFragPayloadMax * static_cast<std::size_t>(frag_count - 1);
  }
  return kFragPayloadMax;
}

}  // namespace

bool split_access_unit(std::uint32_t au_index,
                       const std::vector<std::uint8_t>& payload,
                       std::vector<std::vector<std::uint8_t>>& datagrams,
                       std::string& error) {
  datagrams.clear();
  if (payload.empty() || payload.size() > kMaxAccessUnitBytes) {
    error = "access unit size out of range";
    return false;
  }
  const std::size_t count =
      (payload.size() + kFragPayloadMax - 1) / kFragPayloadMax;
  if (count > 65535) {
    error = "access unit needs too many fragments";
    return false;
  }
  datagrams.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    const std::size_t offset = i * kFragPayloadMax;
    const std::size_t n =
        std::min(kFragPayloadMax, payload.size() - offset);
    std::vector<std::uint8_t> datagram;
    datagram.reserve(kDatagramHeaderBytes + n);
    append_be32(datagram, static_cast<std::uint32_t>(payload.size()));
    append_be16(datagram, static_cast<std::uint16_t>(i));
    append_be16(datagram, static_cast<std::uint16_t>(count));
    append_be32(datagram, au_index);
    datagram.insert(datagram.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset),
                    payload.begin() + static_cast<std::ptrdiff_t>(offset + n));
    datagrams.push_back(std::move(datagram));
  }
  error.clear();
  return true;
}

bool reassemble_access_unit(
    const std::vector<std::vector<std::uint8_t>>& datagrams,
    std::uint32_t expected_au_index, std::vector<std::uint8_t>& payload,
    std::string& error) {
  payload.clear();
  if (datagrams.empty()) {
    error = "no datagrams to reassemble";
    return false;
  }
  std::vector<ParsedFragment> parts;
  parts.reserve(datagrams.size());
  for (const auto& datagram : datagrams) {
    ParsedFragment part;
    if (!parse_datagram(datagram, part, error)) {
      return false;
    }
    parts.push_back(std::move(part));
  }
  const std::uint32_t au_length = parts.front().au_length;
  const std::uint16_t frag_count = parts.front().frag_count;
  if (parts.front().au_index != expected_au_index) {
    error = "fragment au_index does not match the access unit";
    return false;
  }
  if (parts.size() != frag_count) {
    error = "fragment count does not match datagrams received";
    return false;
  }
  std::vector<std::vector<std::uint8_t>> slots(frag_count);
  std::vector<std::uint8_t> got(frag_count, 0);
  for (auto& part : parts) {
    if (part.au_index != expected_au_index || part.au_length != au_length ||
        part.frag_count != frag_count) {
      error = "fragments disagree on access unit identity";
      return false;
    }
    if (got[part.frag_index] != 0) {
      error = "duplicate fragment index";
      return false;
    }
    const std::size_t expect =
        expected_frag_size(au_length, part.frag_index, frag_count);
    if (part.payload.size() != expect) {
      error = "fragment payload size does not match au_length";
      return false;
    }
    slots[part.frag_index] = std::move(part.payload);
    got[part.frag_index] = 1;
  }
  payload.reserve(au_length);
  for (const auto& slot : slots) {
    payload.insert(payload.end(), slot.begin(), slot.end());
  }
  if (payload.size() != au_length) {
    error = "reassembled size does not match au_length";
    return false;
  }
  error.clear();
  return true;
}

}  // namespace learning::video::local_transport
