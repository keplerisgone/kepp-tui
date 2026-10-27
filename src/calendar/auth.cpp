#include "calendar/auth.hpp"
#include "project/process.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <cctype>
namespace kepp::calendar {
namespace {std::string form_encode(const std::string&s){std::ostringstream o;o<<std::hex<<std::uppercase;for(unsigned char c:s)if(std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')o<<c;else o<<'%'<<std::setw(2)<<std::setfill('0')<<(int)c;return o.str();}}
AuthStore::AuthStore(std::filesystem::path p):path_(std::move(p)){if(path_.empty()){if(auto*x=std::getenv("KEPP_CALENDAR_CREDENTIALS"))path_=x;else{auto*y=std::getenv("XDG_CONFIG_HOME");path_=(y&&*y?std::filesystem::path(y):std::filesystem::path(std::getenv("HOME"))/".config")/"kepp/calendar-credentials.json";}}}
Credentials AuthStore::load()const{if(auto*x=std::getenv("KEPP_GOOGLE_ACCESS_TOKEN"))return{x,{},{},{}};if(!std::filesystem::exists(path_))throw std::runtime_error("Calendar credentials not found. See README Calendar setup.");std::ifstream f(path_);auto j=nlohmann::json::parse(f);return{j.value("access_token",""),j.value("refresh_token",""),j.value("client_id",""),j.value("client_secret","")};}
void AuthStore::save(const Credentials&c)const{std::filesystem::create_directories(path_.parent_path());auto t=path_;t+=".tmp";std::ofstream f(t);f<<nlohmann::json{{"access_token",c.access_token},{"refresh_token",c.refresh_token},{"client_id",c.client_id},{"client_secret",c.client_secret}}.dump(2)<<'\n';f.close();::chmod(t.c_str(),0600);std::filesystem::rename(t,path_);}
std::string AuthStore::access_token(){if(!cached_access_token_.empty())return cached_access_token_;auto c=load();if(c.refresh_token.empty()||c.client_id.empty()||c.client_secret.empty()){if(c.access_token.empty())throw std::runtime_error("Calendar credential file is incomplete");return cached_access_token_=c.access_token;}std::string pattern=(std::filesystem::temp_directory_path()/"kepp-oauth-XXXXXX").string();std::vector<char>name(pattern.begin(),pattern.end());name.push_back('\0');int fd=::mkstemp(name.data());if(fd<0)throw std::runtime_error("Cannot create protected OAuth request file");std::string body="client_id="+form_encode(c.client_id)+"&client_secret="+form_encode(c.client_secret)+"&refresh_token="+form_encode(c.refresh_token)+"&grant_type=refresh_token";ssize_t written=::write(fd,body.data(),body.size());::close(fd);if(written!=(ssize_t)body.size()){::unlink(name.data());throw std::runtime_error("Cannot write OAuth request");}auto r=project::run_captured({"curl","--silent","--show-error","--fail","-X","POST","https://oauth2.googleapis.com/token","-H","Content-Type: application/x-www-form-urlencoded","--data-binary","@"+std::string(name.data())},std::filesystem::current_path());::unlink(name.data());if(r.exit_code)throw std::runtime_error("Google token refresh failed");auto j=nlohmann::json::parse(r.output);c.access_token=j.value("access_token","");if(c.access_token.empty())throw std::runtime_error("Google token response omitted access_token");save(c);cached_access_token_=c.access_token;return cached_access_token_;}
}
