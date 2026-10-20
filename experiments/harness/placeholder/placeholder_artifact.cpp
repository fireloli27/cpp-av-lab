#include "placeholder_artifact.hpp"

#include <fstream>

namespace learning::harness {

bool write_placeholder_artifact(const std::filesystem::path& root,
                                std::string& error) {
  namespace fs = std::filesystem;

  const fs::path out_path = root / kPlaceholderRelativePath;
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
  out << kPlaceholderContents;
  out.close();
  if (!out) {
    error = "failed while writing " + out_path.string();
    return false;
  }

  error.clear();
  return true;
}

}  // namespace learning::harness
