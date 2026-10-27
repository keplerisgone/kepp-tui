#include "calendar/service.hpp"
#include <algorithm>
namespace kepp::calendar {
CalendarService::CalendarService(GoogleApi a):api_(std::move(a)){}
bool CalendarService::refresh_calendars(){try{auto fresh=api_.calendars();calendars_=std::move(fresh);error_.clear();return true;}catch(const std::exception&e){error_=e.what();return false;}}
bool CalendarService::refresh_week(const WeekRange&w){std::vector<CalendarEvent>fresh;std::vector<std::string>errors;for(auto&c:calendars_)if(c.selected)try{auto e=api_.events(c.id,w);for(auto&x:e)x.writable=c.writable&&!x.recurring;fresh.insert(fresh.end(),e.begin(),e.end());}catch(const std::exception&e){errors.push_back(c.name+": "+e.what());}if(fresh.empty()&&!errors.empty()){error_=errors.front();return false;}std::sort(fresh.begin(),fresh.end(),[](auto&a,auto&b){return a.start<b.start;});events_=std::move(fresh);error_=errors.empty()?"":("Partial result: "+errors.front());return true;}
bool CalendarService::create_event(const EventInput&i){try{api_.create(i);error_.clear();return true;}catch(const std::exception&e){error_=e.what();return false;}}
bool CalendarService::update_event(const CalendarEvent&e,const EventInput&i){try{api_.update(e,i);error_.clear();return true;}catch(const std::exception&x){error_=x.what();return false;}}
bool CalendarService::delete_event(const CalendarEvent&e){try{api_.remove(e);error_.clear();return true;}catch(const std::exception&x){error_=x.what();return false;}}
}
