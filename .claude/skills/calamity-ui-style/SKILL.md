---
name: calamity-ui-style
description: Calamity(칼라미티) 모드의 Prisma 패널·툴팁 UI를 만들거나 고칠 때의 스타일·문구·검증 기준. Data/PrismaUI/views/CalamityAffixes 아래 HTML/CSS/JS를 건드리거나, 새 버튼·확인창·탭·안내 문구를 추가하거나, 화면이 "AI 슬롭 같다/스카이림 같지 않다/잘린다/스크롤이 이상하다"는 이야기가 나오거나, design-critique·frontend-design·ux-copy 스킬로 패널을 다듬으려 할 때 반드시 먼저 읽을 것. 룬워드·재련·정제·교환 기능 작업 중 UI가 조금이라도 바뀌면 해당된다. Use for any Calamity Prisma panel/tooltip UI, copy, or layout work.
---

# 칼라미티 UI 스타일

칼라미티의 패널은 스카이림 화면 위에 뜨는 게임 UI다. 웹 대시보드가 아니다. 이 스킬은 세 가지를 지키게 한다.

1. 다른 모드와 섞여도 스카이림처럼 보이는 톤
2. Prisma(Ultralight)에서 실제로 동작하는 마크업
3. 사용자가 이미 정한 용어와 문구 원칙

마지막으로 바뀐 화면이 잘리지 않는지 측정하는 방법을 다룬다.

일반 디자인 스킬(frontend-design, design-critique 등)은 기본적으로 현대 웹 스타일로 흐른다. 큰 대비, 네온 강조색, 카드 그림자, 둥근 칩, 굵은 산세리프 같은 것들이다. 그 제안을 그대로 옮기지 말고 아래 기준으로 걸러서 쓴다.

## 1. 톤: 스카이림 문법

사용자는 연출에서 2D·애니·메이플풍을 두 번 거절했다. 외곽선, 별 섬광, 집중선, 선명한 아이콘형 실루엣 같은 것들이다. 받아들인 쪽은 바닐라 문법이었다. 반투명하고, 채도가 낮고, 부드럽게 번지는 것들이다. UI도 같은 기준으로 판단한다. 또 2026-10-04 평가에서 사용자는 현재 패널의 "형광초록 다크 대시보드" 느낌을 슬롭의 원인 중 하나로 꼽았다.

**지향할 것**
- 반투명 검정 바탕 위에 뼈색·은색 계열 글자를 쓴다. 강조는 밝기 차이로 주고, 색은 최소로 쓴다.
- 구분은 얇은 선(1px, 낮은 불투명도)과 여백으로 한다. 상자를 겹겹이 두르지 않는다.
- 색은 의미가 있을 때만 쓴다. 위험·불일치는 바랜 붉은색/호박색이고, 마법·룬 관련은 바닐라 마법 게이지 같은 차가운 청색 계열이다.
- 상태 변화는 짧은 투명도·밝기 전환으로 보여 준다(150~250ms).

**피할 것**
- 형광 강조색(현재 `--accent: #00ff99`가 대표 사례), 네온 글로우, 무한 반복 애니메이션
- 큰 그림자, 유리 효과 남발, 그라데이션 버튼, 이모지와 장식 아이콘
- 알약 모양 칩을 화면 가득 늘어놓는 것. 정보 칩은 꼭 필요한 곳에만 둔다.

**전면 교체는 따로 승인받는다.** 팔레트·글꼴·전체 레이아웃을 바꾸는 건 별도 결정이다. 이렇게 진행한다.
1. 시안을 실제 게임 스크린샷 위에 합성해서 보여 준다. 단색 배경 시안은 판단을 흐린다.
2. 사용자가 고른 뒤에 적용한다.

**기능 작업에서는 기존 토큰을 쓴다.** 새 컴포넌트는 `styles/base.css`의 `:root` 토큰(`--text`, `--muted`, `--border`, `--accent`, `--danger`…)을 쓴다. 필요한 값이 없으면 토큰을 새로 만든다. 색을 하드코딩하지 않으면 나중에 팔레트를 바꿀 때 새 화면도 한 번에 따라온다.

## 2. Prisma(Ultralight) 제약

