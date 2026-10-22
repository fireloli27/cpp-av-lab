#include "loopback_artifact.hpp"
#include "read_opus_caf.hpp"
#include "udp_loopback.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace lt = learning::audio::local_transport;
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

  std::vector<std::vector<std::uint8_t>> packets;
  if (!lt::read_opus_packets_from_caf(input, packets, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  lt::LoopbackStats stats;
  if (!lt::udp_loopback_roundtrip(packets, stats, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  if (stats.units_sent != stats.units_recv ||
      stats.payload_bytes_sent != stats.payload_bytes_recv) {
    std::cerr << "loopback mismatch: sent_units=" << stats.units_sent
              << " recv_units=" << stats.units_recv
              << " sent_bytes=" << stats.payload_bytes_sent
              << " recv_bytes=" << stats.payload_bytes_recv << '\n';
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
            << " port=" << stats.bound_port
            << " artifact=" << lt::kLoopbackOkRelativePath << '\n';
  return 0;
}
