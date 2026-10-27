# KEPP TUI — GitHub Console v0.1 구현 명세

Specification v0.1 · 2026-10-27

MVP 설계

목표: 여러 GitHub 저장소의 Issue, Pull Request, GitHub Actions를 하나의 터미널 인터페이스에서 조회하고, 개발 진행 상황과 검토가 필요한 작업을 빠르게 파악하는 GitHub Console을 구현합니다.

앞서 확정한 `kepp-tui`의 두 번째 모듈이며, Calendar와 동일하게 C++20 기반으로 개발합니다.

```
kepp-tui
├── kepp project     # Project Manager
├── kepp github      # GitHub Console
└── kepp calendar    # Google Calendar
```

이번 v0.1은 읽기 전용 개발 현황 대시보드로 정의하겠습니다. Issue 생성·수정, PR 승인·병합, 워크플로 재실행은 후속 버전에서 구현합니다.

이렇게 구분하면 첫 릴리스에서 GitHub에 의도치 않은 변경을 일으키지 않으면서 API 통합과 TUI 탐색을 검증할 수 있습니다.

## 1. 메인 인터페이스

GitHub Console의 기본 화면은 Repository Overview입니다.

KEPP / GitHub Console

Overview

REPOSITORIES

kepp-labI:5PR:2CI:1mini-shellI:3PR:1network-labI:4PR:0

SELECTED REPOSITORY

kepp-lab

## 5

Issues

## 2

PRs

## 1

CI Fail

PRsIssuesActions

\#42 Add project registry

Draft

\#39 Fix build script

Checks OK

j/k Select · Tab Views · Enter Details · / Search · r Refresh · q Back

인터랙티브 화면 시안입니다. 저장소를 선택하고 Issues/PRs/Actions 탭을 전환할 수 있습니다. 표시된 수치와 항목은 예시이며 실제 GitHub 데이터가 아닙니다.

기본 구조는 세 영역입니다.

- Repository Panel: 등록된 GitHub 저장소와 요약 상태
- Content Panel: 현재 저장소의 Issue·PR·Actions 목록
- Detail View: 선택한 항목의 상세 내용과 관련 상태

최초 실행 시 모든 정보를 한꺼번에 불러오는 대신, 저장소 목록을 먼저 조회하고 선택한 저장소의 상세 데이터를 가져옵니다. 여러 저장소의 요약 상태는 이후 단계적으로 로딩할 수 있습니다.

## 2. 기능 요구사항

| ID    | 기능                  | 구체적 동작                  | v0.1 |
| ----- | ------------------- | ----------------------- | ---- |
| GH-01 | 저장소 목록              | 접근 가능한 저장소 조회 및 선택      | 필수   |
| GH-02 | 저장소 등록              | 관심 저장소 등록·해제            | 필수   |
| GH-03 | Repository Overview | 저장소 설명, 기본 브랜치, 상태 요약   | 필수   |
| GH-04 | Issue 목록            | Open/Closed, 라벨 필터 및 검색 | 필수   |
| GH-05 | Issue 상세            | 본문, 담당자, 라벨, 댓글 조회      | 필수   |
| GH-06 | PR 목록               | Open/Draft/Merged 상태 표시 | 필수   |
| GH-07 | PR 상세               | 본문, 브랜치, 리뷰 상태, 변경 파일   | 필수   |
| GH-08 | PR Checks           | PR별 CI 검사 결과 표시         | 필수   |
| GH-09 | Actions 목록          | 최근 워크플로 실행과 결과          | 필수   |
| GH-10 | Actions 상세          | 실행 상태와 실패 로그 조회         | 필수   |
| GH-11 | 키보드 조작              | 패널 이동, 탐색, 검색, 필터       | 필수   |
| GH-12 | 수동 새로고침             | 현재 화면의 서버 데이터 재조회       | 필수   |
| GH-13 | 오류 처리               | 인증·네트워크·CLI 실패 대응       | 필수   |
| GH-14 | Issue 생성·수정         | 제목·본문·라벨·상태 변경          | 후속   |
| GH-15 | PR Review·Merge     | Diff 확인, 승인, 병합         | 후속   |
| GH-16 | Actions 제어          | 워크플로 실행·재실행·취소          | 후속   |
| GH-17 | GitHub Projects     | 프로젝트 보드와 필드 관리          | 후속   |
| GH-18 | Codex 연동            | Issue → 에이전트 작업 위임      | 후속   |

v0.1의 핵심은 GitHub 전체 기능을 복제하는 것이 아니라, 여러 저장소를 관리할 때 실제로 확인해야 할 정보에 집중하는 것입니다.

