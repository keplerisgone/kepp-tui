#include "app.hpp"

#include "project/project.hpp"
#include "github/github.hpp"
#include "calendar/calendar.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <iostream>
#include <string_view>

namespace kepp {

int run_app(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "project") {
    return project::run();
  }

  if (argc == 2 && std::string_view(argv[1]) == "github")
    return github::run();
  if (argc == 2 && std::string_view(argv[1]) == "calendar")
    return calendar::run();

  if (argc == 1) {
    using namespace ftxui;
    while (true) {
    int selected = 0, chosen = -1;
    std::vector<std::string> entries = {"Project Launcher", "GitHub Console", "Google Calendar", "Quit"};
    auto screen = ScreenInteractive::Fullscreen();
    auto menu = Menu(&entries, &selected);
    auto component = Renderer(menu, [&] {
      static const std::vector<std::string> descriptions = {
          "Projects, Git status, and safe commands",
          "Repositories, Issues, PRs, and Actions",
          "Seven-day view and one-off event CRUD", "Exit KEPP TUI"};
      return vbox({text("KEPP TUI v1.0") | bold | color(Color::Cyan),
                   text("Select a module"), separator(), menu->Render() | frame,
                   separator(), paragraph(descriptions[selected]),
                   text("Enter: Open · q: Quit") | dim}) |
             border | center;
    });
    component = CatchEvent(component, [&](Event event) {
      if (event == Event::Return) { chosen = selected; screen.Exit(); return true; }
      if (event == Event::Character('q')) { chosen = 3; screen.Exit(); return true; }
      return false;
    });
    screen.Loop(component);
    if (chosen == 0) project::run();
    else if (chosen == 1) github::run();
    else if (chosen == 2) calendar::run();
    else return 0;
    }
  }

  std::cerr << "Usage: kepp [project|github|calendar]\n";
  return 2;
}

} // namespace kepp
