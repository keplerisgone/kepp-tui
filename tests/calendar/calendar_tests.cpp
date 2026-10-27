#include "calendar/service.hpp"
#include "calendar/time.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
using namespace kepp::calendar;
void req(bool x,const char*m){if(!x)throw std::runtime_error(m);} 
int main(){::setenv("KEPP_GOOGLE_ACCESS_TOKEN","test-token",1);int ok=0,fail=0;auto test=[&](const char*n,const std::function<void()>&f){try{f();++ok;}catch(const std::exception&e){++fail;std::cerr<<"FAIL "<<n<<": "<<e.what()<<'\n';}};
 test("monday range",[]{auto w=week_containing("2026-10-27");req(w.monday=="2026-10-26"&&w.next_monday=="2026-11-02","range");});
 test("sunday shift",[]{req(week_containing("2026-11-01").monday=="2026-10-26","sunday");req(shift_date("2026-11-01",1)=="2026-11-02","next");});
 test("input validation",[]{EventInput i{"c","x","","2026-01-01","2026-01-02",true};std::string e;req(valid_event(i,e),"valid");i.end=i.start;req(!valid_event(i,e),"range");});
 test("all day exclusive",[]{CalendarEvent e;e.all_day=true;e.start="2026-10-26";e.end="2026-10-28";auto s=layout_week({e},week_containing(e.start));req(s.size()==2,"exclusive");});
 test("midnight split",[]{CalendarEvent e;e.start="2026-10-27T23:00:00+09:00";e.end="2026-10-28T01:00:00+09:00";auto s=layout_week({e},week_containing("2026-10-27"));req(s.size()==2&&s[0].end_minute==1440&&s[1].end_minute==60,"split");});
 test("week clipping",[]{CalendarEvent e;e.all_day=true;e.start="2026-10-20";e.end="2026-11-10";req(layout_week({e},week_containing("2026-10-27")).size()==7,"clip");});
 test("overlap columns",[]{CalendarEvent a;a.start="2026-10-27T09:00";a.end="2026-10-27T11:00";auto b=a;b.start="2026-10-27T10:00";b.end="2026-10-27T12:00";auto s=layout_week({a,b},week_containing("2026-10-27"));req(s[0].columns==2&&s[1].column==1,"overlap");});
 test("calendar list parse",[]{AuthStore a("/tmp/nonexistent");GoogleApi api(a,[](auto const&,auto const&,auto const&,auto const&){return HttpResponse{200,R"({"items":[{"id":"a","summary":"Work","accessRole":"reader"},{"id":"b","summary":"Mine","accessRole":"owner"}]})"};});auto v=api.calendars();req(v.size()==2&&!v[0].writable&&v[1].writable,"calendars");});
 test("event pagination",[]{int n=0;AuthStore a("/tmp/nonexistent");GoogleApi api(a,[&](auto const&,auto const&,auto const&,auto const&){++n;return HttpResponse{200,n==1?R"({"items":[{"id":"1","summary":"A","start":{"date":"2026-10-27"},"end":{"date":"2026-10-28"}}],"nextPageToken":"p"})":R"({"items":[{"id":"2","summary":"B","start":{"dateTime":"2026-10-27T09:00:00+09:00"},"end":{"dateTime":"2026-10-27T10:00:00+09:00"}}]})"};});auto v=api.events("c",week_containing("2026-10-27"));req(v.size()==2&&v[0].all_day&&!v[1].all_day,"events");});
 test("RFC3339 converts to Seoul",[]{AuthStore a("/tmp/nonexistent");GoogleApi api(a,[](auto const&,auto const&,auto const&,auto const&){return HttpResponse{200,R"({"items":[{"id":"1","summary":"A","start":{"dateTime":"2026-10-27T09:00:00Z"},"end":{"dateTime":"2026-10-27T10:00:00Z"}}]})"};});auto v=api.events("c",week_containing("2026-10-27"));req(v[0].start.find("18:00:00+09:00")!=std::string::npos,"timezone");});
 test("create body",[]{std::string body;AuthStore a("/tmp/nonexistent");GoogleApi api(a,[&](auto const&m,auto const&,auto const&b,auto const&){body=b;req(m=="POST","method");return HttpResponse{200,R"({"id":"x","summary":"T","start":{"date":"2026-10-27"},"end":{"date":"2026-10-28"}})"};});api.create({"c","T","","2026-10-27","2026-10-28",true});req(body.find("summary")!=std::string::npos,"body");});
 test("conflict",[]{AuthStore a("/tmp/nonexistent");GoogleApi api(a,[](auto const&,auto const&,auto const&,auto const&){return HttpResponse{412,""};});CalendarEvent e;e.id="e";e.calendar_id="c";e.etag="x";e.writable=true;try{api.update(e,{"c","T","","2026-10-27","2026-10-28",true});throw std::runtime_error("accepted");}catch(const std::runtime_error&x){req(std::string(x.what()).find("412")!=std::string::npos,"message");}});
 test("recurring readonly",[]{AuthStore a("/tmp/nonexistent");GoogleApi api(a,[](auto const&,auto const&,auto const&,auto const&){return HttpResponse{200,"{}"};});CalendarEvent e;e.recurring=true;e.writable=true;try{api.remove(e);throw std::runtime_error("removed");}catch(const std::runtime_error&){};});
 std::cout<<ok<<" passed, "<<fail<<" failed\n";return fail?1:0;}