## 3. `gh` CLI 연동 명세

GitHub Console은 별도 OAuth 인증을 구현하지 않고 설치된 `gh`의 인증 상태와 JSON 출력을 이용합니다.

공식 CLI에서 `--json`을 지원하는 명령은 필요한 필드만 지정해 구조화된 데이터를 받을 수 있습니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://cli.github.com\&sz=32)

Take GitHub to the command line

+1



### Repository

```
gh repo list keplerisgone \
  --json nameWithOwner,description,isPrivate,url \
  --limit 100

gh repo view keplerisgone/kepp-lab \
  --json nameWithOwner,description,defaultBranchRef,url
```

`gh repo list`는 지정한 사용자·조직이 소유한 저장소를 가져옵니다. 접근 권한이 있는 모든 저장소를 자동으로 열거한다는 뜻은 아닙니다. 따라서 다른 소유자의 저장소는 `OWNER/REPO` 형식으로 직접 등록할 수 있도록 합니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://cli.github.com\&sz=32)

Take GitHub to the command line

+1



### Issues

```
gh issue list \
  -R keplerisgone/kepp-lab \
  --state open \
  --json number,title,state,labels,assignees,updatedAt,url

gh issue view 42 \
  -R keplerisgone/kepp-lab \
  --json number,title,body,labels,assignees,comments,url
```

### Pull Requests

```
gh pr list \
  -R keplerisgone/kepp-lab \
  --state all \
  --json number,title,state,isDraft,headRefName,reviewDecision,url

gh pr view 42 \
  -R keplerisgone/kepp-lab \
  --json number,title,body,files,reviewDecision,statusCheckRollup
```

PR의 Draft 상태와 리뷰 결정은 서로 다른 값으로 관리합니다. 검사 결과 역시 리뷰 승인과 구분합니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://cli.github.com\&sz=32)

Take GitHub to the command line

+1



### GitHub Actions

```
gh run list \
  -R keplerisgone/kepp-lab \
  --limit 20 \
  --json databaseId,workflowName,status,conclusion,headBranch,url

gh run view 123456 \
  -R keplerisgone/kepp-lab \
  --log-failed
```

GitHub Actions의 실행 상태(`queued`, `in_progress`, `completed` 등)와 결론(`success`, `failure` 등)은 별개 필드로 처리합니다. 완료되지 않은 실행을 성공·실패로 단정하면 안 됩니다.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://cli.github.com\&sz=32)

Take GitHub to the command line



명령 실행은 셸 문자열을 조립하는 방식이 아니라 인자 배열을 전달하는 프로세스 실행 방식으로 구현합니다. 특히 저장소명·검색어·이슈 제목이 셸에서 재해석되지 않도록 해야 합니다.

## 4. 프로젝트 디렉터리 구조

Calendar와 같은 패턴을 따르되, GitHub 전용 기능은 `src/github`에 격리합니다.

```
kepp-tui/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app.cpp
│   │
│   ├── project/
│   │   └── ...
│   │
│   ├── github/
│   │   ├── github.cpp
│   │   ├── github.hpp
│   │   ├── model.hpp
│   │   ├── overview.cpp
│   │   ├── overview.hpp
│   │   ├── detail_view.cpp
│   │   ├── detail_view.hpp
│   │   ├── service.cpp
│   │   ├── service.hpp
│   │   ├── gh_client.cpp
│   │   ├── gh_client.hpp
│   │   ├── process.cpp
│   │   └── process.hpp
│   │
│   └── calendar/
│       └── ...
│
├── tests/
│   └── github/
│       ├── model_test.cpp
│       ├── overview_test.cpp
│       ├── service_test.cpp
│       ├── gh_client_test.cpp
│       └── process_test.cpp
│
└── docs/
    └── github.md
```

기본 데이터 계층은 다음과 같이 구성합니다.

GitHub Console UI

Overview · Issues · PRs · Actions

GitHub Service

저장소 선택 · 필터 · 데이터 상태

GH Client

명령어 생성 · 실행 결과 변환

gh CLI → GitHub API

인증 · REST / GraphQL · 네트워크

### 모듈별 책임

| 모듈                | 담당 책임                        | 담당하지 않는 것     |
| ----------------- | ---------------------------- | ------------- |
| `github.cpp`      | GitHub Console 초기화와 UI 구성    | CLI 응답 파싱     |
| `model.hpp`       | 저장소·Issue·PR·Workflow Run 모델 | 통신·렌더링        |
| `overview.cpp`    | 저장소 목록·탭·키보드 탐색              | GitHub 명령 실행  |
| `detail_view.cpp` | Issue·PR·Run 상세 화면           | 데이터 변경        |
| `service.cpp`     | 조회 조정·필터·로딩·오류 상태            | 터미널 그리기       |
| `gh_client.cpp`   | `gh` 인자 구성·JSON 파싱           | 화면 선택 상태      |
| `process.cpp`     | 프로세스 실행·종료 코드·타임아웃           | GitHub 데이터 해석 |

