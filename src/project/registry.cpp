#include "project/registry.hpp"

#include "project/paths.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <toml++/toml.hpp>

#include <unistd.h>

namespace kepp::project {
namespace {

std::string trim_whitespace(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::optional<std::filesystem::file_time_type>
timestamp(const std::filesystem::path &path) {
  std::error_code error;
  if (!std::filesystem::exists(path, error) || error)
    return std::nullopt;
  const auto value = std::filesystem::last_write_time(path, error);
  if (error)
    throw std::runtime_error("Cannot inspect registry: " + error.message());
  return value;
}

ProjectCommand parse_command(const toml::array &values,
                             const std::string &name) {
  if (values.empty())
    throw std::runtime_error("Command '" + name + "' must not be empty");
  ProjectCommand command;
  for (std::size_t index = 0; index < values.size(); ++index) {
    const auto value = values[index].value<std::string>();
    if (!value)
      throw std::runtime_error("Command '" + name +
                               "' must contain only strings");
    if (index == 0)
      command.executable = trim_whitespace(*value);
    else
      command.arguments.push_back(*value);
  }
  if (command.executable.empty())
    throw std::runtime_error("Command '" + name +
                             "' executable must not be blank");
  return command;
}

} // namespace

std::filesystem::path default_registry_path() {
  if (const char *override_path = std::getenv("KEPP_PROJECTS_FILE");
      override_path && *override_path) {
    return expand_user_path(override_path);
  }
  if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
    return std::filesystem::path(xdg) / "kepp" / "projects.toml";
  }
  const char *home = std::getenv("HOME");
  if (!home || !*home)
    throw std::runtime_error("Neither XDG_CONFIG_HOME nor HOME is set");
  return std::filesystem::path(home) / ".config" / "kepp" / "projects.toml";
}

Registry::Registry(std::filesystem::path path) : path_(std::move(path)) {}

RegistryData Registry::load() {
  loaded_timestamp_ = timestamp(path_);
  if (!loaded_timestamp_)
    return {};

  toml::table document;
  try {
    document = toml::parse_file(path_.string());
  } catch (const toml::parse_error &error) {
    throw std::runtime_error("Cannot parse registry: " +
                             std::string(error.description()));
  }

  RegistryData data;
  const auto version = document["version"].value<int64_t>();
  if (!version)
    throw std::runtime_error("Registry is missing integer 'version'");
  if (*version != 1)
    throw std::runtime_error("Unsupported registry schema version: " +
                             std::to_string(*version));
  data.version = static_cast<int>(*version);

  const auto *projects = document["projects"].as_array();
  if (!projects)
    return data;
  for (const auto &node : *projects) {
    const auto *table = node.as_table();
    if (!table)
      throw std::runtime_error("Each project must be a TOML table");
    Project project;
    const auto name = (*table)["name"].value<std::string>();
    const auto path = (*table)["path"].value<std::string>();
    if (!name || name->empty())
      throw std::runtime_error("Project name is required");
    if (!path || path->empty())
      throw std::runtime_error("Project path is required");
    project.name = *name;
    project.configured_path = *path;
    project.description = (*table)["description"].value_or(std::string{});
    if (const auto repository = (*table)["repository"].value<std::string>();
        repository && !repository->empty()) {
      project.repository = *repository;
    }
    project.normalized_path = normalize_project_path(project.configured_path);
    project.path_state = inspect_path(project.normalized_path);

    if (const auto *commands = (*table)["commands"].as_table()) {
      for (const auto &[key, value] : *commands) {
        const auto *array = value.as_array();
        if (!array)
          throw std::runtime_error("Command '" + std::string(key.str()) +
                                   "' must be an array");
        project.commands.emplace(std::string(key.str()),
                                 parse_command(*array, std::string(key.str())));
      }
    }
    data.projects.push_back(std::move(project));
  }
  return data;
}

void Registry::save(const RegistryData &data) {
  if (data.version != 1)
    throw std::runtime_error("Only registry schema version 1 can be saved");
  if (timestamp(path_) != loaded_timestamp_) {
    throw std::runtime_error("Registry changed on disk; reload before saving");
  }

  toml::table document;
  document.insert("version", data.version);
  toml::array projects;
  for (const auto &project : data.projects) {
    toml::table table;
    table.insert("name", project.name);
    table.insert("path", project.configured_path);
    if (!project.description.empty())
      table.insert("description", project.description);
    if (project.repository && !project.repository->empty())
      table.insert("repository", *project.repository);
    if (!project.commands.empty()) {
      toml::table commands;
      for (const auto &[name, command] : project.commands) {
        toml::array values;
        values.push_back(command.executable);
        for (const auto &argument : command.arguments)
          values.push_back(argument);
        commands.insert(name, std::move(values));
      }
      table.insert("commands", std::move(commands));
    }
    projects.push_back(std::move(table));
  }
  document.insert("projects", std::move(projects));

  std::error_code error;
  std::filesystem::create_directories(path_.parent_path(), error);
  if (error)
    throw std::runtime_error("Cannot create registry directory: " +
                             error.message());

  auto temporary = path_;
  temporary += ".tmp." + std::to_string(::getpid());
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output)
      throw std::runtime_error("Cannot open temporary registry for writing");
    output << document << '\n';
    output.flush();
    if (!output) {
      std::filesystem::remove(temporary, error);
      throw std::runtime_error("Cannot write temporary registry");
    }
  }
  std::filesystem::permissions(temporary,
                               std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace, error);
  if (error) {
    std::filesystem::remove(temporary, error);
    throw std::runtime_error("Cannot secure temporary registry");
  }
  std::filesystem::rename(temporary, path_, error);
  if (error) {
    std::filesystem::remove(temporary);
    throw std::runtime_error("Cannot replace registry: " + error.message());
  }
  loaded_timestamp_ = timestamp(path_);
}

} // namespace kepp::project
