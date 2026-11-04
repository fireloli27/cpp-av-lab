#include "video_loopback_artifact.hpp"
#include "video_transport_unit.hpp"
#include "video_udp_loopback.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace lt = learning::video::local_transport;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_video_transport_" + stamp);
  fs::create_directories(root);
  return root;
}

std::vector<std::uint8_t> bytes_of(std::size_t n, std::uint8_t seed) {
  std::vector<std::uint8_t> out(n);
  for (std::size_t i = 0; i < n; ++i) {
    out[i] = static_cast<std::uint8_t>(seed + i);
  }
  return out;
}

}  // namespace

TEST_CASE("video transport unit fragments and reassembles in any order",
          "[video][local-transport]") {
  const auto payload = bytes_of(2500, 7);
  std::vector<std::vector<std::uint8_t>> datagrams;
  std::string error;
  REQUIRE(lt::split_access_unit(3, payload, datagrams, error));
  REQUIRE(error.empty());
  REQUIRE(datagrams.size() == 3);
  REQUIRE(datagrams[0].size() == lt::kDatagramHeaderBytes + 1200);
  REQUIRE(datagrams[2].size() == lt::kDatagramHeaderBytes + 100);

  std::swap(datagrams[0], datagrams[2]);
  std::vector<std::uint8_t> rebuilt;
  REQUIRE(lt::reassemble_access_unit(datagrams, 3, rebuilt, error));
  REQUIRE(rebuilt == payload);

  datagrams.pop_back();
  REQUIRE_FALSE(lt::reassemble_access_unit(datagrams, 3, rebuilt, error));

  REQUIRE_FALSE(lt::split_access_unit(0, {}, datagrams, error));
}

TEST_CASE("video udp loopback roundtrips fragmented access units",
          "[video][local-transport]") {
  std::vector<std::vector<std::uint8_t>> units = {
      bytes_of(10, 1),
      bytes_of(1200, 2),
      bytes_of(1201, 3),
  };
  lt::LoopbackStats stats;
  std::string error;
  REQUIRE(lt::udp_loopback_roundtrip(units, stats, error));
  REQUIRE(error.empty());
  REQUIRE(stats.units_sent == 3);
  REQUIRE(stats.units_recv == 3);
  REQUIRE(stats.payload_bytes_sent == 10u + 1200u + 1201u);
  REQUIRE(stats.payload_bytes_recv == stats.payload_bytes_sent);
  REQUIRE(stats.fragments_sent == 1 + 1 + 2);
  REQUIRE(stats.fragments_recv == stats.fragments_sent);
  REQUIRE(stats.bound_port > 0);

  const fs::path root = make_temp_root();
  REQUIRE(lt::write_loopback_ok_artifact(root, stats, error));
  std::ifstream in(root / lt::kLoopbackOkRelativePath);
  std::string text((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  REQUIRE(text.find("learning_video_local_transport: ok\n") == 0);
  REQUIRE(text.find("units_sent=3\n") != std::string::npos);
  REQUIRE(text.find("fragments_sent=4\n") != std::string::npos);
  REQUIRE(text.find("frag_payload_max=1200\n") != std::string::npos);
  fs::remove_all(root);
}