`process.cpp`는 우선 GitHub 모듈 내부에 둡니다. 다른 모듈에서도 실제로 필요한 시점에만 공통 영역으로 옮기는 것이 좋습니다.

## 5. 데이터 모델

외부 API의 필드를 그대로 화면에서 다루지 않고, TUI가 필요한 값으로 정규화합니다.

```
struct Repository {
    std::string name_with_owner;
    std::string description;
    std::string default_branch;
    std::string url;
    bool is_private;
};

struct Issue {
    int number;
    std::string title;
    std::string body;
    std::string state;
    std::vector<std::string> labels;
    std::string updated_at;
    std::string url;
};

struct PullRequest {
    int number;
    std::string title;
    std::string state;
    bool is_draft;
    std::string head_branch;
    std::string review_decision;
    std::string url;
};

struct WorkflowRun {
    std::int64_t id;
    std::string workflow_name;
    std::string status;
    std::string conclusion;
    std::string branch;
    std::string url;
};
```

위 구조는 핵심 목록 데이터의 예시입니다. 댓글·검사 결과·변경 파일 등의 상세 정보는 필요할 때 별도 조회합니다.

특히 다음 상태를 혼동하지 않아야 합니다.

- PR 상태: `OPEN`, `CLOSED`, `MERGED`
- PR Draft 여부: `is_draft`
- 리뷰 상태: 승인·변경 요청·검토 필요
- Actions 실행 상태: 대기·실행 중·완료
- Actions 결과: 성공·실패·취소·건너뜀 등

값을 모두 문자열로 저장하는 초기 설계는 가능하지만, 파싱 이후 서비스 계층에서는 enum이나 구분된 상태 타입을 사용하는 편이 오분류를 방지합니다.

## 6. 키보드 조작 명세

| 키         | 동작                                     |
| --------- | -------------------------------------- |
| `j` / `k` | 목록에서 아래·위 이동                           |
| `h` / `l` | 이전·다음 패널                               |
| `Tab`     | Repository / Issues / PRs / Actions 전환 |
| `Enter`   | 항목 상세 정보 열기                            |
| `Esc`     | 상세 화면 닫기·상위 화면 복귀                      |
| `/`       | 현재 목록 검색                               |
| `f`       | 상태·라벨 필터                               |
| `r`       | 현재 화면 새로고침                             |
| `o`       | 선택 항목을 브라우저에서 열기                       |
| `q`       | KEPP 메인 메뉴로 돌아가기                       |

검색 입력 중에는 단축키를 일반 텍스트로 취급합니다.

필터는 첫 버전에서 다음으로 제한합니다.

| 화면            | 필터                                  |
| ------------- | ----------------------------------- |
| Repositories  | 이름 검색                               |
| Issues        | Open / Closed / All, 라벨             |
| Pull Requests | Open / Closed / Merged / All, Draft |
| Actions       | 전체 / 실패 / 실행 중                      |

목록의 페이지 크기는 처음에 30개로 제한하고, 추가 항목을 요청하는 `Load More` 동작을 지원합니다. GitHub 전체 데이터를 처음부터 가져오지 않습니다.

## 7. 상태 및 오류 처리

Calendar 모듈과 동일하게, 오류가 발생해도 애플리케이션이 종료되지 않는 것을 원칙으로 합니다.

| 상황               | 기대 동작                          |
| ---------------- | ------------------------------ |
| `gh` 미설치         | 설치 필요 안내, 다른 KEPP 모듈은 정상 동작    |
| 미인증              | `gh auth login` 안내             |
| 인증 만료·권한 부족      | 오류 메시지 표시, 인증 정보 노출 금지         |
| 네트워크 끊김          | 기존 성공 데이터 유지, 최신 조회 실패 표시      |
| Repository 접근 불가 | 404 또는 권한 문제를 구분할 수 있는 범위에서 안내 |
| Rate Limit       | 제한 메시지 및 수동 재시도                |
| JSON 파싱 실패       | 잘못된 응답 표시, 빈 목록으로 오인하지 않음      |
| CLI 비정상 종료       | 종료 코드와 정제된 오류 메시지 표시           |
| 요청 지연            | 로딩 상태 유지, UI 입력 반응성 보장         |
| 일부 저장소 조회 실패     | 성공한 저장소 데이터는 계속 표시             |

