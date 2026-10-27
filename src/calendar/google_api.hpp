#pragma once
#include "calendar/auth.hpp"
#include "calendar/model.hpp"
#include <functional>
namespace kepp::calendar {
struct HttpResponse {int status=0;std::string body;};
using HttpRunner=std::function<HttpResponse(const std::string&,const std::string&,const std::string&,const std::vector<std::string>&)>;
class GoogleApi {public:explicit GoogleApi(AuthStore auth=AuthStore{},HttpRunner runner={});std::vector<CalendarInfo> calendars();std::vector<CalendarEvent> events(const std::string&,const WeekRange&);CalendarEvent create(const EventInput&);CalendarEvent update(const CalendarEvent&,const EventInput&);void remove(const CalendarEvent&);private:AuthStore auth_;HttpRunner runner_;HttpResponse request(const std::string &method,const std::string &url,const std::string &body = "",std::vector<std::string> headers = {});};
}
