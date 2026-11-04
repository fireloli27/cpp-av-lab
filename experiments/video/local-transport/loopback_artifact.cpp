#include "video_loopback_artifact.hpp"

#include "video_transport_unit.hpp"

#include <fstream>

namespace learning::video::local_transport {

bool write_loopback_ok_artifact(const std::filesystem::path& root,
                                const LoopbackStats& stats, std::string& error) {
  namespace fs = std::filesystem;
  if (stats.units_sent == 0 || stats.units_sent != stats.units_recv ||
      stats.payload_bytes_sent != stats.payload_bytes_recv ||
      stats.fragments_sent == 0 ||
      stats.fragments_sent != stats.fragments_recv) {
    error = "refusing artifact: send/recv counts disagree";
    return false;
  }

  const fs::path out_path = root / kLoopbackOkRelativePath;
  std::error_code ec;
  fs::create_directories(out_path.parent_path(), ec);
  if (ec) {
    error = "failed to create " + out_path.parent_path().string() + ": " +
            ec.message();
    return false;
  }

  std::ofstream out(out_path);
  if (!out) {
    error = "failed to open " + out_path.string();
    return false;
  }
  out << "learning_video_local_transport: ok\n"
      << "units_sent=" << stats.units_sent << '\n'
      << "units_recv=" << stats.units_recv << '\n'
      << "payload_bytes_sent=" << stats.payload_bytes_sent << '\n'
      << "payload_bytes_recv=" << stats.payload_bytes_recv << '\n'
      << "fragments_sent=" << stats.fragments_sent << '\n'
      << "fragments_recv=" << stats.fragments_recv << '\n'
      << "frag_payload_max=" << kFragPayloadMax << '\n'
      << "port=" << stats.bound_port << '\n';
  out.close();
  if (!out) {
    error = "failed while writing " + out_path.string();
    return false;
  }
  error.clear();
  return true;
}

}  // namespace learning::video::local_transport
