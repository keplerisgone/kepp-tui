#include "calendar/calendar.hpp"
#include "calendar/week_view.hpp"
#include <exception>
#include <iostream>
namespace kepp::calendar {int run(){try{return run_week_view();}catch(const std::exception&e){std::cerr<<"Calendar: "<<e.what()<<'\n';return 1;}}}
