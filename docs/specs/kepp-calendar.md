# KEPP TUI — Calendar Module v0.1 구현 명세

MVP 확정

Specification v0.1 · 2026-10-27

목표: `kepp-tui` 안에서 Google Calendar를 조회·관리하는 버티컬 위클리 TUI를 구현합니다. 기존 Google Calendar를 데이터 원본으로 유지하고, 터미널에서 키보드만으로 주요 일정 작업을 완료할 수 있어야 합니다.

기존에 결정한 통합 구조를 유지하겠습니다.

```
kepp-tui
├── kepp project    # Project Manager
├── kepp github     # GitHub Console
└── kepp calendar   # Google Calendar
```

`obsidian-graph.nvim`은 별도의 저장소로 관리합니다.

## 1. 확정 범위

| ID     | 요구사항        | 완료 기준                       |
| ------ | ----------- | --------------------------- |
| CAL-01 | 7일 버티컬 위클리  | 월요일부터 일요일까지 시간표로 표시         |
| CAL-02 | 키보드 탐색      | 날짜·시간·일정 선택 및 주 이동          |
| CAL-03 | 다중 캘린더 조회   | 접근 가능한 캘린더 선택·표시            |
| CAL-04 | 일정 종류 구분    | 종일 일정과 시간 지정 일정 분리          |
| CAL-05 | 일회성 일정 CRUD | 생성·수정·삭제 후 서버 반영 확인         |
| CAL-06 | 수동 새로고침     | API 재조회, 실패 시 오류와 기존 데이터 표시 |

MVP에서 제외할 기능은 반복 일정 편집, 월간·Agenda 뷰, Google Tasks, 오프라인 편집, 실시간 동기화, 알림, 참석자 관리입니다.

반복 일정은 화면에 표시하되 수정·삭제하지 않는 것으로 처리합니다.

## 2. 화면 및 상호작용

KEPP / Calendar

W44 · 2026

October 26 – November 1

MON 26

TUE 27

WED 28

THU 29

FRI 30

SAT 31

SUN 01

ALL

SP 제출

08:00

09:00

TOEIC

9:00

10:00

11:00

상담

11:00

12:00

13:00

14:00

SP 과제

14:00

15:00

16:00

17:00

18:00

19:00

h/l: Day　H/L: Week　j/k: Time　a: Add　r: Refresh

레이아웃 개념도입니다. 클릭하여 날짜를 선택할 수 있으며, 표시된 일정은 예시입니다. 실제 구현에서는 일정의 정확한 길이·겹침·분 단위 시작 시각을 반영합니다.

실제 TUI는 화면을 다음 네 영역으로 나눕니다.

1. Header: 현재 주, 시간대, 선택된 캘린더, 동기화 상태
2. All-day Area: 종일 및 여러 날짜에 걸친 일정
3. Time Grid: 00:00\~24:00, 시간에 비례하는 일정 블록
4. Status Bar: 키 바인딩, 선택 상태, 오류 메시지

시간축은 한 시간 단위를 여러 터미널 행으로 표시하고 세로 스크롤을 지원합니다. 시작 시각이 정각이 아니어도 일정이 정확한 시간 슬롯에 대응되도록 하되, 터미널의 셀 단위 해상도로 인한 표시 오차는 허용합니다. 실제 시각은 상세 패널에 정확히 표시합니다.

같은 시간에 일정이 겹치면 서로 다른 열에 배치합니다. 공간이 부족하면 일정이 겹쳤음을 표시하고 선택·상세 조회를 가능하게 합니다.

## 3. 프로젝트 디렉터리 구조

`kepp-tui` 저장소 내 Calendar 영역을 다음처럼 구성하겠습니다.

```
kepp-tui/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app.cpp
│   │
│   └── calendar/
│       ├── calendar.cpp
│       ├── calendar.hpp
│       ├── model.hpp
│       ├── week_view.cpp
│       ├── week_view.hpp
│       ├── event_form.cpp
│       ├── event_form.hpp
│       ├── service.cpp
│       ├── service.hpp
│       ├── google_api.cpp
│       ├── google_api.hpp
│       ├── auth.cpp
│       └── auth.hpp
│
├── tests/
│   └── calendar/
│       ├── model_test.cpp
│       ├── week_view_test.cpp
│       ├── service_test.cpp
│       └── google_api_test.cpp
│
└── docs/
    └── calendar.md
```

총괄 앱과 공유 가능한 라이브러리는 기존 `app.cpp`와 빌드 시스템에서 관리합니다. Calendar 전용 코드는 `src/calendar` 밖으로 옮기지 않습니다.

