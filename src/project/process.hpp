#pragma once

#include "project/model.hpp"

#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

namespace kepp::project {

struct ProcessResult {
  int exit_code = -1;
  std::string output;
};

ProcessResult run_captured(const std::vector<std::string> &arguments,
                           const std::filesystem::path &directory,
                           std::chrono::milliseconds timeout = std::chrono::seconds(30));
int run_interactive(const ProjectCommand &command,
                    const std::filesystem::path &directory);

} // namespace kepp::project
