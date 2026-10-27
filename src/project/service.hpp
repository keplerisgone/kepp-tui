#pragma once

#include "project/model.hpp"
#include "project/registry.hpp"

#include <string>
#include <vector>

namespace kepp::project {

class ProjectService {
public:
  explicit ProjectService(Registry registry = Registry{});

  void load();
  void refresh_paths();
  void add(const ProjectInput &input);
  void unregister_project(const std::filesystem::path &normalized_path);
  void set_command(const std::filesystem::path &normalized_path,
                   const std::string &name, const ProjectCommand &command);
  void remove_command(const std::filesystem::path &normalized_path,
                      const std::string &name);
  std::vector<std::size_t> filtered_indices(const std::string &query) const;

  const RegistryData &data() const { return data_; }
  const std::filesystem::path &registry_path() const {
    return registry_.path();
  }

private:
  void sort();

  Registry registry_;
  RegistryData data_;
};

std::vector<std::string> parse_argument_text(const std::string &text);

} // namespace kepp::project