첫 버전에 repository 계층, 범용 Event Bus, 플러그인 로더, 캐시 데이터베이스는 만들지 않습니다.

## 4. 모듈별 책임

| 파일 / 모듈          | 책임                             | 포함하지 않을 책임       |
| ---------------- | ------------------------------ | ---------------- |
| `calendar.cpp`   | Calendar 모듈 시작, 상태 및 화면 연결     | Google API 직접 호출 |
| `model.hpp`      | Calendar·Event·시간 데이터 타입       | 렌더링              |
| `week_view.cpp`  | 7일 화면, 일정 배치, 선택·스크롤           | 서버 변경            |
| `event_form.cpp` | 일정 생성·수정 입력 폼과 입력 검증           | HTTP 요청          |
| `service.cpp`    | 주간 데이터 조회 조정, CRUD 규칙, 오류 상태   | UI 위젯 생성         |
| `google_api.cpp` | Calendar REST 요청·JSON 변환·응답 처리 | 사용자 입력           |
| `auth.cpp`       | OAuth 인증, 토큰 갱신·보관             | 일정 데이터 처리        |

### 주요 데이터 모델

```
struct CalendarInfo {
    std::string id;
    std::string name;
    std::string color;
    bool selected;
    bool writable;
};

struct CalendarEvent {
    std::string id;
    std::string calendar_id;
    std::string title;
    std::string description;
    std::string etag;

    bool all_day;
    bool recurring;
    bool writable;

    std::string start;
    std::string end;
};
```

`start`와 `end`는 API 원본을 보존하는 표현으로 시작하되, 모델 계층에서 날짜 전용 값과 시간대가 있는 시각을 구분해 파싱해야 합니다. 단순 문자열 비교로 날짜 정렬이나 일정 겹침을 판단하지 않습니다.

`etag`는 다른 클라이언트가 일정을 수정했을 때 충돌을 감지하기 위해 사용합니다. Google Calendar API는 `If-Match` 조건이 일치하지 않는 요청에 412 오류를 반환할 수 있습니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://developers.google.com\&sz=32)

Google for Developers



## 5. Google Calendar 연동 명세

### 5.1 API 호출

| 작업       | Google Calendar API | 처리            |
| -------- | ------------------- | ------------- |
| 캘린더 조회   | `calendarList.list` | 접근 가능한 캘린더 목록 |
| 주간 일정 조회 | `events.list`       | 선택된 캘린더마다 호출  |
| 일정 생성    | `events.insert`     | 신규 일정 생성      |
| 일정 수정    | `events.patch`      | 변경할 필드만 갱신    |
| 일정 삭제    | `events.delete`     | 확인 후 삭제       |

`events.list`는 `timeMin`, `timeMax`, `singleEvents`, `pageToken` 등을 지원합니다. 조회 범위는 선택한 주의 월요일 00:00부터 다음 월요일 00:00까지로 정의하겠습니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://developers.google.com\&sz=32)

Google for Developers



주간 조회 처리 흐름은 다음과 같습니다.

```
Open Calendar
      |
      v
Load calendar list
      |
      v
Select enabled calendars
      |
      v
Fetch weekly events
      |
      v
Expand recurring instances
      |
      v
Normalize event dates
      |
      v
Merge + sort events
      |
      v
Render weekly view
```

모든 페이지를 조회하고, 캘린더별 결과를 합친 뒤 렌더링합니다. 한 캘린더에서 조회가 실패했다고 다른 캘린더까지 실패 처리하지 않으며, 부분 조회임을 화면에 표시합니다.

### 5.2 시간 처리 규칙

가장 중요한 부분입니다.

- 기본 표시 시간대는 `Asia/Seoul`입니다.
- API의 RFC 3339 타임스탬프는 정확한 시각으로 변환한 후 표시 시간대에 맞춥니다.
- 종일 일정은 날짜 기반으로 처리합니다.
- 종일 일정의 종료일은 미포함(exclusive)입니다.
- 여러 날짜에 걸친 일정은 해당 주에서 차지하는 범위만 표시합니다.
- 자정을 넘는 일정은 날짜 경계에서 분할 렌더링합니다.
- 일광절약시간제를 사용하는 다른 시간대의 일정도 정확한 순간을 기준으로 변환합니다.

Google Calendar API의 `events.insert` 명세에서도 종일 일정은 `date`, 시간 지정 일정은 `dateTime`으로 구분합니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://developers.google.com\&sz=32)

Google for Developers



### 5.3 인증 및 접근 권한

