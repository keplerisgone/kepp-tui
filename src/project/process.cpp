#include "project/process.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <fcntl.h>
#include <signal.h>
#include <thread>
#include <stdexcept>

#include <sys/wait.h>
#include <unistd.h>

namespace kepp::project {
namespace {

std::vector<char *> argv_for(const std::vector<std::string> &arguments) {
  std::vector<char *> result;
  result.reserve(arguments.size() + 1);
  for (const auto &argument : arguments)
    result.push_back(const_cast<char *>(argument.c_str()));
  result.push_back(nullptr);
  return result;
}

int wait_for(pid_t child) {
  int status = 0;
  while (::waitpid(child, &status, 0) < 0) {
    if (errno != EINTR)
      throw std::runtime_error("Cannot wait for child process: " +
                               std::string(std::strerror(errno)));
  }
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return -1;
}

} // namespace

ProcessResult run_captured(const std::vector<std::string> &arguments,
                           const std::filesystem::path &directory,
                           std::chrono::milliseconds timeout) {
  if (arguments.empty() || arguments.front().empty())
    throw std::runtime_error("Process executable is empty");
  int descriptors[2];
  if (::pipe(descriptors) != 0)
    throw std::runtime_error("Cannot create process pipe");
  const auto child = ::fork();
  if (child < 0) {
    ::close(descriptors[0]);
    ::close(descriptors[1]);
    throw std::runtime_error("Cannot fork process");
  }
  if (child == 0) {
    ::setpgid(0, 0);
    ::close(descriptors[0]);
    ::dup2(descriptors[1], STDOUT_FILENO);
    ::dup2(descriptors[1], STDERR_FILENO);
    ::close(descriptors[1]);
    if (::chdir(directory.c_str()) != 0)
      _exit(126);
    auto argv = argv_for(arguments);
    ::execvp(argv[0], argv.data());
    _exit(errno == ENOENT ? 127 : 126);
  }
  ::close(descriptors[1]);
  ::setpgid(child, child);
  const int flags = ::fcntl(descriptors[0], F_GETFL, 0);
  ::fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK);
  ProcessResult result;
  std::array<char, 4096> buffer{};
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  int status = 0;
  bool finished = false;
  while (!finished) {
    ssize_t count = 0;
    while ((count = ::read(descriptors[0], buffer.data(), buffer.size())) > 0)
      result.output.append(buffer.data(), static_cast<std::size_t>(count));
    const auto waited = ::waitpid(child, &status, WNOHANG);
    if (waited == child) finished = true;
    else if (waited < 0 && errno != EINTR) {
      ::close(descriptors[0]);
      throw std::runtime_error("Cannot wait for child process: " + std::string(std::strerror(errno)));
    } else if (std::chrono::steady_clock::now() >= deadline) {
      ::kill(-child, SIGTERM);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (::waitpid(child, &status, WNOHANG) == 0) ::kill(-child, SIGKILL);
      while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
      result.exit_code = 124;
      result.output += "\nProcess timed out";
      finished = true;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  ssize_t count = 0;
  while ((count = ::read(descriptors[0], buffer.data(), buffer.size())) > 0)
    result.output.append(buffer.data(), static_cast<std::size_t>(count));
  ::close(descriptors[0]);
  if (result.exit_code != 124) {
    if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) result.exit_code = 128 + WTERMSIG(status);
  }
  return result;
}

int run_interactive(const ProjectCommand &command,
                    const std::filesystem::path &directory) {
  std::vector<std::string> arguments{command.executable};
  arguments.insert(arguments.end(), command.arguments.begin(),
                   command.arguments.end());
  if (command.executable.empty())
    throw std::runtime_error("Process executable is empty");
  const auto child = ::fork();
  if (child < 0)
    throw std::runtime_error("Cannot fork process");
  if (child == 0) {
    if (::chdir(directory.c_str()) != 0)
      _exit(126);
    auto argv = argv_for(arguments);
    ::execvp(argv[0], argv.data());
    _exit(errno == ENOENT ? 127 : 126);
  }
  return wait_for(child);
}

} // namespace kepp::project
