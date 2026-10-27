#include "project/git.hpp"
#include "project/paths.hpp"
#include "project/process.hpp"
#include "project/registry.hpp"
#include "project/service.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;
using namespace kepp::project;

namespace {

struct TempDir {
  fs::path path;
  TempDir() {
    path = fs::temp_directory_path() /
           ("kepp-project-test-" + std::to_string(::getpid()) + "-XXXXXX");
    std::string value = path.string();
    value.push_back('\0');
    char *created = ::mkdtemp(value.data());
    if (!created)
      throw std::runtime_error("mkdtemp failed");
    path = created;
  }
  ~TempDir() {
    std::error_code error;
    fs::remove_all(path, error);
  }
};

void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <typename Function>
void require_throws(Function function, const std::string &message) {
  try {
    function();
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error(message);
}

void write(const fs::path &path, const std::string &contents) {
  fs::create_directories(path.parent_path());
  std::ofstream output(path);
  output << contents;
  if (!output)
    throw std::runtime_error("fixture write failed");
}

ProjectInput input(std::string name, const fs::path &path) {
  return {std::move(name), path.string(), "description", "owner/repo"};
}

} // namespace

int main() {
  std::vector<std::pair<std::string, std::function<void()>>> tests;

  tests.push_back({"missing registry is empty", [] {
                     TempDir temp;
                     Registry registry(temp.path / "projects.toml");
                     require(registry.load().projects.empty(),
                             "expected empty registry");
                   }});
  tests.push_back(
      {"normal TOML loads optional fields and commands", [] {
         TempDir temp;
         auto project_dir = temp.path / "alpha";
         fs::create_directory(project_dir);
         auto file = temp.path / "projects.toml";
         write(file, "version=1\n[[projects]]\nname='alpha'\npath='" +
                         project_dir.string() +
                         "'\ndescription='desc'\nrepository='owner/"
                         "repo'\n[projects.commands]\ntest=['make','test']\n");
         auto data = Registry(file).load();
         require(data.projects.size() == 1, "project missing");
         require(data.projects[0].commands.at("test").arguments ==
                     std::vector<std::string>{"test"},
                 "command wrong");
       }});
  tests.push_back({"registration persists", [] {
                     TempDir temp;
                     auto dir = temp.path / "alpha";
                     fs::create_directory(dir);
                     auto file = temp.path / "projects.toml";
                     ProjectService service{Registry(file)};
                     service.load();
                     service.add(input("alpha", dir));
                     ProjectService reloaded{Registry(file)};
                     reloaded.load();
                     require(reloaded.data().projects.size() == 1,
                             "not persisted");
                   }});
  tests.push_back({"canonical duplicate is rejected", [] {
                     TempDir temp;
                     auto dir = temp.path / "alpha";
                     fs::create_directory(dir);
                     auto alias = temp.path / "alias";
                     fs::create_directory_symlink(dir, alias);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     service.add(input("alpha", dir));
                     require_throws([&] { service.add(input("alias", alias)); },
                                    "duplicate accepted");
                   }});
  tests.push_back({"unregister does not delete directory", [] {
                     TempDir temp;
                     auto dir = temp.path / "alpha";
                     fs::create_directory(dir);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     service.add(input("alpha", dir));
                     service.unregister_project(fs::canonical(dir));
                     require(fs::is_directory(dir), "directory deleted");
                     require(service.data().projects.empty(), "entry remains");
                   }});
  tests.push_back({"malformed TOML is rejected and preserved", [] {
                     TempDir temp;
                     auto file = temp.path / "projects.toml";
                     write(file, "not = [valid");
                     require_throws([&] { Registry(file).load(); },
                                    "malformed TOML accepted");
                     std::ifstream input(file);
                     std::string value((std::istreambuf_iterator<char>(input)),
                                       {});
                     require(value == "not = [valid", "malformed file changed");
                   }});
  tests.push_back({"unsupported schema is rejected", [] {
                     TempDir temp;
                     auto file = temp.path / "projects.toml";
                     write(file, "version=2\n");
                     require_throws([&] { Registry(file).load(); },
                                    "unsupported version accepted");
                   }});
  tests.push_back({"external registry change blocks save", [] {
                     TempDir temp;
                     auto file = temp.path / "projects.toml";
                     write(file, "version=1\n");
                     Registry registry(file);
                     auto data = registry.load();
                     write(file, "version=1\n# changed\n");
                     auto changed_time =
                         fs::last_write_time(file) + std::chrono::seconds(1);
                     fs::last_write_time(file, changed_time);
                     require_throws([&] { registry.save(data); },
                                    "external change overwritten");
                   }});
  tests.push_back({"tilde expands", [] {
                     const char *home = std::getenv("HOME");
                     require(home != nullptr, "HOME absent");
                     require(expand_user_path("~/hello") ==
                                 fs::path(home) / "hello",
                             "tilde wrong");
                   }});
  tests.push_back({"spaces and Korean paths normalize", [] {
                     TempDir temp;
                     auto dir = temp.path / "한글 project";
                     fs::create_directory(dir);
                     require(normalize_project_path(dir.string()) ==
                                 fs::canonical(dir),
                             "unicode path wrong");
                   }});
  tests.push_back({"missing path state", [] {
                     TempDir temp;
                     require(inspect_path(temp.path / "missing") ==
                                 PathState::missing,
                             "missing state wrong");
                   }});
  tests.push_back({"regular file is not directory", [] {
                     TempDir temp;
                     auto file = temp.path / "file";
                     write(file, "x");
                     require(inspect_path(file) == PathState::not_directory,
                             "file state wrong");
                   }});
  tests.push_back({"search covers name path and description", [] {
                     TempDir temp;
                     auto alpha = temp.path / "alpha";
                     auto beta = temp.path / "beta";
                     fs::create_directory(alpha);
                     fs::create_directory(beta);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     service.add({"Alpha", alpha.string(), "Rocket", {}});
                     service.add({"Beta", beta.string(), "Database", {}});
                     require(service.filtered_indices("ROCKET").size() == 1,
                             "description search failed");
                     require(service.filtered_indices("beta").size() == 1,
                             "name/path search failed");
                   }});
  tests.push_back(
      {"projects sort case-insensitively", [] {
         TempDir temp;
         auto a = temp.path / "a";
         auto z = temp.path / "z";
         fs::create_directory(a);
         fs::create_directory(z);
         ProjectService service{Registry(temp.path / "projects.toml")};
         service.load();
         service.add(input("zulu", z));
         service.add(input("Alpha", a));
         require(service.data().projects.front().name == "Alpha", "sort wrong");
       }});
  tests.push_back({"empty name is rejected", [] {
                     TempDir temp;
                     auto dir = temp.path / "a";
                     fs::create_directory(dir);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     require_throws([&] { service.add(input("", dir)); },
                                    "empty name accepted");
                   }});
  tests.push_back({"invalid repository is rejected", [] {
                     TempDir temp;
                     auto dir = temp.path / "a";
                     fs::create_directory(dir);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     auto value = input("a", dir);
                     value.repository = "not-a-repository";
                     require_throws([&] { service.add(value); },
                                    "invalid repository accepted");
                   }});
  tests.push_back(
      {"missing registration path is rejected", [] {
         TempDir temp;
         ProjectService service{Registry(temp.path / "projects.toml")};
         service.load();
         require_throws([&] { service.add(input("a", temp.path / "missing")); },
                        "missing path accepted");
       }});
  tests.push_back(
      {"commands survive a registry rewrite", [] {
         TempDir temp;
         auto dir = temp.path / "a";
         fs::create_directory(dir);
         auto file = temp.path / "projects.toml";
         write(file, "version=1\n[[projects]]\nname='a'\npath='" +
                         dir.string() +
                         "'\n[projects.commands]\ntest=['make','test']\n");
         Registry registry(file);
         auto data = registry.load();
         registry.save(data);
         auto again = Registry(file).load();
         require(again.projects[0].commands.at("test").executable == "make",
                 "command lost");
       }});
  tests.push_back({"registry override environment is honored", [] {
                     TempDir temp;
                     const auto path = temp.path / "custom.toml";
                     const char *previous = std::getenv("KEPP_PROJECTS_FILE");
                     const std::string saved = previous ? previous : "";
                     ::setenv("KEPP_PROJECTS_FILE", path.c_str(), 1);
                     require(default_registry_path() == path,
                             "override ignored");
                     if (previous)
                       ::setenv("KEPP_PROJECTS_FILE", saved.c_str(), 1);
                     else
                       ::unsetenv("KEPP_PROJECTS_FILE");
                   }});
  tests.push_back({"repository field is optional", [] {
                     TempDir temp;
                     auto dir = temp.path / "a";
                     fs::create_directory(dir);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     service.add({"a", dir.string(), {}, {}});
                     require(!service.data().projects[0].repository,
                             "empty repository retained");
                   }});
  tests.push_back({"search can produce no results", [] {
                     TempDir temp;
                     auto dir = temp.path / "a";
                     fs::create_directory(dir);
                     ProjectService service{
                         Registry(temp.path / "projects.toml")};
                     service.load();
                     service.add({"alpha", dir.string(), {}, {}});
                     require(service.filtered_indices("does-not-match").empty(),
                             "unexpected result");
                   }});
  tests.push_back(
      {"refresh keeps missing project registered", [] {
         TempDir temp;
         auto dir = temp.path / "a";
         fs::create_directory(dir);
         ProjectService service{Registry(temp.path / "projects.toml")};
         service.load();
         service.add({"alpha", dir.string(), {}, {}});
         fs::remove(dir);
         service.refresh_paths();
         require(service.data().projects.size() == 1,
                 "missing project removed");
         require(service.data().projects[0].path_state == PathState::missing,
                 "missing state not refreshed");
       }});
  tests.push_back(
      {"registry is owner-readable and owner-writable only", [] {
         TempDir temp;
         auto dir = temp.path / "a";
         fs::create_directory(dir);
         auto file = temp.path / "projects.toml";
         ProjectService service{Registry(file)};
         service.load();
         service.add({"alpha", dir.string(), {}, {}});
         const auto permissions = fs::status(file).permissions();
         require((permissions & fs::perms::owner_read) != fs::perms::none,
                 "owner read missing");
         require((permissions & fs::perms::owner_write) != fs::perms::none,
                 "owner write missing");
         require((permissions & (fs::perms::group_all |
                                 fs::perms::others_all)) == fs::perms::none,
                 "registry too permissive");
       }});
  tests.push_back(
      {"unregistering unknown project is rejected", [] {
         TempDir temp;
         ProjectService service{Registry(temp.path / "projects.toml")};
         service.load();
         require_throws(
             [&] { service.unregister_project(temp.path / "unknown"); },
             "unknown unregister accepted");
       }});
  tests.push_back(
      {"argument text supports quotes and escapes", [] {
         const auto values =
             parse_argument_text("--flag \"two words\" escaped\\ value ''");
         require(values == std::vector<std::string>(
                               {"--flag", "two words", "escaped value", ""}),
                 "argument parsing failed");
       }});
  tests.push_back({"unfinished argument quote is rejected", [] {
                     require_throws([] { parse_argument_text("\"unfinished"); },
                                    "unfinished quote accepted");
                   }});
  tests.push_back(
      {"commands can be added and removed", [] {
         TempDir temp;
         auto dir = temp.path / "a";
         fs::create_directory(dir);
         auto file = temp.path / "projects.toml";
         ProjectService service{Registry(file)};
         service.load();
         service.add({"alpha", dir.string(), {}, {}});
         service.set_command(fs::canonical(dir), "test", {"make", {"test"}});
         ProjectService loaded{Registry(file)};
         loaded.load();
         require(loaded.data().projects[0].commands.contains("test"),
                 "command not saved");
         loaded.remove_command(fs::canonical(dir), "test");
         require(loaded.data().projects[0].commands.empty(),
                 "command not removed");
       }});
  tests.push_back(
      {"command executable whitespace is normalized", [] {
         TempDir temp;
         auto dir = temp.path / "a";
         fs::create_directory(dir);
         auto file = temp.path / "projects.toml";
         write(file, "version=1\n[[projects]]\nname='a'\npath='" +
                         dir.string() +
                         "'\n[projects.commands]\neditor=['  true  ']\n");
         ProjectService service{Registry(file)};
         service.load();
         const auto &command = service.data().projects[0].commands.at("editor");
         require(command.executable == "true",
                 "executable whitespace retained");
         require(run_interactive(command, dir) == 0,
                 "normalized command did not run");
       }});
  tests.push_back({"invalid command name is rejected", [] {
                     TempDir temp;
                     auto dir = temp.path / "a";
                     fs::create_directory(dir);
                     ProjectService service{Registry(temp.path / "p.toml")};
                     service.load();
                     service.add({"alpha", dir.string(), {}, {}});
                     require_throws(
                         [&] {
                           service.set_command(fs::canonical(dir), "bad name",
                                               {"true", {}});
                         },
                         "invalid command name accepted");
                   }});
  tests.push_back(
      {"captured process uses project working directory", [] {
         TempDir temp;
         auto result = run_captured({"pwd"}, temp.path);
         while (!result.output.empty() &&
                (result.output.back() == '\n' || result.output.back() == '\r'))
           result.output.pop_back();
         require(result.exit_code == 0 &&
                     fs::path(result.output) == fs::canonical(temp.path),
                 "working directory wrong");
       }});
  tests.push_back(
      {"missing executable returns 127", [] {
         TempDir temp;
         const auto result =
             run_captured({"kepp-command-that-does-not-exist"}, temp.path);
         require(result.exit_code == 127, "missing executable code wrong");
       }});
  tests.push_back({"captured process timeout reaps child", [] {
                     TempDir temp;
                     const auto started = std::chrono::steady_clock::now();
                     const auto result = run_captured(
                         {"sleep", "5"}, temp.path,
                         std::chrono::milliseconds(50));
                     require(result.exit_code == 124, "timeout code wrong");
                     require(std::chrono::steady_clock::now() - started <
                                 std::chrono::seconds(2),
                             "timeout did not terminate promptly");
                   }});
  tests.push_back({"non repository reports Git unavailable", [] {
                     TempDir temp;
                     const auto info = inspect_git(temp.path);
                     require(!info.repository && !info.error.empty(),
                             "non-repository misclassified");
                   }});
  tests.push_back(
      {"Git repository branch and status are read", [] {
         TempDir temp;
         require(run_captured({"git", "init", "-q"}, temp.path).exit_code == 0,
                 "git init failed");
         const auto info = inspect_git(temp.path);
         require(info.repository && !info.branch.empty() &&
                     !info.status.empty(),
                 "Git inspection failed");
       }});

  int failures = 0;
  for (const auto &[name, test] : tests) {
    try {
      test();
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
  }
  std::cout << tests.size() - failures << "/" << tests.size()
            << " tests passed\n";
  return failures == 0 ? 0 : 1;
}