OAuth 2.0 Desktop App 인증 방식을 사용합니다.

최초 실행 시 인증을 진행하고, 이후에는 보관된 자격 증명으로 토큰을 갱신합니다.

인증 범위는 개인 일정 조회와 편집에 필요한 최소 권한으로 제한합니다. 토큰은 로그에 기록하지 않고, 사용자만 접근 가능한 저장소에 보관합니다. macOS Keychain과 Linux Secret Service 지원은 검토하되, 둘을 동시에 구현하는 것이 부담된다면 첫 버전에서는 권한이 제한된 로컬 자격 증명 파일을 사용합니다.

읽기 전용 캘린더는 조회만 허용하고, 수정 및 삭제 동작을 비활성화합니다.

## 6. 키보드 명세

| 키         | 동작                  |
| --------- | ------------------- |
| `h` / `l` | 이전·다음 날짜            |
| `H` / `L` | 이전·다음 주             |
| `j` / `k` | 선택 시간 위·아래 이동       |
| `Tab`     | 종일 영역과 시간표 전환       |
| `Enter`   | 선택된 일정 상세 보기        |
| `a`       | 일정 추가               |
| `e`       | 선택된 일정 수정           |
| `d`       | 선택된 일정 삭제           |
| `r`       | 현재 주 재조회            |
| `t`       | 오늘로 이동              |
| `q`       | Calendar 종료 후 메인 메뉴 |
| `Esc`     | 현재 폼·상세 패널 닫기       |

텍스트 입력 폼이 열렸을 때는 Vim 스타일 단축키보다 텍스트 입력을 우선합니다.

일정 삭제에는 별도의 확인 단계가 필요하며, 저장하지 않은 수정은 `Esc`로 취소할 수 있어야 합니다.

## 7. 오류 처리 명세

| 상황              | 기대 동작                    |
| --------------- | ------------------------ |
| 네트워크 연결 실패      | 기존 일정 유지, 연결 오류 표시       |
| 인증 만료           | 토큰 갱신 후 재시도, 실패 시 재인증 안내 |
| 403 권한 부족       | 편집 중단, 권한 오류 표시          |
| 403 / 429 호출 제한 | 제한된 횟수의 지수 백오프 후 결과 표시   |
| 404 일정 없음       | 삭제·변경 가능성 안내, 새로고침 유도    |
| 412 수정 충돌       | 덮어쓰지 않고 서버 일정 재조회        |
| 5xx 서버 오류       | 제한된 재시도 후 실패 상태 표시       |
| 잘못된 일정 입력       | 요청 전 검증하고 오류 필드 표시       |
| 부분 캘린더 조회 실패    | 성공한 캘린더 표시, 불완전한 결과 경고   |

Google은 호출 제한과 일시적인 서버 오류에 지수 백오프를 권장합니다. 다만 쓰기 요청의 결과가 불명확할 때 무조건 재시도하지 않아야 합니다. 예를 들어 일정 생성 요청이 서버에서 처리되었으나 응답만 유실됐다면 중복 생성될 수 있기 때문입니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://developers.google.com\&sz=32)

Google for Developers



수동 새로고침은 현재 선택된 주와 캘린더를 기준으로 서버를 재조회하며, 실패한 요청 때문에 이전 정상 데이터를 삭제하지 않습니다.

## 8. 테스트 명세

아래는 구현 시 CI에서 자동 검증할 테스트 항목입니다.

M1 — 모델·시간 계산

6 tests

- T01: 임의의 날짜에서 해당 주의 월요일·다음 월요일 계산
- T02: 일요일에서 다음 주로 이동
- T03: 종일 일정의 종료일 미포함 처리
- T04: 자정 통과 일정의 날짜별 분할
- T05: 서로 다른 시간대의 일정 변환
- T06: 주간 경계와 겹치는 일정 필터링

M2 — 화면·입력

6 tests

- T07: 항상 7개 날짜 열 렌더링
- T08: 종일 일정과 시간 지정 일정의 분리
- T09: 일정 충돌 시 겹침 여부와 각 일정 접근성 확인
- T10: 날짜·주 이동 후 선택 상태 유지
- T11: 시간축 스크롤 및 터미널 크기 변화
- T12: 한글·긴 제목 렌더링 및 잘림 처리

M3 — Calendar API

6 tests

- T13: 다중 캘린더 목록 및 선택
- T14: 선택 캘린더만 조회
- T15: 여러 페이지의 일정 모두 수집
- T16: 반복 일정 인스턴스 조회 및 편집 차단
- T17: JSON 응답 누락·잘못된 데이터 처리
- T18: 읽기 전용 캘린더의 쓰기 요청 차단