Prisma UI는 CEF가 아니라 **Ultralight(WebKit 계열)** 이다. 코드 주석에 "CEF"라고 적힌 곳은 옛 표현이다. 아래는 이 레포에서 실제로 겪은 것들이다.

| 제약 | 대응 |
|---|---|
| 패널 배율 `--panel-ui-scale`이 패널 크기에 비례(0.9~1.75) | 모든 길이를 `calc(Npx * var(--panel-ui-scale))`로 쓴다. 패널을 키워도 공간 부족은 안 풀린다. 내용을 줄이거나 스크롤로 해결한다. 툴팁은 `--tooltip-font-scale`을 따로 쓴다. |
| 휠 델타가 작고 `scrollTop`이 정수 | `scroll-behavior: smooth`나 스크롤 라이브러리를 들이지 않는다. 긴 목록에서 휠이 느리면 `data-wheel-scroll-mode="smooth"`를 붙여 `scripts/scroll.js`의 프레임 단위 컨트롤러에 맡긴다(현재 툴팁과 레시피 목록). |
| 포커스 이동 중 `focusout`으로 닫으면 실제 클릭이 취소됨 | 선택창은 바깥 클릭·`Esc`·탭 전환·선택 완료로만 닫는다. |
| 무한 `box-shadow` 애니메이션, `offsetWidth` 강제 reflow는 끊김을 유발 | 짧은 전환과 `requestAnimationFrame` 단위 갱신만 쓴다. |
| `display: grid/flex`를 준 클래스가 `hidden` 속성을 덮음 | `.foo[hidden] { display: none; }` 짝 규칙을 함께 쓴다. |
| 한국어 글꼴은 line-height normal이 영어보다 큼(14→17px) | 고정 높이 칸에서는 `line-height`를 명시한다. |
| 영어판 스카이림 HUD 글꼴에 한글이 없음 | 패널 밖 HUD 알림(`RE::DebugNotification`)은 영어로 쓴다. |
| `backdrop-filter`, 최신 CSS(`:has()`, 컨테이너 쿼리, `color-mix()`, `@layer`, subgrid)는 Ultralight 지원이 확인되지 않음 | 새로 도입하지 않는다. 꼭 써야 하면 실게임 확인을 전제로 하고, 없어도 깨지지 않게 쓴다. 툴팁의 `blur(8px)`는 옛 코드다. |

마크업 구조도 지킨다.
- 뷰는 `index.html` + `styles/*.css` + `scripts/*.js`로 나뉘고 `<script src>` 순서가 실행 순서다. `type="module"`은 쓰지 않는다.
- 새 파일을 추가하면 `index.html`에 연결하고 `tools/verify_prisma_view.py`를 통과시킨다.
- C++이 부르는 interop 함수(`setRunewordPanelState` 등)와 `CalamityAffixes_UI_SetPanel` 모드 이벤트, FormID는 외부 API다. Clemmerson의 의존 모드가 쓰기 때문에 이름과 형식을 바꾸지 않는다. 필드는 추가만 한다.

## 3. 문구와 용어

**다국어**
- 모든 문구는 `t(en, ko)`로 쓴다. MCM 기본값이 **이중 표기**(`English / 한국어`)라서 가장 긴 경우는 두 언어를 합친 길이다.
- 좁은 칸(룬 그리드 칸, 칩, 수치 라벨)은 `tCompact(en, ko)`로 한 언어만 보이고, 전체 `t()` 문구는 `title`(hover)에 둔다.
- 룬워드 이름 같은 데이터 이름은 `nameEn`/`nameKo` 쌍에서 언어별로 고른다(`resolveRecipeName`). 한국어만 넘기던 버그가 v2.2.1~2.2.2에 두 번 났다.
- C++이 보내는 고정 영어 문장은 `engineFeedbackLocalizations`(`scripts/i18n.js`)에 번역을 등록한다.

**정해진 용어**

| 대상 | 한국어 | 영어 |
|---|---|---|
| 모드 이름 | 칼라미티 (칼래미티 아님) | Calamity |
| 작업 대상인 내 장비 | 선택한 장비 | selected item |
| 레시피가 권하는 장비 종류 | 베이스 | base |
| pity | 천장 | Pity |
| 확률 칩 | `발동 24.5%`, `평타 35%`, `행운 적중 30%` 처럼 짧게 | 같은 형식 |
| 레시피 탐색기 위치 | 왼쪽 | left |
| 데이터 대기 중 | 불러오는 중 ("동기화" 쓰지 않음) | Loading |

