#include "calendar/event_form.hpp"
namespace kepp::calendar {EventInput event_input_from(const CalendarEvent&e){return{e.calendar_id,e.title,e.description,e.start,e.end,e.all_day};}}
