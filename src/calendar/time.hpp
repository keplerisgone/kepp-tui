#pragma once
#include "calendar/model.hpp"
#include <chrono>
namespace kepp::calendar {
WeekRange week_containing(const std::string &date);
std::string shift_date(const std::string &date,int days);
int weekday_monday_zero(const std::string &date);
std::vector<DaySegment> layout_week(const std::vector<CalendarEvent>&,const WeekRange&);
bool valid_event(const EventInput&,std::string &error);
}
