#include "project/git.hpp"

#include "project/process.hpp"

namespace kepp::project {
namespace {

std::string trim(std::string value) {
  while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
    value.pop_back();
  return value;
}

} // namespace

GitInfo inspect_git(const std::filesystem::path &directory) {
  GitInfo info;
  const auto probe =
      run_captured({"git", "rev-parse", "--is-inside-work-tree"}, directory);
  if (probe.exit_code != 0 || trim(probe.output) != "true") {
    info.error =
        probe.exit_code == 127 ? "git is not installed" : "Not a Git worktree";
    return info;
  }
  info.repository = true;
  const auto branch =
      run_captured({"git", "branch", "--show-current"}, directory);
  if (branch.exit_code == 0)
    info.branch = trim(branch.output);
  if (info.branch.empty())
    info.branch = "detached HEAD";
  const auto status =
      run_captured({"git", "status", "--short", "--branch"}, directory);
  if (status.exit_code == 0)
    info.status = trim(status.output);
  else
    info.error = "git status failed: " + trim(status.output);
  return info;
}

} // namespace kepp::project
