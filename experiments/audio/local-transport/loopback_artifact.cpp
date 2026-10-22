#include "loopback_artifact.hpp"

#include <fstream>

namespace learning::audio::local_transport {

bool write_loopback_ok_artifact(const std::filesystem::path& root,
                                const LoopbackStats& stats, std::string& error) {
  namespace fs = std::filesystem;

  if (stats.units_sent != stats.units_recv ||
      stats.payload_bytes_sent != stats.payload_bytes_recv) {
    error = "refusing artifact: send/recv counts or payload bytes disagree";
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
  out << "learning_audio_local_transport: ok\n"
      << "units=" << stats.units_sent << '\n'
      << "payload_bytes=" << stats.payload_bytes_sent << '\n'
      << "port=" << stats.bound_port << '\n';
  out.close();
  if (!out) {
    error = "failed while writing " + out_path.string();
    return false;
  }

  error.clear();
  return true;
}

}  // namespace learning::audio::local_transport
