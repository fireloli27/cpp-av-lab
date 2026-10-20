#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
  namespace fs = std::filesystem;

  const fs::path out_path = fs::path("out") / "harness" / "placeholder-ok.txt";
  std::error_code ec;
  fs::create_directories(out_path.parent_path(), ec);
  if (ec) {
    std::cerr << "failed to create " << out_path.parent_path().string() << ": "
              << ec.message() << '\n';
    return 1;
  }

  std::ofstream out(out_path);
  if (!out) {
    std::cerr << "failed to open " << out_path.string() << '\n';
    return 1;
  }
  out << "learning_placeholder: ok\n";
  out.close();
  if (!out) {
    std::cerr << "failed while writing " << out_path.string() << '\n';
    return 1;
  }

  std::cout << "wrote " << out_path.string() << '\n';
  return 0;
}
