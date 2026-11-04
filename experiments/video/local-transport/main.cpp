#include "video_loopback_artifact.hpp"
#include "video_transport_unit.hpp"
#include "video_udp_loopback.hpp"
#include "vau.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace lt = learning::video::local_transport;
namespace enc = learning::video::encode;
namespace fs = std::filesystem;

bool parse_args(int argc, char** argv, fs::path& input, std::string& error) {
  input = lt::kDefaultInputRelativePath;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--input") {
      if (i + 1 >= argc) {
        error = "--input requires a path";
        return false;
      }
      input = argv[++i];
    } else {
      error = "unknown argument: " + arg;
      return false;
    }
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  fs::path input;
  std::string error;
  if (!parse_args(argc, argv, input, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  enc::VauFile file;
  if (!enc::read_vau(input, file, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::vector<std::vector<std::uint8_t>> payloads;
  payloads.reserve(file.access_units.size());
  for (const auto& unit : file.access_units) {
    payloads.push_back(unit.payload);
  }

  lt::LoopbackStats stats;
  if (!lt::udp_loopback_roundtrip(payloads, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  if (!lt::write_loopback_ok_artifact(".", stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  std::cout << "local-transport loopback ok"
            << " input=" << input.string()
            << " units_sent=" << stats.units_sent
            << " units_recv=" << stats.units_recv
            << " payload_bytes_sent=" << stats.payload_bytes_sent
            << " payload_bytes_recv=" << stats.payload_bytes_recv
            << " fragments_sent=" << stats.fragments_sent
            << " fragments_recv=" << stats.fragments_recv
            << " frag_payload_max=" << lt::kFragPayloadMax
            << " port=" << stats.bound_port
            << " artifact=" << lt::kLoopbackOkRelativePath << '\n';
  return 0;
}
