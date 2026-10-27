cmake_minimum_required(VERSION 3.20)
project(kepp_tui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
  ftxui
  URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v6.1.9.tar.gz
  URL_HASH SHA256=45819c1e54914783d4a1ca5633885035d74146778a1f74e1213cdb7b76340e71
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
  URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  tomlplusplus
  URL https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.tar.gz
  URL_HASH SHA256=8517f65938a4faae9ccf8ebb36631a38c1cadfb5efa85d9a72e15b9e97d25155
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(ftxui tomlplusplus nlohmann_json)

add_library(kepp_project_core
  src/project/process.cpp
  src/project/git.cpp
  src/project/paths.cpp
  src/project/registry.cpp
  src/project/service.cpp
)
target_include_directories(kepp_project_core PUBLIC src)
target_link_libraries(kepp_project_core PUBLIC tomlplusplus::tomlplusplus)
target_compile_options(kepp_project_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(kepp_github_core
  src/github/gh_client.cpp
  src/github/service.cpp
)
target_include_directories(kepp_github_core PUBLIC src)
target_link_libraries(kepp_github_core PUBLIC nlohmann_json::nlohmann_json tomlplusplus::tomlplusplus kepp_project_core)
target_compile_options(kepp_github_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(kepp_calendar_core
  src/calendar/time.cpp
  src/calendar/service.cpp
  src/calendar/google_api.cpp
  src/calendar/auth.cpp
)
target_include_directories(kepp_calendar_core PUBLIC src)
target_link_libraries(kepp_calendar_core PUBLIC nlohmann_json::nlohmann_json kepp_project_core)
target_compile_options(kepp_calendar_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(kepp
  src/main.cpp
  src/app.cpp
  src/project/project.cpp
  src/project/view.cpp
  src/github/github.cpp
  src/github/overview.cpp
  src/calendar/calendar.cpp
  src/calendar/week_view.cpp
  src/calendar/event_form.cpp
)
target_link_libraries(kepp PRIVATE
  kepp_project_core
  kepp_github_core
  kepp_calendar_core
  ftxui::screen
  ftxui::dom
  ftxui::component
)
target_compile_definitions(kepp PRIVATE KEPP_VERSION="${PROJECT_VERSION}")
target_compile_options(kepp PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS kepp RUNTIME DESTINATION bin)

enable_testing()
add_executable(project_tests tests/project/project_tests.cpp)
target_link_libraries(project_tests PRIVATE kepp_project_core)
target_compile_options(project_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME project_tests COMMAND project_tests)

add_executable(github_tests tests/github/github_tests.cpp)
target_link_libraries(github_tests PRIVATE kepp_github_core)
target_compile_options(github_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME github_tests COMMAND github_tests)

add_executable(calendar_tests tests/calendar/calendar_tests.cpp)
target_link_libraries(calendar_tests PRIVATE kepp_calendar_core)
target_compile_options(calendar_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME calendar_tests COMMAND calendar_tests)
add_test(NAME install_script_help COMMAND sh ${CMAKE_SOURCE_DIR}/install.sh --help)
