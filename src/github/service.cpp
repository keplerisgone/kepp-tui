#include "github/service.hpp"
#include <toml++/toml.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sys/stat.h>

namespace kepp::github {
namespace {std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return std::tolower(c);});return s;}}
bool valid_repository_name(const std::string&n){static const std::regex r(R"(^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$)");return std::regex_match(n,r);}
std::vector<Issue> filter_issues(const std::vector<Issue>&v,const std::string&q,const std::string&label){std::vector<Issue>o;auto lq=lower(q);for(auto&i:v){bool text=lq.empty()||lower(i.title).find(lq)!=std::string::npos||std::to_string(i.number).find(lq)!=std::string::npos;bool lab=label.empty()||std::find(i.labels.begin(),i.labels.end(),label)!=i.labels.end();if(text&&lab)o.push_back(i);}return o;}
std::vector<PullRequest> filter_prs(const std::vector<PullRequest>&v,const std::string&s,bool d){std::vector<PullRequest>o;for(auto&p:v)if((s.empty()||s=="all"||lower(p.state)==lower(s))&&(!d||p.is_draft))o.push_back(p);return o;}
std::vector<WorkflowRun> filter_runs(const std::vector<WorkflowRun>&v,const std::string&k){std::vector<WorkflowRun>o;for(auto&r:v)if(k.empty()||k=="all"||(k=="failed"&&r.conclusion=="failure")||(k=="running"&&r.status!="completed"))o.push_back(r);return o;}
RepositoryRegistry::RepositoryRegistry(std::filesystem::path p):path_(std::move(p)){if(path_.empty()){if(auto*x=std::getenv("KEPP_GITHUB_FILE"))path_=x;else{auto*y=std::getenv("XDG_CONFIG_HOME");path_=(y&&*y?std::filesystem::path(y):std::filesystem::path(std::getenv("HOME"))/".config")/"kepp/github.toml";}}}
std::vector<std::string> RepositoryRegistry::load()const{if(!std::filesystem::exists(path_))return{};auto t=toml::parse_file(path_.string());std::vector<std::string>o;if(auto*a=t["repositories"].as_array())for(auto&x:*a)if(auto s=x.value<std::string>())o.push_back(*s);return o;}
void RepositoryRegistry::save(const std::vector<std::string>&v)const{std::filesystem::create_directories(path_.parent_path());auto tmp=path_;tmp+=".tmp";std::ofstream f(tmp);f<<"version = 1\nrepositories = [";for(std::size_t i=0;i<v.size();++i)f<<(i?", ":"")<<'"'<<v[i]<<'"';f<<"]\n";f.close();::chmod(tmp.c_str(),0600);std::filesystem::rename(tmp,path_);}
} // namespace kepp::github