**문구 원칙**(2026-10-04 슬롭 평가에서 46항목을 고칠 때 쓴 기준)
- 내부 판정 용어를 화면에 보이지 않는다(ICD, editor ID, 주문 프로필명 등). 플레이어 말로 쓴다.
- 매번 떠 있는 당연한 안내문, 읽기 전용 설명, 변명형 문장("~할 수 있지만 ~입니다")은 쓰지 않는다. 행동을 못 하는 이유가 있을 때만 그 자리에 짧게 쓴다.
- 같은 상태 문장은 한 곳에만 둔다. 머리줄·칩·검토 칸에 같은 경고를 세 번 쓰지 않는다.
- 되돌릴 수 없는 행동의 확인창은 무엇을 잃는지 구체적으로 쓴다. 예: "접두 '폭풍 소환'이 사라집니다". 확인 버튼에는 동사를 쓴다. 예: "룬워드 입히기".

## 4. 작업 순서

1. **현재 화면을 측정하고 캡처한다.** 아래 스크립트를 `--shots`로 돌리면 크기×언어×탭별 패널 캡처가 나온다. 이걸로 무엇을 바꿀지 판단한다.
2. **바꾼다.** 위 기준을 따른다.
3. **릴리스 대비 회귀를 확인한다.**
   ```bash
   node .claude/skills/calamity-ui-style/scripts/panel_audit.js --compare v2.2.8 --shots --out <scratchpad>/audit
   ```
   - `--compare`에는 최신 릴리스 태그를 넣는다. 지금 화면도 결과 0개는 아니다. 레시피 목록과 검토 칸은 작업대 안에서 일부러 스크롤된다. 그래서 절대 개수보다 **새로 생긴 것**을 본다.
   - 새로 생긴 CLIPPED, NESTED-SCROLL, UNREACHABLE, JS 오류는 고친다.
   - TRUNCATED는 의도된 말줄임이다(`text-overflow: ellipsis`, `line-clamp`). 세기만 하고 출력에는 나열하지 않는다. 대신 전체 문구를 볼 길(`title`, 상세 칸)이 있는지 확인한다.
   - 스크롤 열 안의 작은 상자가 넘치는 것은 결함으로 본다. 2.0.1에서 이걸 놓쳤다.
   - 스크립트는 Playwright를 `~/.npm/_npx/*`에서, Chromium을 `~/.cache/ms-playwright`에서 찾는다. `--hide-scrollbars`를 끄고, `no-store`로 서빙한다. 옵션은 파일 머리 주석에 있다.
4. **캡처를 눈으로 본다.** 숫자가 깨끗해도 톤이 틀릴 수 있다. 시각 변경이면 사용자에게 전후 캡처를 보여 준다.
5. **테스트를 돌린다.** 문구를 바꾸면 세 군데가 깨진다. 셋 다 확인한다.
   ```bash
   python3 -m unittest discover -s tools/tests -p 'test_*.py'
   ```
   ```bash
   ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure
   ```
   - 첫 명령은 `test_prisma_*`이고, 그 안에서 node 동작 테스트도 돌아간다.
   - ctest의 `CalamityAffixesRuntimeGateStoreChecks`는 UI 소스 문자열(`prisma_panel_ux_flow`, `runeword_recipe_tooltip_text`)을 검사한다. Python만 돌리면 놓친다.
6. **실게임에서 확인한다.** Chromium 측정은 근사치다. 테스트 ZIP을 만들어 사용자가 MO2 프로필 `TKL - MUNG ADDON - SPS Test`에서 확인한다. 화면이 이상하다는 말이 나오면 녹화 프레임을 잘라 픽셀로 잰다.

## 하지 말 것

- 카메라 흔들림, 화면 플래시 등 화면 전체 효과
- 사용자 확인 없는 팔레트·글꼴 전면 교체
- 테스트를 통과시키려고 소스 문자열 검사(테스트 쪽 기대 문자열)만 고치기. 화면 문구와 테스트 기대값은 같은 의도로 함께 바꾼다.
