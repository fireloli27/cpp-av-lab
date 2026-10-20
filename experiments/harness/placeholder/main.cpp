#include "placeholder_artifact.hpp"

#include <iostream>

int main() {
  std::string error;
  if (!learning::harness::write_placeholder_artifact(".", error)) {
    std::cerr << error << '\n';
    return 1;
  }
  std::cout << "wrote " << learning::harness::kPlaceholderRelativePath << '\n';
  return 0;
}
