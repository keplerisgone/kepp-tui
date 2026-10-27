# KEPP TUI 1.0

KEPP TUI is a C++20 terminal workspace with three independent modules:

- `kepp project` — local project registration, Git status, and safe commands
- `kepp github` — read-only repositories, Issues, pull requests, and Actions
- `kepp calendar` — Google Calendar seven-day view and one-off event CRUD
- `kepp` — interactive module selector

## Build and install

Requirements are CMake 3.20+, a C++20 compiler, Git, and `curl`. GitHub Console also needs the
GitHub CLI (`gh`). CMake downloads checksum-pinned FTXUI, toml++, and nlohmann/json sources on the
first configure. It does not invoke Homebrew, apt, or sudo.

```sh
make build
make test
make release
make install PREFIX="$HOME/.local"
```

Then ensure `$HOME/.local/bin` is on `PATH` and run `kepp`. Without installing, run
`./build/kepp` or a module such as `./build/kepp project`.

The build is POSIX-oriented and supports macOS and Linux. Windows is not currently supported.

## Project Launcher

Projects live in `$XDG_CONFIG_HOME/kepp/projects.toml` (normally
`~/.config/kepp/projects.toml`). `KEPP_PROJECTS_FILE` overrides the complete path.

Register a project with `a`, enter name/path/description/repository, and press `Ctrl-S`. Press
Enter on a project for its commands; `a` adds a command and `Ctrl-S` saves it. A command is stored
as an executable and argument list and starts in the project directory. For Neovim use executable
`nvim` and argument `.` (do not add a trailing space to the executable).

```toml
version = 1

[[projects]]
name = "mini-shell"
path = "~/workspace/mini-shell"
description = "Unix shell implementation"
repository = "keplerisgone/mini-shell"

[projects.commands]
editor = ["nvim", "."]
agent = ["codex"]
test = ["make", "test"]
```

Commands do not pass through a shell, so pipes, redirects, substitutions, and expansion are not
interpreted. Removing a registration never deletes its directory.

## GitHub Console

Authenticate the official CLI first, then open the module:

```sh
gh auth login
gh auth status
kepp github
```

Press `a` to register `OWNER/REPO`, `d` to unregister it, `Tab` to switch Issues/PRs/Actions,
Enter for details, and `r` to refresh. Data access is read-only. Repository preferences live at
`$XDG_CONFIG_HOME/kepp/github.toml`; `KEPP_GITHUB_FILE` overrides the path. Commands have a
30-second timeout and are executed as argument arrays, not shell text.

## Google Calendar

Create an OAuth 2.0 Desktop client in Google Cloud, enable Google Calendar API, and grant the
`https://www.googleapis.com/auth/calendar` scope. KEPP reads credentials from
`$XDG_CONFIG_HOME/kepp/calendar-credentials.json` (normally
`~/.config/kepp/calendar-credentials.json`), or from `KEPP_CALENDAR_CREDENTIALS`.

```json
{
  "access_token": "",
  "refresh_token": "YOUR_REFRESH_TOKEN",
  "client_id": "YOUR_DESKTOP_CLIENT_ID",
  "client_secret": "YOUR_DESKTOP_CLIENT_SECRET"
}
```

The file is rewritten with mode `0600` after token refresh. For a short-lived session,
`KEPP_GOOGLE_ACCESS_TOKEN` may supply an access token without a file. KEPP never prints tokens.
The OAuth consent/browser exchange itself is external; token refresh and Calendar API access
happen inside KEPP.

Use `h/l` for days, `H/L` for weeks, `j/k` for events, `c` for calendar selection, `a/e/d` for
one-off event CRUD, `r` to refresh, and `t` for today. Recurring and read-only events cannot be
edited or deleted. Calendar writes use ETags so a concurrent edit returns a conflict instead of
being overwritten.

## Data and safety

- GitHub is read-only in 1.0.
- Calendar deletes require confirmation.
- Network refresh errors preserve the last successful in-memory data.
- Configuration and credentials are local; secrets are not logged.
- Test suites use fixtures/fake runners and do not access or mutate real accounts.
