좋습니다. 별도의 TUI 프로젝트 세 개를 관리하기보다는 `kepp-tui`라는 하나의 애플리케이션 안에 세 개의 독립 모듈을 두는 구조가 더 적합하겠습니다.

사용자 관점에서는 하나의 프로그램이지만, 개발 관점에서는 각 모듈을 독립적으로 구현하고 테스트할 수 있도록 구성하는 것입니다.

# KEPP TUI — 통합 프로젝트 설계 v0.1

## 1. 실행 방식

두 가지 진입점을 지원합니다.

① 인자 없이 실행 — 메인 대시보드

```
kepp
```

KEPP TUI

Main Menu

Select a module

Project Manager프로젝트 탐색 · 실행 · 작업 환경 전환GitHub ConsoleRepository · Issues · PR · ActionsGoogle Calendar버티컬 위클리 · 일정 생성 및 편집

COMMAND

kepp project

&#x20;Copy

메인 화면 인터랙티브 목업입니다. 모듈을 선택해 실행 명령을 확인할 수 있습니다.

② 인자로 실행 — 해당 모듈에 직접 진입

```
kepp project
kepp github
kepp calendar
```

짧은 별칭도 고려할 수 있습니다.

```
kepp p
kepp g
kepp c
```

다만 MVP에서는 정식 서브커맨드 세 개만 구현해도 충분합니다.

## 2. 프로젝트 구조

```
kepp-tui/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app.cpp
│   │
│   ├── project/
│   │   ├── project.cpp
│   │   └── project.hpp
│   │
│   ├── github/
│   │   ├── github.cpp
│   │   └── github.hpp
│   │
│   └── calendar/
│       ├── calendar.cpp
│       └── calendar.hpp
│
├── tests/
└── README.md
```

처음에는 공통 프레임워크나 플러그인 시스템을 만들지 않습니다. `main.cpp`에서 인자를 해석하고 해당 모듈을 실행하는 구조면 충분합니다.

| 모듈              | 담당 기능                    | 외부 의존성              |
| --------------- | ------------------------ | ------------------- |
| Project Manager | 프로젝트 등록·검색·이동·실행         | 로컬 파일시스템, Git       |
| GitHub Console  | Issue·PR·Actions 조회 및 관리 | `gh` CLI            |
| Google Calendar | 주간 일정 조회 및 CRUD          | Google Calendar API |
| 공통 UI           | 메인 메뉴·키보드 이벤트·렌더링        | FTXUI               |

개발 언어는 C++20으로 통일하고, 각 모듈은 필요할 때 독립적으로 확장하도록 하겠습니다.

## 3. 각 모듈의 MVP

M1. Project Manager

프로젝트 등록·삭제·검색, 디렉터리 이동, Neovim 및 Codex 실행, 프로젝트별 사용자 명령 관리.

완료 기준: 등록한 프로젝트를 선택해 올바른 디렉터리에서 작업을 시작할 수 있음.

M2. GitHub Console

저장소 선택, Issue·PR 목록과 상세 내용 확인, GitHub Actions 결과 조회.

완료 기준: `gh` 인증을 재사용하고 여러 저장소의 실제 GitHub 데이터를 조회할 수 있음. 변경 기능은 후속 구현.

M3. Google Calendar

버티컬 위클리 화면, 종일·시간 지정 일정 표시, 일정 추가·수정·삭제.

완료 기준: Google Calendar와 일정 내용이 일치하며 일회성 일정의 CRUD가 정상 작동함.

## 4. 통합 설계에서 주의할 점

첫째, 셸 디렉터리 이동입니다. `kepp project`가 종료된 뒤에도 현재 셸의 디렉터리를 변경하려면 셸 함수가 필요합니다. 따라서 `kepp project --select`처럼 선택한 경로만 출력하는 인터페이스를 별도로 두는 것이 좋겠습니다.

둘째, 모듈 간 결합은 최소화합니다. Project Manager가 GitHub Console을 여는 편의 기능은 추가할 수 있지만, GitHub 연동이 실패해도 프로젝트 탐색은 정상적으로 작동해야 합니다. Google 인증이 없어도 나머지 모듈은 사용 가능해야 합니다.

셋째, 메인 메뉴는 단순하게 유지합니다. 처음부터 모든 모듈의 최신 상태를 수집하는 종합 대시보드로 만들지 않습니다. 세 모듈을 선택하는 런처만으로도 첫 버전의 목적을 달성합니다.

## 5. 개발 순서와 검증

| 단계 | 작업                | 검증                       |
| -- | ----------------- | ------------------------ |
| 1  | `kepp` 실행기와 메인 메뉴 | 세 서브커맨드 정상 진입            |
| 2  | Project Manager   | 프로젝트 등록·이동·실행            |
| 3  | GitHub Console    | `gh` 데이터 조회              |
| 4  | Google Calendar   | 주간 렌더링 및 API CRUD        |
| 5  | 통합 테스트            | macOS·Linux·tmux에서 정상 동작 |

최종적으로 프로젝트 구성을 아래처럼 정리하면 되겠습니다.

KEPP LAB — Software Projects

kepp-tui

C++20

├── Project Manager

├── GitHub Console

└── Google Calendar

obsidian-graph.nvim

Lua

obsidian.nvim 확장용 독립 플러그인

이렇게 하면 하나의 저장소, 하나의 빌드 시스템, 하나의 실행 파일로 세 TUI를 관리하면서도, 각 기능을 개별 마일스톤으로 완성할 수 있습니다.

추후 KEPP LAB의 Codex 개발 자동화에 맡길 때도 `project`, `github`, `calendar`를 별도 작업 영역으로 지정하기 편한 구조입니다.
