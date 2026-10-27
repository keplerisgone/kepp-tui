#include "github/gh_client.hpp"
#include "github/service.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <nlohmann/json.hpp>
using namespace kepp::github; namespace fs=std::filesystem;
void req(bool x,const char*m){if(!x)throw std::runtime_error(m);} 
int main(){int ok=0,fail=0;auto test=[&](const char*n,const std::function<void()>&f){try{f();++ok;}catch(const std::exception&e){++fail;std::cerr<<"FAIL "<<n<<": "<<e.what()<<'\n';}};
 test("repo validation",[]{req(valid_repository_name("owner/repo"),"valid rejected");req(!valid_repository_name("bad"),"bad accepted");});
 test("auth failure",[]{GhClient c([](auto const&){return kepp::project::ProcessResult{1,"not logged in"};});std::string e;req(!c.authenticated(e)&&e.find("logged")!=std::string::npos,"auth wrong");});
 test("repo parse",[]{GhClient c([](auto const&){return kepp::project::ProcessResult{0,R"([{"nameWithOwner":"o/r","description":"d","isPrivate":true,"url":"u","defaultBranchRef":{"name":"main"}}])"};});auto v=c.repositories("o");req(v.size()==1&&v[0].default_branch=="main"&&v[0].is_private,"parse wrong");});
 test("issue parse and argv",[]{std::vector<std::string> got;GhClient c([&](auto const&a){got=a;return kepp::project::ProcessResult{0,R"([{"number":2,"title":"T","state":"OPEN","labels":[{"name":"agent:ready"}],"assignees":[]}])"};});auto v=c.issues("o/r","open");req(v[0].labels[0]=="agent:ready","label");req(got[0]=="gh"&&got[3]=="-R"&&got[4]=="o/r","argv");});
 test("invalid json",[]{GhClient c([](auto const&){return kepp::project::ProcessResult{0,"{"};});try{c.repositories("o");throw std::runtime_error("accepted");}catch(const nlohmann::json::exception&){};});
 test("label and query filter",[]{Issue a;a.number=42;a.title="Build thing";a.labels={"agent:ready"};Issue b;b.title="Other";req(filter_issues({a,b},"42","agent:ready").size()==1,"filter");});
 test("draft and state filter",[]{PullRequest a;a.state="OPEN";a.is_draft=true;PullRequest b;b.state="CLOSED";req(filter_prs({a,b},"open",true).size()==1,"filter");});
 test("run states",[]{WorkflowRun a;a.status="completed";a.conclusion="failure";WorkflowRun b;b.status="in_progress";req(filter_runs({a,b},"failed").size()==1,"fail");req(filter_runs({a,b},"running").size()==1,"run");});
 test("registry round trip",[]{auto p=fs::temp_directory_path()/("kepp-gh-"+std::to_string(getpid())+".toml");RepositoryRegistry r(p);r.save({"a/b","c/d"});req(r.load().size()==2,"roundtrip");fs::remove(p);});
 test("detail parsing",[]{GhClient c([](auto const&a){if(a[1]=="issue")return kepp::project::ProcessResult{0,R"({"number":1,"title":"x","comments":[{"author":{"login":"u"},"body":"b"}]})"};return kepp::project::ProcessResult{0,R"({"number":1,"title":"p","isDraft":true,"files":[{"path":"a.cpp"}],"statusCheckRollup":[{"name":"ci","status":"COMPLETED","conclusion":"SUCCESS"}]})"};});req(c.issue("a/b",1).comments[0].author=="u","comment");req(c.pull_request("a/b",1).files[0]=="a.cpp","file");});
 std::cout<<ok<<" passed, "<<fail<<" failed\n";return fail?1:0;}
