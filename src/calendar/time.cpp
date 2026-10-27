#include "calendar/time.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <stdexcept>

namespace kepp::calendar {namespace {
std::tm date_tm(const std::string&s){std::tm t{};if(std::sscanf(s.c_str(),"%d-%d-%d",&t.tm_year,&t.tm_mon,&t.tm_mday)!=3)throw std::runtime_error("Invalid date: "+s);t.tm_year-=1900;t.tm_mon-=1;t.tm_hour=12;return t;}
std::string fmt(std::tm t){char b[11];std::strftime(b,sizeof b,"%Y-%m-%d",&t);return b;}
std::time_t local_epoch(std::tm &t){return std::mktime(&t);}
int minute(const std::string&s){int h=0,m=0;if(s.size()>=16)std::sscanf(s.c_str()+11,"%d:%d",&h,&m);return h*60+m;}
std::string daypart(const std::string&s){return s.substr(0,std::min<std::size_t>(10,s.size()));}
}
int weekday_monday_zero(const std::string&s){auto t=date_tm(s);local_epoch(t);return (t.tm_wday+6)%7;}
std::string shift_date(const std::string&s,int days){auto t=date_tm(s);t.tm_mday+=days;local_epoch(t);return fmt(t);}
WeekRange week_containing(const std::string&s){auto m=shift_date(s,-weekday_monday_zero(s));return{m,shift_date(m,7)};}
bool valid_event(const EventInput&e,std::string&err){if(e.title.empty()){err="Title is required";return false;}if(e.calendar_id.empty()){err="Calendar is required";return false;}if(e.start.empty()||e.end.empty()){err="Start and end are required";return false;}if(e.end<=e.start){err="End must be after start";return false;}err.clear();return true;}
std::vector<DaySegment> layout_week(const std::vector<CalendarEvent>&events,const WeekRange&w){
  std::vector<DaySegment>o;
  for(auto&e:events){
    auto first=std::max(daypart(e.start),w.monday);
    auto last_exclusive=std::min(daypart(e.end),w.next_monday);
    if(e.all_day){
      for(auto d=first;d<last_exclusive;d=shift_date(d,1))o.push_back({e,weekday_monday_zero(d),0,1440,0,1});
    }else{
      auto d=daypart(e.start);
      if(d>=w.monday&&d<w.next_monday)o.push_back({e,weekday_monday_zero(d),minute(e.start),daypart(e.end)==d?minute(e.end):1440,0,1});
      if(daypart(e.end)>d){
        for(d=shift_date(d,1);d<daypart(e.end)&&d<w.next_monday;d=shift_date(d,1))if(d>=w.monday)o.push_back({e,weekday_monday_zero(d),0,1440,0,1});
        if(daypart(e.end)>=w.monday&&daypart(e.end)<w.next_monday&&minute(e.end)>0)o.push_back({e,weekday_monday_zero(daypart(e.end)),0,minute(e.end),0,1});
      }
    }
  }
  for(int day=0;day<7;++day){std::vector<DaySegment*>v;for(auto&x:o)if(x.day==day&&!x.event.all_day)v.push_back(&x);std::sort(v.begin(),v.end(),[](auto*a,auto*b){return a->start_minute<b->start_minute;});for(std::size_t i=0;i<v.size();++i){int col=0;for(std::size_t j=0;j<i;++j)if(v[j]->end_minute>v[i]->start_minute)col=std::max(col,v[j]->column+1);v[i]->column=col;for(auto*x:v)if(x->start_minute<v[i]->end_minute&&x->end_minute>v[i]->start_minute)x->columns=std::max(x->columns,col+1);}}
  return o;}
}
