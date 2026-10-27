#include "project/view.hpp"

#include "project/git.hpp"
#include "project/paths.hpp"
#include "project/process.hpp"

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

namespace kepp::project {
namespace {

using namespace ftxui;

enum class Mode {
  browse,
  search,
  add_project,
  confirm_unregister,
  commands,
  command_form,
  confirm_remove_command
};

std::string compact_path(const Project &project) {
  return project.configured_path.empty() ? project.normalized_path.string()
                                         : project.configured_path;
}

Element status_element(PathState state) {
  auto value = text(path_state_text(state));
  return state == PathState::available ? value | color(Color::Green)
                                       : value | color(Color::Red);
}

std::string arguments_text(const ProjectCommand &command) {
  std::string result;
  for (const auto &argument : command.arguments) {
    if (!result.empty())
      result += ' ';
    if (argument.find_first_of(" \\t\"'") == std::string::npos)
      result += argument;
    else {
      result += '"';
      for (const char character : argument) {
        if (character == '"' || character == '\\')
          result += '\\';
        result += character;
      }
      result += '"';
    }
  }
  return result;
}

} // namespace

int run_view(ProjectService &service) {
  auto screen = ScreenInteractive::Fullscreen();
  Mode mode = Mode::browse;
  int active_tab = 0;
  int selected = 0;
  int command_selected = 0;
  std::string query;
  std::string message;
  ProjectInput project_draft;
  std::string command_name;
  std::string command_original_name;
  std::string command_executable;
  std::string command_arguments;
  std::optional<std::pair<std::filesystem::path, GitInfo>> git_info;

  std::vector<std::string> menu_entries;
  std::vector<std::size_t> visible;
  std::vector<std::string> command_entries;
  auto rebuild = [&] {
    visible = service.filtered_indices(query);
    menu_entries.clear();
    for (const auto index : visible) {
      const auto &project = service.data().projects[index];
      const auto marker =
          project.path_state == PathState::available ? "" : " !";
      menu_entries.push_back(project.name + "  " + compact_path(project) +
                             marker);
    }
    selected = menu_entries.empty()
                   ? 0
                   : std::clamp(selected, 0,
                                static_cast<int>(menu_entries.size() - 1));
    command_entries.clear();
    if (!visible.empty()) {
      for (const auto &[name, command] :
           service.data().projects[visible[selected]].commands)
        command_entries.push_back(name + "  " + command.executable);
    }
    command_selected =
        command_entries.empty()
            ? 0
            : std::clamp(command_selected, 0,
                         static_cast<int>(command_entries.size() - 1));
  };
  rebuild();

  auto current_project = [&]() -> const Project * {
    return visible.empty() ? nullptr
                           : &service.data().projects[visible[selected]];
  };
  auto selected_command_name = [&]() -> std::string {
    const auto *project = current_project();
    if (!project || project->commands.empty())
      return {};
    auto iterator = project->commands.begin();
    std::advance(iterator, command_selected);
    return iterator->first;
  };

  auto menu = Menu(&menu_entries, &selected);
  auto command_menu = Menu(&command_entries, &command_selected);
  auto search_input = Input(&query, "name, path, description");
  auto name_input = Input(&project_draft.name, "required");
  auto path_input = Input(&project_draft.path, "required");
  auto description_input = Input(&project_draft.description, "optional");
  auto repository_input =
      Input(&project_draft.repository, "OWNER/REPO (optional)");
  auto project_form = Container::Vertical(
      {name_input, path_input, description_input, repository_input});
  auto command_name_input = Input(&command_name, "letters, numbers, ._-");
  auto command_executable_input = Input(&command_executable, "e.g. nvim");
  auto command_arguments_input =
      Input(&command_arguments, "e.g. . or --flag \"two words\"");
  auto command_form = Container::Vertical(
      {command_name_input, command_executable_input, command_arguments_input});
  auto base = Container::Tab(
      {menu, search_input, project_form, command_menu, command_form},
      &active_tab);

  auto component = CatchEvent(
      Renderer(
          base,
          [&] {
            Element content;
            if (mode == Mode::add_project) {
              content =
                  vbox({text("Register project") | bold, separator(),
                        hbox({text("Name        ") | size(WIDTH, EQUAL, 12),
                              name_input->Render()}),
                        hbox({text("Path        ") | size(WIDTH, EQUAL, 12),
                              path_input->Render()}),
                        hbox({text("Description ") | size(WIDTH, EQUAL, 12),
                              description_input->Render()}),
                        hbox({text("Repository  ") | size(WIDTH, EQUAL, 12),
                              repository_input->Render()}),
                        separator(),
                        text("Tab Fields · Ctrl-S Save · Esc Cancel")}) |
                  border;
            } else if (mode == Mode::command_form) {
              content =
                  vbox({text(command_original_name.empty() ? "Add command"
                                                           : "Edit command") |
                            bold,
                        separator(),
                        hbox({text("Name       ") | size(WIDTH, EQUAL, 11),
                              command_name_input->Render()}),
                        hbox({text("Executable ") | size(WIDTH, EQUAL, 11),
                              command_executable_input->Render()}),
                        hbox({text("Arguments  ") | size(WIDTH, EQUAL, 11),
                              command_arguments_input->Render()}),
                        separator(),
                        text("Arguments are parsed without a shell."),
                        text("Tab Fields · Ctrl-S Save · Esc Cancel")}) |
                  border;
            } else if (mode == Mode::confirm_unregister ||
                       mode == Mode::confirm_remove_command) {
              const auto title =
                  mode == Mode::confirm_unregister
                      ? "Unregister project?"
                      : "Remove command '" + selected_command_name() + "'?";
              const auto warning =
                  mode == Mode::confirm_unregister
                      ? "Only the registry entry is removed. Files are never "
                        "deleted."
                      : "The project directory is not modified.";
              content = vbox({text(title) | bold, text(warning), separator(),
                              text("y Confirm · n/Esc Cancel")}) |
                        border;
            } else if (mode == Mode::commands) {
              const auto *project = current_project();
              auto list =
                  command_entries.empty()
                      ? text("No commands. Press a to add one.")
                      : command_menu->Render() | vscroll_indicator | frame;
              content = vbox({text("COMMANDS / " +
                                   std::string(project ? project->name : "")) |
                                  bold,
                              separator(), list | flex, separator(),
                              text("Enter Run · a Add · e Edit · d Remove · "
                                   "Esc Back")}) |
                        border;
            } else {
              Element detail;
              const auto *project = current_project();
              if (!project) {
                detail =
                    vbox({text("No matching projects") | bold,
                          text(service.data().projects.empty()
                                   ? "Press a to register the first project."
                                   : "Change or clear the search query.")});
              } else {
                Elements git_elements{separator(), text("GIT") | dim};
                if (git_info && git_info->first == project->normalized_path) {
                  const auto &info = git_info->second;
                  git_elements.push_back(
                      text(info.repository ? "Branch: " + info.branch
                                           : info.error) |
                      (info.repository ? color(Color::Green)
                                       : color(Color::Yellow)));
                  if (!info.status.empty())
                    git_elements.push_back(paragraph(info.status));
                } else {
                  git_elements.push_back(text("Press g to inspect Git status"));
                }
                Elements details{
                    text(project->name) | bold,
                    paragraph(project->description.empty()
                                  ? "No description"
                                  : project->description),
                    separator(),
                    text("DIRECTORY") | dim,
                    paragraph(project->normalized_path.string()),
                    separator(),
                    text("REPOSITORY") | dim,
                    text(project->repository.value_or("Not registered")),
                    separator(),
                    text("STATUS") | dim,
                    status_element(project->path_state)};
                details.insert(details.end(), git_elements.begin(),
                               git_elements.end());
                detail = vbox(std::move(details));
              }
              auto list = vbox({text("PROJECTS") | bold, separator(),
                                menu->Render() | vscroll_indicator | frame});
              content = hbox({list | size(WIDTH, EQUAL, 45) | border,
                              detail | flex | border});
              if (mode == Mode::search)
                content = vbox(
                    {hbox({text("Search: "), search_input->Render()}) | border,
                     content | flex});
            }
            auto status =
                message.empty()
                    ? text("j/k Select · Enter Commands · g Git · / Search · a "
                           "Add · d Unregister · q Back")
                    : text(message) | color(Color::Yellow);
            return vbox({text("KEPP / Projects v0.2") | bold | center,
                         separator(), content | flex, separator(), status});
          }),
      [&](Event event) {
        if (mode == Mode::add_project || mode == Mode::command_form) {
          if (event == Event::Escape) {
            mode = mode == Mode::add_project ? Mode::browse : Mode::commands;
            active_tab = mode == Mode::browse ? 0 : 3;
            project_draft = {};
            message.clear();
            return true;
          }
          if (event == Event::CtrlS) {
            try {
              if (mode == Mode::add_project) {
                service.add(project_draft);
                project_draft = {};
                query.clear();
                mode = Mode::browse;
                active_tab = 0;
                message = "Project registered";
              } else {
                const auto *project = current_project();
                if (!project)
                  throw std::runtime_error("No project selected");
                ProjectCommand command{command_executable,
                                       parse_argument_text(command_arguments)};
                if (!command_original_name.empty() &&
                    command_original_name != command_name)
                  throw std::runtime_error(
                      "Command names cannot be changed while editing; add a "
                      "new command instead");
                service.set_command(project->normalized_path, command_name,
                                    command);
                mode = Mode::commands;
                active_tab = 3;
                message = "Command saved";
              }
              rebuild();
            } catch (const std::exception &error) {
              message = error.what();
            }
            return true;
          }
          return false;
        }
        if (mode == Mode::confirm_unregister ||
            mode == Mode::confirm_remove_command) {
          if (event == Event::Character('y')) {
            try {
              const auto *project = current_project();
              if (!project)
                throw std::runtime_error("No project selected");
              if (mode == Mode::confirm_unregister) {
                service.unregister_project(project->normalized_path);
                mode = Mode::browse;
                active_tab = 0;
                message = "Project unregistered; no files were deleted";
              } else {
                service.remove_command(project->normalized_path,
                                       selected_command_name());
                mode = Mode::commands;
                active_tab = 3;
                message = "Command removed";
              }
              rebuild();
            } catch (const std::exception &error) {
              mode = Mode::browse;
              active_tab = 0;
              message = error.what();
            }
            return true;
          }
          if (event == Event::Character('n') || event == Event::Escape) {
            mode = mode == Mode::confirm_unregister ? Mode::browse
                                                    : Mode::commands;
            active_tab = mode == Mode::browse ? 0 : 3;
            message = "Cancelled";
            return true;
          }
          return true;
        }
        if (mode == Mode::search) {
          if (event == Event::Escape) {
            query.clear();
            mode = Mode::browse;
            active_tab = 0;
            rebuild();
            return true;
          }
          if (event == Event::Return) {
            mode = Mode::browse;
            active_tab = 0;
            return true;
          }
          const auto handled = search_input->OnEvent(event);
          rebuild();
          return handled;
        }
        if (mode == Mode::commands) {
          if (event == Event::Escape || event == Event::Character('q')) {
            mode = Mode::browse;
            active_tab = 0;
            return true;
          }
          if (event == Event::Character('a')) {
            command_name.clear();
            command_original_name.clear();
            command_executable.clear();
            command_arguments.clear();
            mode = Mode::command_form;
            active_tab = 4;
            return true;
          }
          if (event == Event::Character('e') && !command_entries.empty()) {
            const auto *project = current_project();
            command_name = selected_command_name();
            command_original_name = command_name;
            const auto &command = project->commands.at(command_name);
            command_executable = command.executable;
            command_arguments = arguments_text(command);
            mode = Mode::command_form;
            active_tab = 4;
            return true;
          }
          if (event == Event::Character('d') && !command_entries.empty()) {
            mode = Mode::confirm_remove_command;
            return true;
          }
          if (event == Event::Return && !command_entries.empty()) {
            const auto *project = current_project();
            const auto name = selected_command_name();
            const auto command = project->commands.at(name);
            int exit_code = -1;
            screen.WithRestoredIO([&] {
              exit_code = run_interactive(command, project->normalized_path);
            })();
            message = name + " exited with code " + std::to_string(exit_code);
            return true;
          }
          return command_menu->OnEvent(event);
        }

        if (event == Event::Character('q')) {
          screen.Exit();
          return true;
        }
        if (event == Event::Character('/')) {
          mode = Mode::search;
          active_tab = 1;
          query.clear();
          rebuild();
          return true;
        }
        if (event == Event::Character('a')) {
          mode = Mode::add_project;
          active_tab = 2;
          project_draft = {};
          message.clear();
          return true;
        }
        if (event == Event::Character('d') && current_project()) {
          mode = Mode::confirm_unregister;
          return true;
        }
        if (event == Event::Character('r')) {
          service.refresh_paths();
          message = "Path status refreshed";
          rebuild();
          return true;
        }
        if (event == Event::Character('g') && current_project()) {
          const auto path = current_project()->normalized_path;
          git_info = std::make_pair(path, inspect_git(path));
          message = git_info->second.repository ? "Git status refreshed"
                                                : git_info->second.error;
          return true;
        }
        if (event == Event::Character('j'))
          return menu->OnEvent(Event::ArrowDown);
        if (event == Event::Character('k'))
          return menu->OnEvent(Event::ArrowUp);
        if (event == Event::Return && current_project()) {
          mode = Mode::commands;
          active_tab = 3;
          command_selected = 0;
          rebuild();
          return true;
        }
        return menu->OnEvent(event);
      });

  screen.Loop(component);
  return 0;
}

} // namespace kepp::project
