#pragma once
#include <filesystem>
#include <string>
namespace kepp::calendar {
struct Credentials {std::string access_token,refresh_token,client_id,client_secret;};
class AuthStore {public:explicit AuthStore(std::filesystem::path path={});Credentials load()const;void save(const Credentials&)const;std::string access_token();const std::filesystem::path&path()const{return path_;}private:std::filesystem::path path_;std::string cached_access_token_;};
}
