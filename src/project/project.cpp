#include "project/project.hpp"

#include "project/service.hpp"
#include "project/view.hpp"

#include <iostream>

namespace kepp::project {

int run() {
  try {
    ProjectService service;
    service.load();
    return run_view(service);
  } catch (const std::exception &error) {
    std::cerr << "Project Launcher: " << error.what() << '\n';
    return 1;
  }
}

} // namespace kepp::project
