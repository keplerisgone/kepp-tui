#include "github/gh_client.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;
namespace kepp::github {
namespace {
std::vector<std::string> names(const json &items, const char *nested = nullptr) {
  std::vector<std::string> out;
  if (!items.is_array()) return out;
  for (const auto &item : items) {
    const auto &value = nested && item.contains(nested) ? item.at(nested) : item;
    if (value.is_string()) out.push_back(value.get<std::string>());
    else if (value.is_object()) out.push_back(value.value("name", value.value("login", "")));
  }
  return out;
}
Repository repo_from(const json &j) { return {j.value("nameWithOwner", ""), j.value("description", ""), j.value("defaultBranchRef", json::object()).value("name", ""), j.value("url", ""), j.value("isPrivate", false)}; }
Issue issue_from(const json &j) {
  Issue value; value.number=j.value("number",0); value.title=j.value("title",""); value.body=j.value("body",""); value.state=j.value("state",""); value.updated_at=j.value("updatedAt",""); value.url=j.value("url","");
  value.labels=names(j.value("labels",json::array())); value.assignees=names(j.value("assignees",json::array()));
  for (const auto &c:j.value("comments",json::array())) value.comments.push_back({c.value("author",json::object()).value("login",""),c.value("body",""),c.value("createdAt","")});
  return value;
}
PullRequest pr_from(const json &j) {
  PullRequest v; v.number=j.value("number",0); v.title=j.value("title",""); v.body=j.value("body",""); v.state=j.value("state",""); v.is_draft=j.value("isDraft",false); v.head_branch=j.value("headRefName",""); v.review_decision=j.value("reviewDecision",""); v.url=j.value("url","");
  for (const auto &f:j.value("files",json::array())) v.files.push_back(f.value("path",""));
  for (const auto &c:j.value("statusCheckRollup",json::array())) v.checks.push_back({c.value("name",c.value("context","check")),c.value("status",""),c.value("conclusion","")});
  return v;
}
WorkflowRun run_from(const json &j) { return {j.value("databaseId",std::int64_t{0}),j.value("workflowName",""),j.value("status",""),j.value("conclusion",""),j.value("headBranch",""),j.value("url",""),{}}; }
template<class T, class F> std::vector<T> parse_list(const std::string &s,F f){auto j=json::parse(s);if(!j.is_array())throw std::runtime_error("Expected a JSON array");std::vector<T> out;for(auto &x:j)out.push_back(f(x));return out;}
}

GhClient::GhClient(Runner runner):runner_(std::move(runner)){if(!runner_)runner_=[](const auto&a){return project::run_captured(a,std::filesystem::current_path());};}
std::string GhClient::invoke(const std::vector<std::string>&args)const{auto r=runner_(args);if(r.exit_code!=0)throw std::runtime_error(r.output.empty()?"gh command failed (exit "+std::to_string(r.exit_code)+")":r.output);return r.output;}
bool GhClient::authenticated(std::string &error)const{try{invoke({"gh","auth","status"});error.clear();return true;}catch(const std::exception&e){error=e.what();return false;}}
std::vector<Repository> GhClient::repositories(const std::string&o)const{return parse_list<Repository>(invoke({"gh","repo","list",o,"--json","nameWithOwner,description,isPrivate,url,defaultBranchRef","--limit","100"}),repo_from);}
Repository GhClient::repository(const std::string&r)const{return repo_from(json::parse(invoke({"gh","repo","view",r,"--json","nameWithOwner,description,defaultBranchRef,url,isPrivate"})));}
std::vector<Issue> GhClient::issues(const std::string&r,const std::string&s,int n)const{return parse_list<Issue>(invoke({"gh","issue","list","-R",r,"--state",s,"--json","number,title,state,labels,assignees,updatedAt,url","--limit",std::to_string(n)}),issue_from);}
Issue GhClient::issue(const std::string&r,int n)const{return issue_from(json::parse(invoke({"gh","issue","view",std::to_string(n),"-R",r,"--json","number,title,body,state,labels,assignees,comments,updatedAt,url"})));}
std::vector<PullRequest> GhClient::pull_requests(const std::string&r,int n)const{return parse_list<PullRequest>(invoke({"gh","pr","list","-R",r,"--state","all","--json","number,title,state,isDraft,headRefName,reviewDecision,url","--limit",std::to_string(n)}),pr_from);}
PullRequest GhClient::pull_request(const std::string&r,int n)const{return pr_from(json::parse(invoke({"gh","pr","view",std::to_string(n),"-R",r,"--json","number,title,body,state,isDraft,headRefName,reviewDecision,url,files,statusCheckRollup"})));}
std::vector<WorkflowRun> GhClient::runs(const std::string&r,int n)const{return parse_list<WorkflowRun>(invoke({"gh","run","list","-R",r,"--limit",std::to_string(n),"--json","databaseId,workflowName,status,conclusion,headBranch,url"}),run_from);}
WorkflowRun GhClient::run(const std::string&r,std::int64_t id)const{auto v=run_from(json::parse(invoke({"gh","run","view",std::to_string(id),"-R",r,"--json","databaseId,workflowName,status,conclusion,headBranch,url"})));auto logs=runner_({"gh","run","view",std::to_string(id),"-R",r,"--log-failed"});v.failed_log=logs.output;return v;}
} // namespace kepp::github
