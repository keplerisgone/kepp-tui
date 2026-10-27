#pragma once
#include "calendar/google_api.hpp"
namespace kepp::calendar {
class CalendarService {public:explicit CalendarService(GoogleApi api=GoogleApi{});bool refresh_calendars();bool refresh_week(const WeekRange&);bool create_event(const EventInput&);bool update_event(const CalendarEvent&,const EventInput&);bool delete_event(const CalendarEvent&);const std::vector<CalendarInfo>&calendars()const{return calendars_;}std::vector<CalendarInfo>&calendars(){return calendars_;}const std::vector<CalendarEvent>&events()const{return events_;}const std::string&error()const{return error_;}GoogleApi&api(){return api_;}private:GoogleApi api_;std::vector<CalendarInfo>calendars_;std::vector<CalendarEvent>events_;std::string error_;};
}
