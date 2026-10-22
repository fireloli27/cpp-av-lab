#include "loopback_artifact.hpp"
#include "transport_unit.hpp"
#include "udp_loopback.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace lt = learning::audio::local_transport;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_audio_local_transport_" + stamp);
  fs::create_directories(root);
  return root;
}

}  // namespace

TEST_CASE("transport unit pack/unpack roundtrips synthetic Opus payload",
          "[audio][local-transport]") {
  const std::vector<std::uint8_t> payload = {0x01, 0x02, 0xfc, 0x00, 0xff};

  const auto unit = lt::pack_transport_unit(payload);
  REQUIRE(unit.size() == 4 + payload.size());
  // Length is big-endian uint32 equal to payload size (not including the 4 bytes).
  REQUIRE(unit[0] == 0);
  REQUIRE(unit[1] == 0);
  REQUIRE(unit[2] == 0);
  REQUIRE(unit[3] == 5);
  REQUIRE(std::vector<std::uint8_t>(unit.begin() + 4, unit.end()) == payload);

  std::vector<std::uint8_t> out;
  std::string error;
  REQUIRE(lt::unpack_transport_unit(unit, out, error));
  REQUIRE(error.empty());
  REQUIRE(out == payload);
}

TEST_CASE("transport unit unpack rejects truncated length prefix",
          "[audio][local-transport]") {
  const std::vector<std::uint8_t> bad = {0x00, 0x00};
  std::vector<std::uint8_t> out;
  std::string error;
  REQUIRE_FALSE(lt::unpack_transport_unit(bad, out, error));
  REQUIRE_FALSE(error.empty());
}

TEST_CASE("transport unit unpack rejects length longer than remaining bytes",
          "[audio][local-transport]") {
  const std::vector<std::uint8_t> bad = {0x00, 0x00, 0x00, 0x03, 0xaa};
  std::vector<std::uint8_t> out;
  std::string error;
  REQUIRE_FALSE(lt::unpack_transport_unit(bad, out, error));
  REQUIRE_FALSE(error.empty());
}

TEST_CASE("udp loopback roundtrips length-prefixed synthetic payloads",
          "[audio][local-transport]") {
  const std::vector<std::vector<std::uint8_t>> payloads = {
      {0xde, 0xad},
      {0x01, 0x02, 0x03, 0x04, 0x05},
      {},
  };
  lt::LoopbackStats stats;
  std::string error;
  REQUIRE(lt::udp_loopback_roundtrip(payloads, stats, error));
  REQUIRE(error.empty());
  REQUIRE(stats.units_sent == 3);
  REQUIRE(stats.units_recv == 3);
  REQUIRE(stats.payload_bytes_sent == 2 + 5 + 0);
  REQUIRE(stats.payload_bytes_recv == stats.payload_bytes_sent);
  REQUIRE(stats.bound_port > 0);
}

TEST_CASE("loopback ok artifact matches contract contents",
          "[audio][local-transport]") {
  const fs::path root = make_temp_root();
  lt::LoopbackStats stats;
  stats.units_sent = 7;
  stats.units_recv = 7;
  stats.payload_bytes_sent = 420;
  stats.payload_bytes_recv = 420;
  stats.bound_port = 54321;
  std::string error;
  REQUIRE(lt::write_loopback_ok_artifact(root, stats, error));
  REQUIRE(error.empty());

  const fs::path out_path = root / lt::kLoopbackOkRelativePath;
  REQUIRE(fs::is_regular_file(out_path));
  std::ifstream in(out_path);
  std::ostringstream body;
  body << in.rdbuf();
  REQUIRE(body.str() ==
          "learning_audio_local_transport: ok\n"
          "units=7\n"
          "payload_bytes=420\n"
          "port=54321\n");
  fs::remove_all(root);
}
