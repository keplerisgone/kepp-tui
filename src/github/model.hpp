#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kepp::github {

struct Repository { std::string name_with_owner, description, default_branch, url; bool is_private = false; };
struct Comment { std::string author, body, created_at; };
struct Issue { int number = 0; std::string title, body, state, updated_at, url; std::vector<std::string> labels, assignees; std::vector<Comment> comments; };
struct Check { std::string name, status, conclusion; };
struct PullRequest { int number = 0; std::string title, body, state, head_branch, review_decision, url; bool is_draft = false; std::vector<std::string> files; std::vector<Check> checks; };
struct WorkflowRun { std::int64_t id = 0; std::string workflow_name, status, conclusion, branch, url, failed_log; };

} // namespace kepp::github
