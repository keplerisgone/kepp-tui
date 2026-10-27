#pragma once

#include "project/model.hpp"

#include <filesystem>
#include <string>

namespace kepp::project {

std::filesystem::path expand_user_path(const std::string &value);
std::filesystem::path normalize_project_path(const std::string &value);
PathState inspect_path(const std::filesystem::path &path);
std::string path_state_text(PathState state);

} // namespace kepp::project
