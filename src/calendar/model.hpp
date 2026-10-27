#pragma once
#include <string>
#include <vector>
namespace kepp::calendar {
struct CalendarInfo {std::string id,name,color;bool selected=true,writable=false;};
struct CalendarEvent {std::string id,calendar_id,title,description,etag,start,end;bool all_day=false,recurring=false,writable=false;};
struct EventInput {std::string calendar_id,title,description,start,end;bool all_day=false;};
struct DaySegment {CalendarEvent event;int day=0,start_minute=0,end_minute=0,column=0,columns=1;};
struct WeekRange {std::string monday,next_monday;};
}
