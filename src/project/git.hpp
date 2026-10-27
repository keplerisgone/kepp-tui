#pragma once

#include <filesystem>
#include <string>

namespace kepp::project {

struct GitInfo {
  bool repository = false;
  std::string branch;
  std::string status;
  std::string error;
};

GitInfo inspect_git(const std::filesystem::path &directory);

} // namespace kepp::project