M4 — CRUD·오류

8 tests

- T19: 일회성 시간 지정 일정 생성
- T20: 일회성 종일 일정 생성
- T21: 일정 수정 후 서버 반영 확인
- T22: 삭제 확인·취소 및 실제 삭제
- T23: 잘못된 시간 범위 입력 차단
- T24: 네트워크 실패 후 기존 일정 유지
- T25: 401·403·429·5xx 응답별 처리
- T26: 412 동시 수정 충돌 시 덮어쓰기 방지

총 26개 테스트 항목입니다. 모두 앞으로 구현·실행해야 할 검증 조건이며, 현재 통과한 테스트를 의미하지 않습니다.

### 테스트 전략

단위 테스트에서는 실제 Google API를 호출하지 않고, 고정된 JSON 응답과 가짜 Calendar API 구현을 사용합니다. 그래야 네트워크 상태나 계정 권한에 영향을 받지 않고 반복 검증할 수 있습니다.

실제 API 통합 테스트는 별도의 테스트 캘린더에서 수행합니다. 테스트용 일정만 생성·수정·삭제하며, 개인 캘린더의 기존 일정은 건드리지 않습니다.

## 9. 구현 마일스톤 및 완료 기준

| 마일스톤    | 작업                         | 통과 조건               |
| ------- | -------------------------- | ------------------- |
| M1      | Calendar 진입점·데이터 모델·주간 화면  | T01\~T12            |
| M2      | OAuth·Calendar API 읽기 연동   | T13\~T18            |
| M3      | 일정 생성·수정·삭제 폼              | T19\~T23            |
| M4      | 오류·복구 및 통합 검증              | T24\~T26 및 실 API 검증 |
| Release | macOS/Linux 빌드·tmux 사용성 확인 | 두 OS에서 전체 기본 흐름 성공  |

권장 도구는 C++20, CMake, FTXUI, libcurl, nlohmann/json과 Catch2입니다. 최초 구현은 mock 데이터와 실제 API의 경계를 분리하여, 각 마일스톤을 독립적으로 검증할 수 있도록 합니다.

## 10. 최종 Definition of Done

다음 시나리오가 모두 성공하면 Calendar v0.1이 완료된 것으로 판단합니다.

사용자 인수 테스트

1. `kepp calendar` 실행 시 현재 주의 버티컬 위클리가 열립니다.
2. 서로 다른 Google 캘린더 두 개 이상의 일정을 한 화면에 표시할 수 있습니다.
3. 키보드만으로 날짜·시간·일정을 탐색할 수 있습니다.
4. 종일 일정과 시간 지정 일정이 서로 다른 영역에 표시됩니다.
5. 일정을 생성·수정·삭제한 결과가 Google Calendar 웹에서도 일치합니다.
6. 외부에서 수정한 일정이 `r` 새로고침으로 반영됩니다.
7. 네트워크가 끊겨도 프로그램이 종료되지 않으며 오류가 표시됩니다.
8. macOS, Linux 및 tmux에서 기본 동작을 완료할 수 있습니다.

### 명세 확정 시 유지할 결정

- 저장소: `kepp-tui`
- 실행 명령: `kepp calendar`
- 기본 화면: 7일 버티컬 위클리
- 데이터 원본: Google Calendar
- 동기화: 사용자의 수동 새로고침
- 반복 일정: 읽기 전용
- 오프라인 변경: 지원하지 않음
- 일정 편집: 일회성 일정에 한정
- 개발 원칙: 공통 프레임워크를 과도하게 설계하지 않고 Calendar 모듈부터 완성

이 명세라면 KEPP LAB의 Codex 개발 에이전트에 M1\~M4를 순서대로 맡기고, 각 단계에서 테스트 통과 여부를 기준으로 결과물을 검수할 수 있습니다.

핵심은 M1에서 Google API를 연결하지 않고도 완성된 버티컬 위클리를 검증할 수 있어야 한다는 것입니다. 그래야 UI 구현 문제와 인증·네트워크 문제를 분리하여 개발할 수 있습니다.

참고 공식 문서: [Events.list](https://developers.google.com/workspace/calendar/api/v3/reference/events/list?utm_source=chatgpt.com) · [Events.insert](https://developers.google.com/workspace/calendar/api/v3/reference/events/insert?utm_source=chatgpt.com) · [Calendar API Error Handling](https://developers.google.com/workspace/calendar/api/guides/errors?utm_source=chatgpt.com)
