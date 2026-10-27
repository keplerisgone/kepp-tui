#pragma once

#include "project/model.hpp"

#include <filesystem>
#include <optional>

namespace kepp::project {

std::filesystem::path default_registry_path();

class Registry {
public:
  explicit Registry(std::filesystem::path path = default_registry_path());

  RegistryData load();
  void save(const RegistryData &data);
  const std::filesystem::path &path() const { return path_; }

private:
  std::filesystem::path path_;
  std::optional<std::filesystem::file_time_type> loaded_timestamp_;
};

} // namespace kepp::project
