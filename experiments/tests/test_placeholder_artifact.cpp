#include "placeholder_artifact.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_root() {
  const auto stamp = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const fs::path root =
      fs::temp_directory_path() / ("learning_placeholder_" + stamp);
  fs::create_directories(root);
  return root;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path);
  std::ostringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

}  // namespace

TEST_CASE("placeholder writes observable success artifact under root",
          "[harness][placeholder]") {
  const fs::path root = make_temp_root();
  std::string error;

  REQUIRE(learning::harness::write_placeholder_artifact(root, error));
  REQUIRE(error.empty());

  const fs::path out_path =
      root / learning::harness::kPlaceholderRelativePath;
  REQUIRE(fs::is_regular_file(out_path));
  REQUIRE(read_file(out_path) == learning::harness::kPlaceholderContents);

  fs::remove_all(root);
}
