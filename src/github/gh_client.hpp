#pragma once
#include "github/model.hpp"
#include "project/process.hpp"
#include <chrono>
#include <filesystem>
#include <functional>

namespace kepp::github {
using Runner = std::function<project::ProcessResult(const std::vector<std::string>&)>;

class GhClient {
public:
  explicit GhClient(Runner runner = {});
  bool authenticated(std::string &error) const;
  std::vector<Repository> repositories(const std::string &owner) const;
  Repository repository(const std::string &name) const;
  std::vector<Issue> issues(const std::string &repo, const std::string &state, int limit = 30) const;
  Issue issue(const std::string &repo, int number) const;
  std::vector<PullRequest> pull_requests(const std::string &repo, int limit = 30) const;
  PullRequest pull_request(const std::string &repo, int number) const;
  std::vector<WorkflowRun> runs(const std::string &repo, int limit = 30) const;
  WorkflowRun run(const std::string &repo, std::int64_t id) const;
private:
  Runner runner_;
  std::string invoke(const std::vector<std::string> &args) const;
};
} // namespace kepp::github