프로세스 실행에는 타임아웃과 취소 처리가 필요합니다. 단, 타임아웃 시 자식 프로세스가 백그라운드에 남지 않도록 종료 및 회수까지 처리해야 합니다.

다중 저장소 상태 조회는 제한된 병렬 작업으로 처리하거나, 선택된 저장소부터 점진적으로 로딩합니다. 무제한 병렬 호출은 하지 않습니다.

보안 규칙: 토큰·환경변수의 비밀값을 로그에 기록하지 않으며, `gh` 호출에 사용자 입력을 셸 명령 문자열로 삽입하지 않습니다.

## 8. KEPP LAB 운영과의 연결

GitHub Console이 일반적인 GitHub 클라이언트와 달라질 수 있는 부분은 AI 개발 작업의 검토 인터페이스입니다.

예를 들어 KEPP LAB에서는 다음과 같은 흐름이 가능합니다.

```
GitHub Issue
    |
    | label: agent:ready
    v
KEPP LAB Worker
    |
    | Implement + Test
    v
Draft Pull Request
    |
    | CI + Review
    v
GitHub Console
    |
    | Human inspection
    v
Approve / Merge
```

v0.1에서는 위 과정 중 조회와 검토에 집중합니다.

특히 `agent:ready` 라벨을 가진 Issue, Draft PR, 실패한 Actions 실행을 손쉽게 찾을 수 있도록 필터를 제공하면 좋겠습니다.

다만 실제로 Codex 에이전트를 시작하거나 PR을 자동 승인하는 기능은 구현하지 않습니다. 이는 Console 자체보다 KEPP LAB의 작업 실행기와 승인 정책이 담당해야 하기 때문입니다.

## 9. 테스트 명세

총 30개 테스트 항목으로 나누겠습니다. 단위 테스트는 실제 GitHub 계정에 접근하지 않고, 고정 JSON과 가짜 프로세스 실행기를 사용합니다.

M1 — CLI 및 모델 검증

T01–T07 · 7개

- T01: `gh` 설치 여부 확인
- T02: 인증 상태 정상·실패 처리
- T03: `gh` 명령의 인자 배열 정확성
- T04: 정상 JSON 응답 파싱
- T05: 잘못된 JSON 및 필수 필드 누락 처리
- T06: 프로세스 종료 코드·표준 오류 처리
- T07: 타임아웃 발생 시 자식 프로세스 종료

M2 — Repository / Issue

T08–T14 · 7개

- T08: 여러 저장소 조회 및 표시
- T09: 수동 등록한 `OWNER/REPO` 조회
- T10: 등록·해제 후 설정 유지
- T11: 저장소 선택 시 상세 내용 갱신
- T12: Issue 상태별 필터링
- T13: `agent:ready` 등 라벨 필터
- T14: Issue 상세·댓글 표시

M3 — Pull Requests / Actions

T15–T22 · 8개

- T15: Open / Closed / Merged PR 구분
- T16: Draft PR 구분
- T17: 리뷰 상태 표시
- T18: PR 검사 성공·실패·대기 구분
- T19: PR 변경 파일 목록 조회
- T20: Actions 실행 목록 조회
- T21: Actions 실행 상태와 결과 분리
- T22: 실패한 실행 로그 조회

M4 — TUI / 오류 / 통합

T23–T30 · 8개

- T23: 키보드만으로 화면 탐색
- T24: 검색·필터·페이지 추가 조회
- T25: 새로고침 결과 정상 반영
- T26: 네트워크 실패 시 기존 데이터 유지
- T27: 인증 실패·Rate Limit 처리
- T28: 저장소 전환 중 오래된 비동기 응답 무시
- T29: CLI 호출 중에도 TUI 입력 반응 유지
- T30: macOS·Linux·tmux에서 기본 탐색 동작

여기서 T28은 특히 중요합니다. A 저장소의 데이터를 조회하는 도중 B 저장소로 전환했다면, 뒤늦게 완료된 A의 결과가 B의 화면을 덮어쓰면 안 됩니다. 요청 세대 번호나 선택 저장소 식별자를 확인하는 방식으로 방지할 수 있습니다.

### 실제 GitHub 통합 검증

별도의 테스트용 저장소를 사용하여 다음을 확인합니다.

- 비공개 저장소와 공개 저장소 조회
- 라벨이 있는 Issue 및 댓글 조회
- Draft PR과 일반 PR 조회
- 성공·실패한 CI 결과 확인
- 없는 저장소 및 접근 권한 부족 상태 처리
- 인증 오류와 네트워크 연결 실패 후 복구

