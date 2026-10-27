#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace kepp::project {

struct ProjectCommand {
  std::string executable;
  std::vector<std::string> arguments;

  bool operator==(const ProjectCommand &) const = default;
};

enum class PathState { available, missing, not_directory, inaccessible };

struct Project {
  std::string name;
  std::string configured_path;
  std::filesystem::path normalized_path;
  std::string description;
  std::optional<std::string> repository;
  std::map<std::string, ProjectCommand> commands;
  PathState path_state = PathState::missing;
};

struct ProjectInput {
  std::string name;
  std::string path;
  std::string description;
  std::string repository;
};

struct RegistryData {
  int version = 1;
  std::vector<Project> projects;
};

} // namespace kepp::project
