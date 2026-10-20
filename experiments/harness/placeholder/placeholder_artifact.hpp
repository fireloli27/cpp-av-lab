#pragma once

#include <filesystem>
#include <string>

namespace learning::harness {

inline constexpr const char* kPlaceholderRelativePath =
    "out/harness/placeholder-ok.txt";
inline constexpr const char* kPlaceholderContents = "learning_placeholder: ok\n";

// Writes the harness placeholder success artifact under `root`
// (same relative layout as cwd = repo root).
bool write_placeholder_artifact(const std::filesystem::path& root,
                                std::string& error);

}  // namespace learning::harness