v0.1은 읽기 전용이므로 통합 테스트 과정에서도 기존 Issue·PR을 수정하지 않습니다.

## 10. 개발 마일스톤

M1. GH Client + Repository Overview

`kepp github` 진입점, `gh` 호출, 모델 파싱, 저장소 목록 화면 구현.

완료 기준: T01\~T11 통과. 저장소 선택과 상세 조회 가능.

M2. Issue Browser

Issue 목록, 라벨·상태 필터, 상세 내용 및 댓글 조회.

완료 기준: T12\~T14 통과. 여러 저장소에서 Issue 탐색 가능.

M3. PR + Actions Monitor

Draft PR, 리뷰 상태, CI 결과, 워크플로 실행 정보 및 실패 로그 구현.

완료 기준: T15\~T22 통과. PR과 검사 결과를 함께 확인 가능.

M4. 오류 처리 + 릴리스

탐색·새로고침·비동기 처리 안정화 및 플랫폼별 검증.

완료 기준: T23\~T30 통과. macOS/Linux 및 tmux에서 정상 동작.

## 11. 최종 Definition of Done

아래의 사용자 시나리오를 모두 수행하면 GitHub Console v0.1 완료로 판단하겠습니다.

인수 테스트

1. `kepp github`로 실행하면 저장소 목록 화면이 나타납니다.
2. 여러 GitHub 저장소를 등록하고 전환할 수 있습니다.
3. 선택한 저장소의 Open Issues와 PR을 확인할 수 있습니다.
4. `agent:ready` 라벨의 Issue만 필터링할 수 있습니다.
5. Draft PR과 리뷰가 필요한 PR을 구분할 수 있습니다.
6. PR의 CI 검사 결과와 실패한 Actions 로그를 확인할 수 있습니다.
7. 키보드로 검색·탐색·상세 조회가 가능합니다.
8. `r`로 변경된 GitHub 데이터를 다시 읽어옵니다.
9. 인증·네트워크 오류가 발생해도 다른 모듈을 사용할 수 있습니다.
10. macOS, Linux, tmux에서 기본 기능이 동작합니다.

## 12. 버전별 발전 방향

| 버전   | 범위                      | 개발 목적              |
| ---- | ----------------------- | ------------------ |
| v0.1 | 저장소·Issue·PR·Actions 조회 | 개발 현황 가시화          |
| v0.2 | Issue CRUD, 라벨 관리       | 실제 작업 관리           |
| v0.3 | PR 리뷰·승인·병합, Actions 제어 | 개발 검토 콘솔           |
| v0.4 | Project Manager 연동      | 로컬 프로젝트와 GitHub 연결 |
| v0.5 | KEPP LAB Agent 연동       | AI 개발 작업 감독        |

GitHub Projects 보드 기능은 별도 우선순위로 두겠습니다. v0.1의 GitHub Console은 저장소 중심이기 때문에 Projects까지 포함하면 데이터 모델과 UI 범위가 커집니다.

## 최종 확정 제안

| 항목      | 명세                  |
| ------- | ------------------- |
| 프로젝트    | `kepp-tui`          |
| 모듈      | `github`            |
| 실행 명령   | `kepp github`       |
| 언어      | C++20               |
| 인터페이스   | FTXUI               |
| 데이터 연동  | GitHub CLI (`gh`)   |
| 인증      | 기존 `gh` 계정 재사용      |
| 시작 화면   | Repository Overview |
| v0.1 정책 | 읽기 전용               |
| 갱신 방식   | 수동 새로고침             |
| 테스트     | 30개 항목              |
| 저장소 설정  | 관심 저장소 등록 목록만 로컬 저장 |

가장 먼저 구현할 것은 M1의 Repository Overview입니다. GitHub API와 CLI의 차이를 직접 이해하면서, 나머지 Issue·PR·Actions 화면에서도 재사용할 통신 구조를 검증할 수 있기 때문입니다.

특히 KEPP LAB의 AI 개발팀이 만드는 여러 PR의 상태를 한 화면에서 확인하는 것이 이 프로젝트의 실질적인 활용처가 될 것입니다.

공식 참고 자료: [GitHub CLI Manual](https://cli.github.com/manual/gh?utm_source=chatgpt.com) · [Pull Request 명령](https://cli.github.com/manual/gh_pr?utm_source=chatgpt.com) · [Actions 명령](https://cli.github.com/manual/gh_run?utm_source=chatgpt.com) · [GitHub API 호출](https://cli.github.com/manual/gh_api?utm_source=chatgpt.com)
