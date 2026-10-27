#include "project/paths.hpp"

#include <cstdlib>
#include <stdexcept>

#include <unistd.h>

namespace kepp::project {

std::filesystem::path expand_user_path(const std::string &value) {
  if (value.empty() || value[0] != '~')
    return value;
  if (value.size() > 1 && value[1] != '/') {
    throw std::runtime_error("Only '~' and '~/' home expansion are supported");
  }
  const char *home = std::getenv("HOME");
  if (!home || !*home)
    throw std::runtime_error("HOME is not set");
  if (value.size() == 1)
    return std::filesystem::path(home);
  return std::filesystem::path(home) / value.substr(2);
}

std::filesystem::path normalize_project_path(const std::string &value) {
  if (value.empty())
    throw std::runtime_error("Project path is empty");
  auto path = expand_user_path(value);
  std::error_code error;
  if (path.is_relative())
    path = std::filesystem::absolute(path, error);
  if (error)
    throw std::runtime_error("Cannot make project path absolute: " +
                             error.message());
  path = path.lexically_normal();
  if (std::filesystem::exists(path, error) && !error) {
    auto canonical = std::filesystem::canonical(path, error);
    if (!error)
      return canonical;
  }
  return path;
}

PathState inspect_path(const std::filesystem::path &path) {
  std::error_code error;
  const auto status = std::filesystem::status(path, error);
  if (error == std::errc::no_such_file_or_directory)
    return PathState::missing;
  if (error)
    return PathState::inaccessible;
  if (!std::filesystem::exists(status))
    return PathState::missing;
  if (!std::filesystem::is_directory(status))
    return PathState::not_directory;
  if (::access(path.c_str(), R_OK | X_OK) != 0)
    return PathState::inaccessible;
  return PathState::available;
}

std::string path_state_text(PathState state) {
  switch (state) {
  case PathState::available:
    return "Directory available";
  case PathState::missing:
    return "Directory missing";
  case PathState::not_directory:
    return "Path is not a directory";
  case PathState::inaccessible:
    return "Directory inaccessible";
  }
  return "Unknown";
}

} // namespace kepp::project
