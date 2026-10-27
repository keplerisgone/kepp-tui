#include "project/service.hpp"

#include "project/paths.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>

namespace kepp::project {
namespace {

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

bool repository_valid(const std::string &value) {
  static const std::regex pattern(R"(^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$)");
  return std::regex_match(value, pattern);
}

std::string trim_whitespace(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

} // namespace

ProjectService::ProjectService(Registry registry)
    : registry_(std::move(registry)) {}

void ProjectService::load() {
  data_ = registry_.load();
  sort();
}

void ProjectService::refresh_paths() {
  for (auto &project : data_.projects)
    project.path_state = inspect_path(project.normalized_path);
}

void ProjectService::add(const ProjectInput &input) {
  if (input.name.empty())
    throw std::runtime_error("Project name is required");
  const auto normalized = normalize_project_path(input.path);
  const auto state = inspect_path(normalized);
  if (state != PathState::available)
    throw std::runtime_error("Project path must be an accessible directory");
  if (!input.repository.empty() && !repository_valid(input.repository)) {
    throw std::runtime_error("Repository must use OWNER/REPO format");
  }
  if (std::any_of(data_.projects.begin(), data_.projects.end(),
                  [&](const Project &project) {
                    return project.normalized_path == normalized;
                  })) {
    throw std::runtime_error("This directory is already registered");
  }

  auto updated = data_;
  Project project;
  project.name = input.name;
  project.configured_path = input.path;
  project.normalized_path = normalized;
  project.description = input.description;
  if (!input.repository.empty())
    project.repository = input.repository;
  project.path_state = state;
  updated.projects.push_back(std::move(project));
  std::sort(updated.projects.begin(), updated.projects.end(),
            [](const Project &left, const Project &right) {
              return lowercase(left.name) < lowercase(right.name);
            });
  registry_.save(updated);
  data_ = std::move(updated);
}

void ProjectService::unregister_project(
    const std::filesystem::path &normalized_path) {
  auto updated = data_;
  const auto original_size = updated.projects.size();
  std::erase_if(updated.projects, [&](const Project &project) {
    return project.normalized_path == normalized_path;
  });
  if (updated.projects.size() == original_size)
    throw std::runtime_error("Project is no longer registered");
  registry_.save(updated);
  data_ = std::move(updated);
}

void ProjectService::set_command(const std::filesystem::path &normalized_path,
                                 const std::string &name,
                                 const ProjectCommand &command) {
  static const std::regex name_pattern(R"(^[A-Za-z0-9_.-]+$)");
  const auto normalized_name = trim_whitespace(name);
  auto normalized_command = command;
  normalized_command.executable = trim_whitespace(command.executable);
  if (!std::regex_match(normalized_name, name_pattern))
    throw std::runtime_error(
        "Command name may contain letters, numbers, '.', '_' and '-'");
  if (normalized_command.executable.empty())
    throw std::runtime_error("Command executable is required");
  auto updated = data_;
  const auto found = std::find_if(
      updated.projects.begin(), updated.projects.end(),
      [&](const Project &p) { return p.normalized_path == normalized_path; });
  if (found == updated.projects.end())
    throw std::runtime_error("Project is no longer registered");
  found->commands[normalized_name] = std::move(normalized_command);
  registry_.save(updated);
  data_ = std::move(updated);
}

void ProjectService::remove_command(
    const std::filesystem::path &normalized_path, const std::string &name) {
  auto updated = data_;
  const auto found = std::find_if(
      updated.projects.begin(), updated.projects.end(),
      [&](const Project &p) { return p.normalized_path == normalized_path; });
  if (found == updated.projects.end() || found->commands.erase(name) == 0)
    throw std::runtime_error("Command is no longer registered");
  registry_.save(updated);
  data_ = std::move(updated);
}

std::vector<std::string> parse_argument_text(const std::string &text) {
  std::vector<std::string> result;
  std::string current;
  char quote = 0;
  bool escaping = false;
  bool started = false;
  for (const char character : text) {
    if (escaping) {
      current.push_back(character);
      escaping = false;
      started = true;
    } else if (character == '\\') {
      escaping = true;
      started = true;
    } else if (quote) {
      if (character == quote)
        quote = 0;
      else
        current.push_back(character);
    } else if (character == '\'' || character == '"') {
      quote = character;
      started = true;
    } else if (std::isspace(static_cast<unsigned char>(character))) {
      if (started) {
        result.push_back(current);
        current.clear();
        started = false;
      }
    } else {
      current.push_back(character);
      started = true;
    }
  }
  if (escaping || quote)
    throw std::runtime_error("Arguments contain an unfinished quote or escape");
  if (started)
    result.push_back(current);
  return result;
}

std::vector<std::size_t>
ProjectService::filtered_indices(const std::string &query) const {
  const auto needle = lowercase(query);
  std::vector<std::size_t> result;
  for (std::size_t index = 0; index < data_.projects.size(); ++index) {
    const auto &project = data_.projects[index];
    const auto haystack =
        lowercase(project.name + "\n" + project.configured_path + "\n" +
                  project.description);
    if (needle.empty() || haystack.find(needle) != std::string::npos)
      result.push_back(index);
  }
  return result;
}

void ProjectService::sort() {
  std::sort(data_.projects.begin(), data_.projects.end(),
            [](const Project &left, const Project &right) {
              return lowercase(left.name) < lowercase(right.name);
            });
}

} // namespace kepp::project
