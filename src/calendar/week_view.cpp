#include "calendar/week_view.hpp"
#include "calendar/service.hpp"
#include "calendar/time.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace kepp::calendar {
using namespace ftxui;
namespace {std::string hour_text(int hour){std::ostringstream out;out<<std::setw(2)<<std::setfill('0')<<hour<<":00";return out.str();}}

int run_week_view() {
  std::time_t now=std::time(nullptr);std::tm local=*std::localtime(&now);char today_buf[11];std::strftime(today_buf,sizeof today_buf,"%Y-%m-%d",&local);
  std::string selected_date=today_buf,status="Loading calendars...",title,description,start,end;
  auto week=week_containing(selected_date);CalendarService service;
  int selected=0,calendar_i=0;bool form=false,editing=false,confirm=false,calendar_panel=false,detail=false,all_day=false;
  auto load=[&]{if(service.calendars().empty()&&!service.refresh_calendars()){status=service.error();return;}service.refresh_week(week);status=service.error().empty()?"Updated":service.error();selected=0;};
  load();

  auto screen=ScreenInteractive::Fullscreen();
  auto title_in=Input(&title,"Title"),desc_in=Input(&description,"Description"),start_in=Input(&start,"Start"),end_in=Input(&end,"End");
  auto fields=Container::Vertical({title_in,desc_in,start_in,end_in});
  auto renderer=Renderer(fields,[&] {
    if(form)return vbox({text(editing?"Edit event":"Add event")|bold,
      hbox({text("Title       ")|size(WIDTH,EQUAL,12),title_in->Render()}),
      hbox({text("Description ")|size(WIDTH,EQUAL,12),desc_in->Render()}),
      hbox({text("Start       ")|size(WIDTH,EQUAL,12),start_in->Render()}),
      hbox({text("End         ")|size(WIDTH,EQUAL,12),end_in->Render()}),
      text(std::string("Type: ")+(all_day?"all-day":"timed")+" (Ctrl-T toggles)"),
      text("Ctrl-S Save · Esc Cancel"),text(status)|color(Color::Red)})|border|center;
    if(calendar_panel){Elements rows={text("Calendars (j/k, Space toggle)")|bold};for(int i=0;i<(int)service.calendars().size();++i){auto&c=service.calendars()[i];rows.push_back(text(std::string(i==calendar_i?"> ":"  ")+(c.selected?"[x] ":"[ ] ")+c.name+(c.writable?"":" [read-only]")));}return vbox({text("KEPP / Calendar")|bold,vbox(rows)|border|center,text("c/Esc: Apply and close")|dim});}
    if(confirm)return vbox({text("Delete selected event?")|bold,text("y: delete · n/Esc: cancel")})|border|center;
    if(detail&&selected<(int)service.events().size()){auto&e=service.events()[selected];return vbox({text("Event detail")|bold,text(e.title),separator(),text(e.start+" — "+e.end),paragraph(e.description),text(e.calendar_id)|dim,text(e.recurring?"Recurring · read-only":e.writable?"Editable":"Read-only"),separator(),text("Esc/Enter: Back")})|border;}

    auto segments=layout_week(service.events(),week);Elements columns;
    for(int day=0;day<7;++day){auto date=shift_date(week.monday,day);Elements rows;
      std::string all="ALL ";for(auto&s:segments)if(s.day==day&&s.event.all_day)all+=s.event.title+"  ";rows.push_back(paragraph(all)|color(Color::Yellow));rows.push_back(separator());
      for(int hour=0;hour<24;++hour){std::string line=hour_text(hour)+" ";bool chosen=false;for(auto&s:segments)if(s.day==day&&!s.event.all_day&&s.start_minute/60==hour){line+=(s.start_minute%60?std::to_string(s.start_minute%60)+"m ":"")+s.event.title+" ";if(selected<(int)service.events().size()&&s.event.id==service.events()[selected].id)chosen=true;}Element row=paragraph(line);if(chosen)row=row|inverted|focus;rows.push_back(row);}
      columns.push_back(vbox({text(date+(date==selected_date?" *":""))|bold,separator(),vbox(rows)|frame|flex})|flex|border);
    }
    return vbox({text("KEPP / Calendar")|bold|color(Color::Cyan),text(week.monday+" — "+shift_date(week.next_monday,-1)+" · Asia/Seoul"),hbox(columns)|flex,separator(),paragraph(status),text("h/l Day · H/L Week · j/k Event · Enter Detail · a Add · e Edit · d Delete · c Calendars · r Refresh · t Today · q Back")|dim});
  });

  auto component=CatchEvent(renderer,[&](Event event){
    if(form){if(event==Event::Escape){form=false;return true;}if(event==Event::Character('\x14')){all_day=!all_day;return true;}if(event==Event::Character('\x13')){EventInput in;in.title=title;in.description=description;in.start=start;in.end=end;in.all_day=all_day;for(auto&c:service.calendars())if(c.selected&&c.writable){in.calendar_id=c.id;break;}bool ok=editing&&selected<(int)service.events().size()?service.update_event(service.events()[selected],in):service.create_event(in);status=service.error();if(ok){form=false;load();}return true;}return false;}
    if(confirm){if(event==Event::Character('y')){if(selected<(int)service.events().size())service.delete_event(service.events()[selected]);confirm=false;load();return true;}if(event==Event::Character('n')||event==Event::Escape){confirm=false;return true;}return false;}
    if(detail){if(event==Event::Escape||event==Event::Return){detail=false;return true;}return false;}
    if(calendar_panel){if(event==Event::Escape||event==Event::Character('c')){calendar_panel=false;load();return true;}if((event==Event::Character('j')||event==Event::ArrowDown)&&calendar_i+1<(int)service.calendars().size()){++calendar_i;return true;}if((event==Event::Character('k')||event==Event::ArrowUp)&&calendar_i>0){--calendar_i;return true;}if(event==Event::Character(' ')&&calendar_i<(int)service.calendars().size()){service.calendars()[calendar_i].selected=!service.calendars()[calendar_i].selected;return true;}return false;}
    if(event==Event::Character('q')){screen.Exit();return true;}if(event==Event::Character('r')){load();return true;}if(event==Event::Character('c')){calendar_panel=true;calendar_i=0;return true;}
    if(event==Event::Character('h')){selected_date=shift_date(selected_date,-1);if(selected_date<week.monday){week=week_containing(selected_date);load();}return true;}if(event==Event::Character('l')){selected_date=shift_date(selected_date,1);if(selected_date>=week.next_monday){week=week_containing(selected_date);load();}return true;}
    if(event==Event::Character('H')){week={shift_date(week.monday,-7),week.monday};selected_date=shift_date(selected_date,-7);load();return true;}if(event==Event::Character('L')){week={week.next_monday,shift_date(week.next_monday,7)};selected_date=shift_date(selected_date,7);load();return true;}if(event==Event::Character('t')){selected_date=today_buf;week=week_containing(selected_date);load();return true;}
    if(event==Event::Character('j')&&selected+1<(int)service.events().size()){++selected;return true;}if(event==Event::Character('k')&&selected>0){--selected;return true;}if(event==Event::Return&&selected<(int)service.events().size()){detail=true;return true;}
    if(event==Event::Character('a')){editing=false;title=description="";start=selected_date+"T09:00:00+09:00";end=selected_date+"T10:00:00+09:00";all_day=false;form=true;return true;}
    if(event==Event::Character('e')&&selected<(int)service.events().size()){auto&x=service.events()[selected];if(!x.writable){status="Recurring or read-only event cannot be edited";return true;}editing=true;title=x.title;description=x.description;start=x.start;end=x.end;all_day=x.all_day;form=true;return true;}
    if(event==Event::Character('d')&&selected<(int)service.events().size()){if(service.events()[selected].writable)confirm=true;else status="Recurring or read-only event cannot be deleted";return true;}return false;
  });
  screen.Loop(component);return 0;
}
} // namespace kepp::calendar
