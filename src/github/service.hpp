#pragma once
#include "github/gh_client.hpp"
#include <filesystem>
#include <optional>

namespace kepp::github {
bool valid_repository_name(const std::string &name);
std::vector<Issue> filter_issues(const std::vector<Issue>&, const std::string &query, const std::string &label);
std::vector<PullRequest> filter_prs(const std::vector<PullRequest>&, const std::string &state, bool drafts_only);
std::vector<WorkflowRun> filter_runs(const std::vector<WorkflowRun>&, const std::string &kind);
class RepositoryRegistry {
public:
  explicit RepositoryRegistry(std::filesystem::path path = {});
  std::vector<std::string> load() const;
  void save(const std::vector<std::string>&) const;
  const std::filesystem::path &path() const{return path_;}
private: std::filesystem::path path_;
};
} // namespace kepp::github
