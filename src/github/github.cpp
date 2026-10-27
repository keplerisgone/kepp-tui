#include "github/github.hpp"
#include "github/overview.hpp"
#include <exception>
#include <iostream>
namespace kepp::github {int run(){try{return run_overview();}catch(const std::exception&e){std::cerr<<"GitHub Console: "<<e.what()<<'\n';return 1;}}}
