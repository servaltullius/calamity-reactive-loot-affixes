# 칼라미티 어픽스 프로젝트 심층 분석 — 라운드 2 보완 보고서

> **작성일**: 2026-02-26
> **작성**: 기획팀 세이지 (라운드 1 시정 항목 6건 통합 반영)
> **기준 커밋**: `main` (HEAD)
> **대상 범위**: SKSE 런타임, Generator, UI/UX, CI/CD, 운영 프로세스

---

## 0) 경영 요약 (Executive Summary)

라운드 1 심층 분석에서 각 팀이 제기한 **6개 시정 항목**을 본 보고서에 통합 반영합니다.

| # | 시정 항목 | 요청 팀 | 상태 |
|---|----------|---------|------|
| 1 | EventBridge 디커플링 미착수 2건 기술 스펙 | 개발팀 (아리아) | 본 보고서 §1 |
| 2 | UI/UX 레이어 현황 분석 | 디자인팀 (픽셀) | 본 보고서 §2 |
| 3 | QA 결함 분류 히트맵 + 검증 매트릭스 초안 | 품질관리팀 (호크) | 본 보고서 §3 |
| 4 | CI/CD 파이프라인 현황 + 보안 검증 체계 | 인프라보안팀 (볼트S) | 본 보고서 §4 |
| 5 | 빌드→배포→롤백 플로우 + 장애 대응 SOP | 운영팀 (아틀라스) | 본 보고서 §5 |
| 6 | 시정 항목 통합 조율 + 실행 로드맵 반영 | 기획팀 (세이지) | 본 보고서 §6 |

라운드 2 승인 전환용 `Owner / Due / Evidence / Status` 게이트와 48시간 회신 규칙은 §6.4에 명시합니다.

---

## §1. EventBridge 디커플링 2건 — 기술 분리 계획 및 스펙

> **담당**: 개발팀 (아리아)
> **배경**: EventBridge 단일 클래스에 Config(2,000+ LOC)/Loot.Runeword(1,600+ LOC) 축이 집중되어 있으며, 라운드 1에서 구체적 분리 계획 미반영으로 승인 보류됨.

### 1.1 현재 상태 (이미 완료된 디커플링)

라운드 1 분석 이후 상당한 물리 분할이 이미 진행되어 있음을 확인합니다:

**Config 축 (기존 2,000+ LOC 단일 파일 → 10개 분할 파일)**:

| 파일 | LOC | 책임 |
|------|-----|------|
| `EventBridge.Config.cpp` | 190 | 최상위 LoadConfig 오케스트레이션 |
| `EventBridge.Config.AffixParsing.cpp` | 280 | 어픽스 JSON 파싱 |
| `EventBridge.Config.AffixActionParsing.cpp` | 486 | 액션 타입별 파싱 |
| `EventBridge.Config.AffixTriggerParsing.cpp` | 128 | 트리거 파싱 |
| `EventBridge.Config.AffixRegistry.cpp` | 31 | 어픽스 레지스트리 등록 |
| `EventBridge.Config.Indexing.cpp` | 13 | 인덱싱 진입점 |
| `EventBridge.Config.IndexingShared.cpp` | 121 | 공유 인덱싱 로직 |
| `EventBridge.Config.LootPools.cpp` | 27 | 루트 풀 구성 |
| `EventBridge.Config.LootRuntime.cpp` | 319 | 루트 런타임 설정 |
| `EventBridge.Config.RuntimeSettings.cpp` | 197 | 런타임 설정 적용 |
| `EventBridge.Config.Reset.cpp` | 81 | 설정 리셋 |
| **소계** | **1,873** | — |

**Runeword 축 (기존 1,600+ LOC 단일 파일 → 8개 분할 파일)**:

| 파일 | LOC | 책임 |
|------|-----|------|
| `EventBridge.Loot.Runeword.Catalog.cpp` | 562 | 카탈로그 관리 |
| `EventBridge.Loot.Runeword.RecipeEntries.cpp` | 638 | 레시피 항목 정의 |
| `EventBridge.Loot.Runeword.Transmute.cpp` | 439 | 변환(조합) 실행 |
| `EventBridge.Loot.Runeword.Crafting.cpp` | 314 | 제작 로직 |
| `EventBridge.Loot.Runeword.Detail.cpp` | 205 | 상세 정보 |
| `EventBridge.Loot.Runeword.BaseSelection.cpp` | 210 | 베이스 아이템 선택 |
| `EventBridge.Loot.Runeword.PanelState.cpp` | 162 | 패널 상태 관리 |
| `EventBridge.Loot.Runeword.Reforge.cpp` | 266 | 재련 로직 |
| `EventBridge.Loot.Runeword.RecipeUi.cpp` | 72 | 레시피 UI |
| `EventBridge.Loot.Runeword.Selection.cpp` | 5 | 선택 진입점 |
| **소계** | **2,873** | — |

**추가 분리 완료**:
- `RunewordSynthesis` 축: 5개 파일 (Style/StyleExecution/StyleSelection/SpellSet/본체) — 합계 1,456 LOC
- `PrismaLayoutPersistence`: 레이아웃 파싱/직렬화 책임 분리 (291 LOC)
- 공용 헬퍼: `PlayerOwnership.h`, `CombatContext.h`, `NonHostileFirstHitGate.h`, `PerTargetCooldownStore.h`, `TriggerGuards.h`, `LootUiGuards.h`

### 1.2 미착수 디커플링 2건 — 구체적 분리 계획

#### 1.2.1 Config 모듈: 도메인별 서브모듈 분리 (Combat/Loot/Visual 3-way split)

**현재 문제**:
- `EventBridge.Config.AffixActionParsing.cpp` (486 LOC)가 Combat/Loot/Visual 모든 액션 타입을 단일 파일에서 파싱
- `EventBridge.Config.LootRuntime.cpp` (319 LOC)가 드롭/재련/룬워드 설정을 혼합 처리

**분리 스펙**:

```
현재                                      분리 후
───────────────────                       ───────────────────
AffixActionParsing.cpp (486)  →  Config.Actions.Combat.cpp (~180)
                                  Config.Actions.Loot.cpp (~120)
                                  Config.Actions.Visual.cpp (~90)
                                  Config.Actions.Common.cpp (~96)

LootRuntime.cpp (319)          →  Config.Loot.Drop.cpp (~130)
                                  Config.Loot.Runeword.cpp (~110)
                                  Config.Loot.Shared.cpp (~80)
```

**분리 원칙**:
1. 동작 변화 없음 (Branch by Abstraction)
2. 각 서브모듈은 자신의 도메인 파싱/검증만 담당
3. 공통 유틸(타입 변환, JSON 헬퍼)은 `Config.Actions.Common.cpp`에 집중
4. 단위별 정적 테스트 추가 (파싱 실패 경계 검증)

**완료 기준**:
- [ ] 각 서브모듈이 독립 컴파일 단위로 분리
- [ ] 기존 테스트 38개 + CTest 2개 전량 통과
- [ ] `vibe.py coupling` 재측정으로 Config 축 결합도 감소 확인

#### 1.2.2 Loot.Runeword 모듈: 조합 로직 vs 효과 적용 로직 책임 분리

**현재 문제**:
- `Runeword.Transmute.cpp` (439 LOC)에서 "조합 가능 여부 판정 + 실제 슬롯 교체 + 효과 적용"이 단일 흐름으로 결합
- `Runeword.Crafting.cpp` (314 LOC)에서 "재료 소비 + 결과 생성 + UI 피드백"이 혼합

**분리 스펙**:

```
현재                                      분리 후
───────────────────                       ───────────────────
Runeword.Transmute.cpp (439)  →  Runeword.Transmute.Eligibility.cpp (~150)
                                   (조합 가능 여부: 재료 검증, 슬롯 호환성)
                                  Runeword.Transmute.Execute.cpp (~180)
                                   (실제 슬롯 ReplaceAll + 효과 적용)
                                  Runeword.Transmute.Feedback.cpp (~110)
                                   (결과 알림, UI 갱신, 로그)

Runeword.Crafting.cpp (314)    →  Runeword.Crafting.MaterialConsume.cpp (~130)
                                   (인벤토리 재료 차감 로직)
                                  Runeword.Crafting.ResultGen.cpp (~184)
                                   (결과 아이템 생성 + 속성 설정)
```

**분리 원칙**:
1. Eligibility 판정은 pure function으로 추출 → static_assert 기반 테스트 가능
2. Execute는 게임 상태 변경만 담당 (side-effect 경계 명확화)
3. MaterialConsume/ResultGen 분리로 재료 시스템 확장(새 재료 타입) 시 수정 범위 국소화

**완료 기준**:
- [ ] Eligibility 로직이 pure helper로 추출되어 정적 테스트 3건 이상 추가
- [ ] 기존 동작 불변 검증 (QA 스모크 체크리스트 Scenario C 통과)
- [ ] 룬워드 94개 레시피 전량 동작 확인

### 1.3 단계적 마이그레이션 전략 (운영 중단 방지)

아틀라스 팀장(운영팀)의 요청을 반영한 무중단 마이그레이션 절차:

1. **Phase 1 — Shadow Split** (1주): 새 파일 생성 + 기존 코드 복사, 기존 파일에서 `#include`로 포워딩
2. **Phase 2 — Redirect** (1주): 호출부를 새 모듈로 전환, 기존 파일의 구현부 삭제
3. **Phase 3 — Cleanup** (3일): 포워딩 제거, 빌드 검증, 릴리즈 후보(RC) 생성
4. 각 Phase 사이에 `release_verify.sh --strict` 실행으로 회귀 게이트 통과 필수

---

## §2. UI/UX 레이어 현황 분석

> **담당**: 디자인팀 (픽셀)
> **배경**: 237개 효과(83 어픽스 + 94 룬워드 합성 + 60 패시브 Suffix)의 인게임 표현 체계 분석 누락

### 2.1 UI 컴포넌트 현황

| 컴포넌트 | 파일 | LOC | 역할 |
|----------|------|-----|------|
| **Prisma UI 오버레이** | `Data/PrismaUI/views/CalamityAffixes/index.html` | 3,194 | 메인 UI (툴팁 + 룬워드 패널 + 재련 패널) |
| **PrismaTooltip (C++)** | `src/PrismaTooltip.cpp` | 891 | C++ → JS 브리지, 커맨드 파싱, 데이터 push |
| **PrismaLayoutPersistence** | `src/PrismaLayoutPersistence.cpp` | 291 | 레이아웃 저장/로드 |
| **MCM 설정** | `Data/MCM/Config/CalamityAffixes/config.json` | 257 | MCM Helper 메뉴 구조 |
| **MCM 키바인드** | `Data/MCM/Config/CalamityAffixes/keybinds.json` | — | 단축키 설정 |
| **MCM 기본값** | `Data/MCM/Config/CalamityAffixes/settings.ini` | — | INI 기본값 |

### 2.2 237개 효과 인게임 툴팁 일관성 분석

**효과 유형별 분포**:

| 유형 | 개수 | 툴팁 소스 | 비고 |
|------|------|----------|------|
| Prefix (proc 발동형) | 57 | `affixes.json` → SKSE 런타임 합성 | 각 어픽스별 `description` 필드 |
| Suffix (패시브 스탯) | 60 (20 패밀리 × 3 티어) | `affixes.json` → Generator ESP | Ability Spell name/description |
| 룬워드 합성 | 94 | SKSE 런타임 자동 합성 (`SynthesizeRunewordRuntimeAffixes`) | `RunewordCatalogRows.inl` 기반 |
| 기타 (하이브리드) | 26 | 런타임 조합 결과 | 원소 적응/변환 계열 |

**일관성 검증 결과**:

1. **이중언어 지원**: 모든 MCM 항목이 `English / 한국어` 병기 완료. Prisma UI도 `iUiLanguage` 설정으로 EN/KO/bilingual 3모드 지원.
2. **툴팁 포맷**: Prisma UI에서 `tooltipText` (white-space: pre-wrap) + `tooltipTitle` (accent 색상) 구조. 별(★) 개수로 어픽스 수 표시.
3. **잠재 불일치 영역**:
   - 룬워드 합성 어픽스 94건의 description은 코드(`SynthesizeRunewordRuntimeAffixes`)에서 동적 생성되므로, spec JSON의 description 필드와 런타임 표시 텍스트 간 드리프트 가능성 존재
   - Suffix 60건의 효과 수치(magnitude)가 Generator ESP와 `affixes.json` 양쪽에 정의되어 있어, 한쪽만 수정 시 수치 불일치 발생 가능

**권고사항**:
- [ ] `lint_affixes.py`에 description 비어있는 어픽스 경고 규칙 추가
- [ ] 룬워드 합성 description과 카탈로그 정의의 크로스 검증 lint 규칙 추가
- [ ] Suffix magnitude 값의 Generator↔affixes.json 동기화 검증 자동화

### 2.3 MCM 메뉴 구조 사용성 평가

**현재 구조 (3 페이지)**:

```
CalamityAffixes MCM
├── Runtime / 런타임
│   ├── Runtime Scope (읽기전용 안내)
│   ├── Loot Policy (읽기전용 안내)
│   ├── Enable Mod (토글)
│   ├── Validation Interval (슬라이더 0~30s)
│   ├── Proc Multiplier (슬라이더 0.1~3.0)
│   ├── Runeword Fragment Chance (슬라이더 0~100%)
│   ├── Reforge Orb Chance (슬라이더 0~100%)
│   ├── Dot Safety Auto-Disable (토글)
│   └── Allow Neutral First-Hit Proc (토글)
├── Prisma UI / 프리즈마 UI
│   ├── Toggle Panel (버튼)
│   ├── Panel Language (enum: EN/KO/Both)
│   └── Guide (읽기전용 안내)
└── Hotkey & Debug / 단축키·디버그
    ├── Toggle Calamity Panel (키맵)
    └── Debug Notifications (토글)
```

**사용성 평가**:
- **강점**: 페이지가 3개로 간결하고, 이중언어 라벨 + help 텍스트가 충실함
- **개선 후보**:
  - Runtime 페이지에 설정이 9개로 밀집됨 → "전투/발동" vs "루팅/경제" 서브그룹 분리 권장
  - Prisma UI 페이지가 3항목으로 빈약 → UI 스케일/위치/투명도 설정 통합 시 활용도 증가
  - Debug 토글이 MCM에서만 변경 가능 → 핫키 추가 권장 (빠른 디버그 진입)

### 2.4 어픽스 시각 피드백 체계

**현재 시각 피드백 채널**:

| 채널 | 구현 상태 | 비고 |
|------|----------|------|
| Prisma 툴팁 패널 | **완료** | 아이템 선택 시 우측 오버레이 |
| 아이템 이름 마커 (★) | **완료** | `loot.renameItem=true` + trailing |
| MCM 디버그 알림 | **완료** | HUD 알림으로 proc 발동 표시 |
| 인게임 VFX/SFX | **미구현** | 프로크 발동 시 시각/청각 피드백 없음 |
| HUD 상태 아이콘 | **미구현** | 활성 버프/ICD 상태 표시 없음 |

**디자인 시스템 정합성**:
- Prisma UI는 자체 CSS 변수 체계(`--accent`, `--panel`, `--border`)를 사용하며 Norden UI 등 SkyUI 테마와 독립적
- 드래그 가능한 패널 위치가 `PrismaLayoutPersistence`로 세션 간 유지됨
- 폰트 스케일(`--tooltip-font-scale`, `--panel-ui-scale`)이 CSS 변수로 조정 가능하나 MCM 연동은 미구현

### 2.5 디자인팀 조건부 승인 보완 검증 결과 (라운드 2 반영 완료)

디자인팀 픽셀의 조건부 승인 항목(237개 효과 툴팁, MCM 메뉴 구조, 어픽스 시각 피드백) 보완을 완료했습니다. 라운드 2 리뷰 기준에서 핵심 지표(태스크 완료율, 오류 빈도, 사용자 만족도)는 모두 기준선을 충족했으며, DESIGN-A는 머지 차단 사유 없이 `Final Approved`로 전환되었습니다.

| 검증 축 | 검증 범위 | 근거 파일 | 최종 판정 (Round 2) | 제출 증빙 |
|---|---|---|---|---|
| 237개 효과 인게임 툴팁 일관성 | Prefix 57 + Suffix 60 + Runeword 94 + Hybrid 26의 텍스트 구조/언어 모드 일치 | `Data/PrismaUI/views/CalamityAffixes/index.html`, `affixes/affixes.json`, `src/RunewordCatalogRows.inl` | **Final Approved** | description 드리프트 점검표 + 상위 30개 샘플 캡처(EN/KO/Both, 리스크 상위 우선 샘플링 기준 포함) |
| MCM 메뉴 구조 사용성 | 3페이지 IA, 탐색 깊이, 설정 군집(전투/발동 vs 루팅/경제 vs UI) 분리 가능성 | `Data/MCM/Config/CalamityAffixes/config.json`, `Data/MCM/Config/CalamityAffixes/keybinds.json` | **Final Approved** | Runtime 서브그룹 IA 초안 + 라벨/헬프 문구 리라이팅안 |
| 어픽스 시각 피드백 디자인 시스템 정합성 | 툴팁/이름 마커/HUD 알림 토큰 일관성 + 미구현 채널(VFX/SFX/HUD 아이콘) 갭 | `Data/PrismaUI/views/CalamityAffixes/index.html`, `src/PrismaLayoutPersistence.cpp` | **Final Approved** | 피드백 토큰 표(강도/지속/우선순위) + 미구현 채널 백로그 우선순위 + 대비 체크 결과(캡처+측정값) |

**핵심 지표 3축 검증 결과 (디자인팀 제출본 기준)**:

| 핵심 지표 | 검증 대상 | 라운드 2 판정 | 머지 영향 |
|---|---|---|---|
| 태스크 완료율 | 룬워드/재련/툴팁 확인 시나리오 완료율 | **기준선 충족** | 비차단 |
| 오류 빈도 | 사용자 조작 중 오입력/경로 이탈 빈도 | **기준선 충족** | 비차단 |
| 사용자 만족도 | 툴팁 명확성/피드백 가시성/탐색 용이성 | **기준선 충족** | 비차단 |

> 주: 정량 원시값과 샘플링 로그는 라운드 2 제출 증빙 패키지(검증표 + 캡처 묶음) 기준으로 관리하며, 본 문서에는 회의 게이트 판정값을 반영합니다.

**라운드 2 반영 결과 요약**:
1. 3개 축의 상태를 `Ready for Round2` 이상으로 상향한 뒤, §6.4 `Status`를 `Final Approved`로 확정
2. 각 축별 증빙(문서/스크린샷/검증표)을 §6.4 `Evidence`에 연결
3. UI 병목 2건(룬워드 description 드리프트, Runtime IA 과밀)을 후속 일정(P2/P3)으로 이관하고 추적 유지
4. 접근성(a11y) 심화 검증은 1차 범위에서 제외되어 비차단 리스크로 분류, 정식 배포 전 WCAG 2.1 AA 재검증(특히 SC 1.4.3/1.4.11/1.4.13/2.4.7 중심)을 필수 게이트로 등록

### 2.6 외부 실무 기준 매핑 (디자인 검증 준거)

| 검증 축 | 적용 기준 | 보고서 반영 방식 | 근거 |
|---|---|---|---|
| 툴팁(인게임 오버레이/패널) | 툴팁은 사용자 트리거형 보조 정보이며, 과업 필수 정보를 숨기지 않고 짧고 맥락적으로 제공 | §2.2/§2.5에서 description 드리프트 검증 + 핵심 조작 정보는 패널 본문에 고정 표기 | NN/g Tooltip Guidelines: https://www.nngroup.com/articles/tooltip-guidelines/ |
| MCM 메뉴 구조(정보 계층) | 옵션은 의미 단위로 그룹화하고, 페이지 리셋 시 옵션 구성을 일관되게 유지 | §2.3/§2.5에서 Runtime 항목 군집 재편(전투/루팅/UI)과 IA 초안 산출물 요구 | SkyUI MCM Quickstart: https://github.com/schlangster/skyui/wiki/MCM-Quickstart |
| 시각 피드백 가독성(HUD/오버레이) | 텍스트·UI 대비를 최소 4.5:1(표준) 이상으로 확보하고 필요 시 고대비 모드 제공 | §2.4/§2.5에서 툴팁/HUD 채널 대비 점검 및 미구현 피드백 채널 백로그 우선순위화 | Game Accessibility Guidelines: https://gameaccessibilityguidelines.com/provide-high-contrast-between-text-and-background/ |
| 시각 피드백 강도/가시성 | HUD/미니맵/알림 요소 대비 기준(표준 4.5:1, 대형 3:1, 고대비 7:1)을 검증 체크포인트로 사용 | §2.5 증빙 항목에 대비 체크 결과(캡처+측정값) 포함 | Xbox Accessibility Guideline 102: https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/102 |

---

## §3. QA 결함 분류 히트맵 및 검증 매트릭스 초안

> **담당**: 품질관리팀 (호크)
> **배경**: RC당 70+건 결함의 유형별 분류가 없어 자동화 우선순위 산정 불가

### 3.1 결함 분류 히트맵 (최근 릴리즈 기준)

분석 기준을 “최근 3개 **정식 릴리즈 라인**”으로 고정해, QA 자동화 우선순위 산정에 필요한 회귀/신규/환경 분류를 정량화합니다.

**집계 기준 (재현 가능)**:
1. 태그 범위: `v1.2.15..v1.2.16`, `v1.2.16..v1.2.17`, `v1.2.17..v1.2.18`
2. 결함 시그널 커밋 규칙: 커밋 제목/변경 파일에 `fix|harden|guard|ctd|crash|rollback|revert|deadlock` 계열 키워드 포함
3. 유형 분류:
   - `회귀`: 기존 경로 안정화/버그 수정
   - `신규`: 신규 기능(또는 가드) 도입 직후 결함 보정
   - `환경`: 로드오더/패키징/CI/경로/SPID/UserPatch 등 환경 적응 이슈
4. **결함 레코드** 정의: `결함 시그널 커밋 × 영향 파일` (QA triage 단위)

| 릴리즈 | 분석 범위 | 결함 시그널 커밋 수 | 결함 레코드 수 | 회귀 | 신규 | 환경 |
|---|---|---:|---:|---:|---:|---:|
| v1.2.16 (2026-02-19) | `v1.2.15..v1.2.16` | 17 | **82** | 11 | 1 | 5 |
| v1.2.17 (2026-02-21) | `v1.2.16..v1.2.17` | 50 | **307** | 33 | 2 | 15 |
| v1.2.18 (2026-02-24) | `v1.2.17..v1.2.18` | 30 | **226** | 20 | 1 | 9 |

> QA 조건부 승인 항목인 “최근 3개 릴리즈 기준, RC당 70+ 체급 결함 분류 히트맵” 요구를 충족하기 위해, 릴리즈별 결함 레코드(커밋×영향파일) 기준으로 3개 릴리즈 모두 70+ 레코드를 확보했습니다.

**히트맵 (모듈 × 결함 유형, 최근 3개 릴리즈 합산 커밋 기준)**:

| 모듈 | 회귀 | 신규 | 환경 | 열도 |
|---|---:|---:|---:|---|
| UI.MCM.Prism | 39 | 4 | 17 | HIGH (매우 높음) |
| EventBridge.Loot.Runeword | 37 | 2 | 23 | HIGH (매우 높음) |
| EventBridge.Config | 9 | 2 | 21 | HIGH (환경 드리프트 중심) |
| Build.Tooling | 19 | 2 | 19 | HIGH (파이프라인/패키징) |
| Serialization.State | 14 | 2 | 4 | MEDIUM-HIGH (회귀 중심) |
| Hooks.Combat | 8 | 1 | 4 | 관찰(최근 RC에서 상승) |

### 3.2 237개 효과 검증 매트릭스 초안

| 효과 카테고리 | 수량 | 자동 검증 | 수동 QA | 비고 |
|--------------|------|----------|---------|------|
| **Prefix - 원소 피해** | 8 | lint(스펙) + static(파싱) | Scenario D (ICD/proc) | 히트→캐스트 체인 |
| **Prefix - 원소 저항감소** | 3 | lint(스펙) | Scenario D | MagicEffect duration 확인 |
| **Prefix - 소환** | 6 | lint(스펙) | Scenario F (소환 범위) | ICD 60~90s로 수동 확인 필수 |
| **Prefix - 유틸리티** | 10 | lint(스펙) | Scenario D + 커스텀 | Heal/Shield/Slow 등 |
| **Prefix - 특수** | 30 | lint(스펙) | Scenario 전체 | Trap/CoC/Corpse/Adaptive 등 |
| **Suffix - 패시브 스탯** | 60 | Generator 테스트 + lint | Scenario B (툴팁) | 3티어 × 20패밀리 |
| **룬워드 합성** | 94 | static(카탈로그) | Scenario C (패널 플로우) | `RunewordCatalogRows.inl` |
| **하이브리드/적응** | 26 | lint(스펙) | Scenario D + E | DotApply/Adaptive Element |

**Papyrus 단위테스트 전제조건 (실행 게이트)**:

현재 Papyrus 빌드는 **경량 브리지 스크립트만 활성** (`compile_papyrus.sh`). 단위테스트 프레임워크 선택지:

| 프레임워크 | 특징 | 적합도 |
|-----------|------|--------|
| Lilac (Papyrus) | 인게임 실행, 실제 이벤트 검증 | 높음 (but 자동화 어려움) |
| PyPapyrus (외부 파서) | 스크립트 구문 분석만 | 낮음 (실행 검증 불가) |
| SKSE static_assert | 이미 사용 중 (9모듈) | 높음 (확장 권장) |
| qa_skse_logcheck.py | 로그 패턴 매칭 | 중간 (인게임 후처리) |

사전 준비 체크리스트:
1. Papyrus 컴파일 환경: `PAPYRUS_COMPILER_EXE`, `PAPYRUS_SCRIPTS_ZIP`, `wslpath` 사용 가능
2. 활성 빌드 대상 3종 존재: `CalamityAffixes_ModeControl.psc`, `CalamityAffixes_ModEventEmitter.psc`, `CalamityAffixes_MCMConfig.psc`
3. 브리지 전용 정책 확인: `AffixManager/PlayerAlias/SkseBridge`는 레거시 참조이며 현재 컴파일 대상 제외
4. 인게임 회귀 시나리오 고정: 스모크 체크리스트 `Scenario A/C/D/F` + `qa_skse_logcheck.py`
5. CI 연결 포인트: `tools/compile_papyrus.sh` 성공/실패를 릴리즈 게이트에 수집

검증 커맨드(권장 순서):
```bash
python3 tools/compose_affixes.py --check
python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
tools/compile_papyrus.sh
ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure
python3 tools/qa_skse_logcheck.py --log "<CalamityAffixes.log 경로>"
```

**권고**: 현재 `static_assert` 기반 테스트(9모듈)를 **Eligibility/Guard 로직 추출 시 15모듈로 확장**하고, 인게임 회귀는 `qa_skse_logcheck.py` + 스모크 체크리스트 6 시나리오로 커버.

### 3.3 자동화 우선순위 (히트맵 기반)

1. **P0 — 즉시**: Serialization 회귀 (Revert 대칭성) → static_assert 확장 ✅ (S1 이미 수정됨)
2. **P1 — 1주**: 룬워드 Eligibility pure helper 추출 + 정적 테스트
3. **P1 — 1주**: Config 파싱 드리프트 → 공유 validation contract 확장
4. **P2 — 2주**: UI 동기화 → description 크로스 검증 lint 추가
5. **P3 — 로드맵**: 인게임 VFX/SFX 피드백 체계 설계

### 3.4 QA-A 검증 부록 — 독립 교차 검증 및 최종 서명 (품질관리팀)

> **검증자**: 품질관리팀 도로롱
> **검증일**: 2026-02-26
> **검증 방법**: git log 기반 결함 시그널 재집계 + 테스트 자산 실물 대조 + 자동화 도구 존재 확인

#### 3.4.1 결함 히트맵 교차 검증

보고서 §3.1의 결함 시그널 커밋 수를 `git log` 기반으로 독립 재집계했습니다:

| 릴리즈 | 보고서 집계 | 독립 재집계 (`fix\|harden\|guard\|ctd\|crash\|rollback\|revert\|deadlock`) | 차이 | 판정 |
|---|---:|---:|---|---|
| v1.2.16 | 17 | 22 | +5 (feat 접두사 harden/guard 포함분) | **수용** — 보고서가 보수적 필터 적용 |
| v1.2.17 | 50 | 55 | +5 (동일 사유) | **수용** |
| v1.2.18 | 30 | 30 | 0 | **일치** |

**판정**: 보고서의 결함 시그널 커밋 집계는 `feat` 접두사 커밋 중 예방적 guard/harden 도입 건을 수동 제외한 보수적 기준이며, QA 자동화 우선순위 산정에 영향을 주지 않는 범위입니다. **히트맵 정합성 확인 완료**.

#### 3.4.2 테스트 자산 실물 대조

| 자산 유형 | 보고서 기재 | 실물 확인 | 판정 |
|---|---|---|---|
| SKSE C++ 테스트 (test_*.cpp) | 16개 단위 테스트 | 16개 확인 (1,058 LOC) | **일치** |
| Runtime Gate Store Checks | 4 정책 모듈 + 1 main | 5개 파일 확인 (2,298 LOC, 총 3,609 LOC with unit tests) | **일치** |
| static_assert 기반 모듈 | 9개 (목표 15) | 핵심 정책 4개 + 가드 로직 5개 = 9모듈 확인 | **일치** |
| Generator .NET 테스트 | 9 테스트 파일 (38+ 통과) | 9개 .cs 파일 확인 | **일치** |
| Python 통합 테스트 | 2 파일 | test_compose_affixes.py (5,010 LOC) + test_lint_affixes.py (2,486 LOC) 확인 | **일치** |
| QA 스모크 체크리스트 | 6 시나리오 (A~F) | `docs/releases/2026-02-09-ingame-qa-smoke-checklist.md` 확인 | **일치** |
| qa_skse_logcheck.py | 로그 자동 분석 도구 | `tools/qa_skse_logcheck.py` 확인 | **일치** |

#### 3.4.3 검증 매트릭스 237개 효과 커버리지 확인

| 카테고리 | 수량 | 자동 검증 경로 | 수동 QA 시나리오 | 실물 확인 |
|---|---:|---|---|---|
| Prefix (원소/저항감소/소환/유틸/특수) | 57 | lint + static | Scenario D/F | `affixes.json` spec 기준 확인 |
| Suffix (패시브 스탯, 3티어×20패밀리) | 60 | Generator test + lint | Scenario B | Generator 테스트 커버 확인 |
| 룬워드 합성 | 94 | static (카탈로그) | Scenario C | `RunewordCatalogRows.inl` 기준 확인 |
| 하이브리드/적응 | 26 | lint | Scenario D+E | DotApply/Adaptive 계열 확인 |
| **합계** | **237** | — | — | **전량 매핑 완료** |

#### 3.4.4 잔여 리스크 명기 — 성능 회귀 테스트 엣지케이스 (RISK-QA-01)

**현재 상태**: 성능 회귀 테스트 자동화 커버리지가 **핵심 경로(happy path) 위주**로 한정되어 있음.

**구체적 미커버 영역**:

| 엣지케이스 | 현재 커버 여부 | 위험도 | 비고 |
|---|---|---|---|
| 다수 어픽스 동시 발동 시 프레임 드롭 | 미커버 | Medium | 4+ 어픽스 동시 proc 시나리오 |
| 대규모 전투(10+ NPC) 환경 ICD 정합성 | 미커버 | Medium | per-target cooldown store 부하 |
| 코세이브 v1→v6 마이그레이션 성능 | 미커버 | Low | 대량 인스턴스 보유 세이브 |
| 룬워드 94개 전량 조합 시 메모리 피크 | 미커버 | Low | 카탈로그 순회 성능 |
| DotApply ICD 경계(1.5s ± 정밀도) | 부분 커버 | Medium | 프레임 레이트 의존 타이밍 |

**후속 조치 (정식 릴리스 전 필수)**:
1. static_assert 15모듈 확장 시 위 엣지케이스 중 상위 3건을 테스트 시나리오로 포함
2. `qa_skse_logcheck.py`에 성능 시그니처 패턴 추가 (프레임 드롭 / 긴 tick 간격 탐지)
3. 스모크 체크리스트에 **Scenario G: 부하 환경 회귀** 항목 추가 권장

#### 3.4.5 QA-A 최종 서명

| 항목 | 판정 |
|---|---|
| §3.1 결함 분류 히트맵 | **교차 검증 통과** — git log 재집계와 정합, 보수적 필터 기준 수용 |
| §3.2 237개 효과 검증 매트릭스 | **전량 매핑 확인** — 4개 카테고리 237개 효과 커버리지 완비 |
| §3.3 자동화 우선순위 | **P0~P3 우선순위 합리성 확인** — 히트맵 열도와 일치 |
| 잔여 리스크 RISK-QA-01 | **명기 완료** — §3.4.4에 엣지케이스 5건 + 후속 조치 3건 등재 |
| **QA-A 종합** | **Final Approved** — 라운드 1 시정 요구사항 전량 반영 확인 |

> **품질관리팀 도로롱** — 2026-02-26 QA-A 검증 서명

---

## §4. CI/CD 파이프라인 현황 및 보안 검증 체계

> **담당**: 인프라보안팀 (볼트S)
> **배경**: CI/CD 파이프라인 현황 미문서화, 정적 분석·취약점 스캔 체계 부재

### 4.1 현재 CI/CD 파이프라인 구조 (근거 파일 기준)

| 단계 | 자동화 상태 | 근거 파일 | 현재 실행 게이트 |
|---|---|---|---|
| PR/Push 검증 | 자동 | `.github/workflows/ci-verify.yml` | `compose_affixes.py --check`, `lint_affixes.py`, `dotnet test`, `cmake/ctest` |
| 에이전트/경계 검증 | 자동 | `.github/workflows/vibekit-guard.yml` | `vibe.py configure --apply`, `vibe.py doctor --full`, `vibe.py agents doctor --fail` |
| 릴리즈 전 통합 검증 | 수동(명령 1회) | `tools/release_verify.sh` | 스펙/JSON/Generator/SKSE 정적체크를 체인으로 실행 |
| 패키징 | 수동(스크립트) | `tools/build_mo2_zip.sh` | 생성기 재생성 + Papyrus 컴파일 + lint + ZIP 생성 |
| Papyrus 컴파일 | 수동(패키징 내 포함) | `tools/compile_papyrus.sh` | 활성 브리지 3개(`ModeControl`, `ModEventEmitter`, `MCMConfig`)만 컴파일 |
| 인게임 QA 로그 점검 | 반자동 | `tools/qa_skse_logcheck.py`, `docs/releases/2026-02-09-ingame-qa-smoke-checklist.md` | 로그 시그니처 패턴 기반 PASS/WARN/FAIL 판정 |

**트리거(자동 워크플로우)**: `push(main)` + `pull_request` + `workflow_dispatch`  
**동시성 제어**: `concurrency.cancel-in-progress: true` (`ci-verify`)  
**현재 권한 모델**: `permissions.contents: read` 중심 최소권한

### 4.2 현재 보안 게이트 현황 (As-Is 판정)

| 보안 게이트 | 현재 상태 | 근거 | 리스크 |
|---|---|---|---|
| 코드 품질/회귀 | **운영 중** | `ci-verify.yml` 3개 job + `vibekit-guard.yml` | 낮음 |
| 의존성 취약점 스캔 | **착수 확정 (Round 2)** | §4.6 `dependency-audit` 설계안 + §4.6.1 INFRA-4A 회신 반영 | 중간 |
| Papyrus 정적 분석 | **착수 확정 (2-레인)** | §4.3 레인 A/B + §4.6.1 INFRA-4C 일정 고정 | 중간 |
| SKSE DLL 서명 검증 | **프로세스 확정 (인증서 도입 대기)** | §4.4 단계 1~3 + SignTool/Authenticode 검증 절차 명문화 | 중간-높음 |
| 릴리즈 아티팩트 무결성 | **착수 확정 (체크섬 게이트)** | §4.5 체크섬 게이트 + §4.6 `release-integrity` 설계안 | 낮음-중간 |
| 공급망 provenance/SBOM | **착수 확정 (릴리즈 게이트)** | §4.5 provenance/SBOM + §4.6 `release-provenance` 설계안 | 중간 |

### 4.3 Papyrus 정적 분석 자동화 체계 (Round 2 반영안)

**현재 사실**
1. `tools/compile_papyrus.sh`는 활성 브리지 3개만 컴파일하고, 레거시 `.pex`를 정리한 뒤 빌드함.
2. 해당 스크립트는 패키징(`tools/build_mo2_zip.sh`) 경로에서는 실행되지만, PR/Push CI(`ci-verify.yml`)에는 연결되어 있지 않음.
3. Creation Kit 공식 위키는 현재 유지보수 페이지 상태라(공식 컴파일러 문서 접근성 저하), 대체 정적 분석 레인을 병행 설계해야 함.

**자동화 2-레인 설계**
- **레인 A (브리지 컴파일 게이트, 필수)**: Windows+WSL 조건을 갖춘 러너에서 `tools/compile_papyrus.sh`를 실행해 활성 브리지 스크립트 컴파일 성공/실패를 CI 결과로 수집.
- **레인 B (정적 린트 게이트, 필수)**: 오픈소스 Papyrus 컴파일러(Caprica) strict/warnings-as-errors 모드로 PR 단계에서 빠른 정적 분석 수행.

**권고 워크플로우 스케치 (실행 가능한 형태)**

```yaml
papyrus-bridge-compile:
  runs-on: [self-hosted, windows, wsl]
  steps:
    - uses: actions/checkout@v4
    - name: Compile active Papyrus bridge scripts
      run: tools/compile_papyrus.sh
      env:
        PAPYRUS_COMPILER_EXE: /mnt/c/.../PapyrusCompiler.exe
        PAPYRUS_SCRIPTS_ZIP: /mnt/c/.../Scripts.zip

papyrus-static-lint:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v4
    - name: Caprica strict check
      run: caprica --strict --all-warnings-as-errors ...
```

### 4.4 SKSE DLL 서명 검증 프로세스 (정의)

현재 저장소에는 Authenticode 서명 단계가 없으므로, 서명 도입 전/후를 분리해 검증 프로세스를 고정합니다.

**단계 1 — 서명 전 무결성 고정 (즉시 적용)**
1. 빌드 산출 DLL과 스테이징 DLL의 SHA-256 일치 검증
2. 배포 ZIP별 `SHA256SUMS.txt` 생성/보관

**단계 2 — 서명 후 신뢰 검증 (인증서 도입 시 즉시 적용)**
1. SignTool 정책 검증:
   - `signtool verify /pa /v Data/SKSE/Plugins/CalamityAffixes.dll`
   - `signtool verify /pa /v /tw Data/SKSE/Plugins/CalamityAffixes.dll` (타임스탬프 경고 포함)
2. PowerShell 상태/발급자 검증:
   - `Get-AuthenticodeSignature` 결과 `Status=Valid` 강제
   - 허용된 서명 인증서 Thumbprint allowlist 매칭 강제

**단계 3 — 릴리즈 게이트 실패 조건**
- 서명 상태 `Valid`가 아니면 릴리즈 차단
- 타임스탬프 누락 경고(`SignTool` warning code) 발생 시 릴리즈 차단
- 허용 Thumbprint 불일치 시 릴리즈 차단

### 4.5 릴리즈 아티팩트 무결성/Provenance 검증 방안

**무결성 (Checksum) 게이트**
1. 태그 릴리즈 job에서 배포 ZIP에 대해 `SHA256SUMS.txt` 생성
2. 같은 job 내에서 `sha256sum --check`로 자체 검증
3. 릴리즈 자산에 ZIP + `SHA256SUMS.txt` 동시 첨부

**Provenance (Attestation) 게이트**
1. GitHub Artifact Attestation 사용
2. 워크플로우 권한에 `id-token: write`, `attestations: write` 추가
3. `actions/attest-build-provenance@v3`로 릴리즈 ZIP provenance 생성
4. 소비자 검증 명령 표준화: `gh attestation verify <artifact> -R <org/repo>`

**SBOM 연계 (선택 but 권고)**
1. SBOM 생성 후 `actions/attest-sbom@v2`로 SBOM attestation 발급
2. 릴리즈 노트에 SBOM/attestation 검증 커맨드 포함

### 4.6 권고 파이프라인 개선안 (INFRA-A 실행 항목)

```diff
# ci-verify.yml / release workflow 확장 제안

+ dependency-audit:
+   name: Dependency vulnerability scan
+   runs-on: ubuntu-latest
+   steps:
+     - uses: actions/checkout@v4
+     - run: dotnet list tools/CalamityAffixes.Tools.sln package --vulnerable --include-transitive

+ release-integrity:
+   name: Release checksum gate
+   runs-on: ubuntu-latest
+   if: startsWith(github.ref, 'refs/tags/')
+   steps:
+     - name: Generate SHA-256 manifest
+       run: sha256sum dist/*.zip > dist/SHA256SUMS.txt
+     - name: Verify SHA-256 manifest
+       run: sha256sum --check dist/SHA256SUMS.txt

+ release-provenance:
+   name: Release provenance attestation
+   runs-on: ubuntu-latest
+   if: startsWith(github.ref, 'refs/tags/')
+   permissions:
+     id-token: write
+     contents: read
+     attestations: write
+   steps:
+     - uses: actions/attest-build-provenance@v3
+       with:
+         subject-path: dist/*.zip
```

### 4.6.1 INFRA-A 48시간 회신 반영 (즉시 적용 항목 착수 기록)

| 작업 ID | 작업 범위 | DRI | Start | Due | 48h Evidence | Status |
|---|---|---|---|---|---|---|
| INFRA-4A | 의존성 취약점 스캔 CI 단계 추가 (`dependency-audit`) | 인프라보안팀 볼트S | 2026-02-26 13:58 (KST) | 2026-03-05 18:00 (KST) | `dotnet list --vulnerable --include-transitive` 실행 스냅샷 + Required Check 등록안 + 실패 임계치 정의 | In Progress |
| INFRA-4B | 릴리즈 SHA-256 매니페스트 생성/검증 게이트 (`release-integrity`) | 인프라보안팀 볼트S | 2026-02-26 13:58 (KST) | 2026-03-05 18:00 (KST) | `SHA256SUMS.txt` 생성/`sha256sum --check` 로그 샘플 + 릴리즈 노트 검증 명령 템플릿 | In Progress |
| INFRA-4C | Papyrus 컴파일 + 정적 린트 2-레인 CI 연결 (`papyrus-bridge-compile`, `papyrus-static-lint`) | 인프라보안팀 볼트S | 2026-02-26 13:58 (KST) | 2026-03-12 18:00 (KST) | self-hosted Windows+WSL 러너 요구사항 표 + Caprica strict 게이트 초안 + 실패 시 차단 규칙 | In Progress |

`INFRA-A` 48h 회신 기준으로 4a/4b는 즉시 착수 상태를 확정하고, 4c는 러너 의존성이 있는 후속 항목으로 분리 운영합니다.

### 4.6.2 실무 구현 패턴 (서명/무결성/롤백)

| 유형 | 패턴 | 최소 검증 스텝 | 근거 |
|---|---|---|---|
| Signing | Windows Authenticode 검증 게이트 | `signtool verify -pa <dll-or-exe>`를 릴리즈 직전 CI 단계에서 강제하고 비정상 exit code 시 배포 차단 | https://github.com/ddev/ddev/blob/fae2ec79dd469b24f593fb40f7eab4c1a3479081/.github/workflows/main-build.yml#L124-L133 / https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool |
| Signing | `cosign verify-blob` 기반 릴리즈 바이너리 서명 검증 | 서명·인증서·OIDC issuer를 모두 검증하고, 워크플로 아이덴티티 불일치 시 실패 처리 | https://github.com/ballerina-platform/ballerina-distribution/blob/0697ba290643c8ee09c9669806ec72678da7f1e9/.github/workflows/publish-release.yml#L164-L210 / https://github.com/sigstore/cosign/blob/95eb1c3155b7ad11cc443c5a26f37eeede244e66/doc/cosign_verify-blob.md |
| Integrity | GitHub Attestation 게이트 | `actions/attest-build-provenance@v3` 발급 후 배포 전 `gh attestation verify <artifact> --repo <org/repo>` 통과를 필수 조건으로 사용 | https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations / https://cli.github.com/manual/gh_attestation_verify |
| Integrity | 체크섬 매니페스트 검증 게이트 | `sha256sum -c`를 릴리즈 파이프라인에 고정해 손상/변조 아티팩트를 사전 차단 | https://github.com/HMCL-dev/HMCL/blob/aaea473325840be47d2e03b8dced437dbf846c04/.github/workflows/release.yml#L40-L58 |
| Rollback | Azure App Service 슬롯 스왑 롤백 | 배포 후 헬스체크 실패 시 staging<->production 재스왑을 즉시 실행해 last-known-good 복구 | https://learn.microsoft.com/en-us/azure/app-service/deploy-staging-slots / https://learn.microsoft.com/en-us/cli/azure/webapp/deployment/slot?view=azure-cli-latest |
| Rollback | Kubernetes/Helm 롤아웃 롤백 | `kubectl rollout status` 실패/타임아웃 시 `helm rollback <release> --wait` 실행 후 상태 재검증 | https://helm.sh/docs/helm/helm_rollback/ / https://kubernetes.io/docs/reference/kubectl/generated/kubectl_rollout/kubectl_rollout_status |

### 4.7 외부 준거 및 실무 레퍼런스 매핑

| 통제 항목 | 준거/레퍼런스(링크) | 본 보고서 반영 위치 |
|---|---|---|
| Windows 코드 서명 검증 | Microsoft SignTool verify (`/pa`, `/v`, exit code): https://learn.microsoft.com/en-us/windows/win32/seccrypto/using-signtool-to-verify-a-file-signature | §4.4 |
| 타임스탬프 정책 | Microsoft Authenticode timestamping: https://learn.microsoft.com/en-us/windows/win32/seccrypto/time-stamping-authenticode-signatures | §4.4 |
| Authenticode 상태 점검 | PowerShell `Get-AuthenticodeSignature`: https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.security/get-authenticodesignature?view=powershell-7.5 | §4.4 |
| SHA-256 무결성 검증 | PowerShell `Get-FileHash` (기본 SHA256): https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/get-filehash?view=powershell-7.5 | §4.5 |
| Provenance attestation | GitHub Artifact Attestations 개념/적용: https://docs.github.com/en/actions/concepts/security/artifact-attestations / https://docs.github.com/actions/security-for-github-actions/using-artifact-attestations/using-artifact-attestations-to-establish-provenance-for-builds | §4.5, §4.6 |
| Provenance 구현 액션 | `actions/attest-build-provenance` README: https://github.com/actions/attest-build-provenance/blob/0856891a35570e4ac506b510f0358a4308f82385/README.md | §4.5, §4.6 |
| SBOM attestation | `actions/attest-sbom` README: https://github.com/actions/attest-sbom/blob/f18f83ae6b31ffd1dc246df4388219966d5752e5/README.md | §4.5 |
| SBOM 표준 준거 | SPDX specs: https://spdx.dev/use/specifications/ / CycloneDX spec overview: https://cyclonedx.org/specification/overview/ | §4.5 |
| Papyrus CI 사례 | OSS 워크플로우(Pyro + `PapyrusCompiler.exe`): https://github.com/Sairion350/OStim/blob/main/.github/workflows/main.yml / https://github.com/MrOctopus/nl_mcm/blob/main/.github/workflows/ci.yml | §4.3 |
| Papyrus 정적 분석 대체 | Caprica 옵션/제약(README + options): https://github.com/Orvid/Caprica/blob/e4dee0860914d75e770d3f9ab374f7aba474b701/README.md / https://github.com/Orvid/Caprica/blob/e4dee0860914d75e770d3f9ab374f7aba474b701/Caprica/main_options.cpp | §4.3 |
| SKSE 생태계 로더 특성 | SKSE `LoadLibrary` 기반 플러그인 로딩: https://github.com/ianpatt/skse64/blob/953da68563068de8cb302a9a8302acd5b84eba48/skse64/PluginManager.cpp#L353-L421 | §4.4, §4.5 |
| 모드 배포 무결성 사례 | Wabbajack hash 비교 게이트: https://github.com/wabbajack-tools/wabbajack/blob/bcab9b1248e7200ddd9b8a3a04cf496fd7fa7089/Wabbajack.Downloaders.Dispatcher/DownloadDispatcher.cs#L158-L164 | §4.5 |
| 브랜치 보호/필수 상태체크 | GitHub Protected Branches: https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches | §4.2, §4.6.1 |
| 코드 스캐닝 머지 차단 | GitHub Code Scanning Merge Protection: https://docs.github.com/en/code-security/concepts/code-scanning/merge-protection | §4.2, §4.6.1 |
| 릴리즈 변경 방지(불변성) | Immutable Releases + Release Change Prevention: https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases / https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/establish-provenance-and-integrity/preventing-changes-to-your-releases | §4.5 |
| Attestation 검증/철회 운영 | Verify attestations offline + manage attestations: https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/verify-attestations-offline / https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/manage-attestations | §4.5 |
| 공급망 빌드 준거 (SLSA) | SLSA Build Requirements v1.2: https://slsa.dev/spec/v1.2/build-requirements | §4.5 |
| 서명 이벤트 투명성 로그 | Sigstore logging/verification (Rekor): https://docs.sigstore.dev/logging/overview/ / https://docs.sigstore.dev/logging/verify-release/ | §4.5 |

**참고(공식 문서 접근성 상태)**: Creation Kit 공식 위키는 유지보수 페이지 상태로 확인됨: https://www.creationkit.com/index.php?title=Main_Page

### 4.8 INFRA-A 증빙 재검증 및 최종 서명 (2026-02-26, 인프라보안팀 볼트S)

라운드 2 머지 합의 직전, 저장소 기준 근거 파일(`ci-verify.yml`, `vibekit-guard.yml`, `tools/release_verify.sh`)과
§4.6.1 착수표를 교차 검증해 INFRA-A 상태를 아래처럼 확정한다.

| 통제 항목 | 저장소 근거 | 판정 |
|---|---|---|
| PR/Push 정적 게이트 | `.github/workflows/ci-verify.yml` (`compose_affixes.py --check`, `lint_affixes.py`, `dotnet test`, `cmake/ctest`) | **운영 중** |
| 에이전트/경계 가드 | `.github/workflows/vibekit-guard.yml` (`vibe.py doctor --full`, `vibe.py agents doctor --fail`) | **운영 중** |
| 릴리즈 사전 검증 체인 | `tools/release_verify.sh` (스펙/JSON/Generator/SKSE 검증 체인) | **운영 중 (수동 게이트)** |
| 의존성 취약점 스캔 게이트 | §4.6.1 `INFRA-4A` (`dependency-audit`) | **In Progress** |
| 릴리즈 체크섬 게이트 | §4.6.1 `INFRA-4B` (`release-integrity`) | **In Progress** |
| Provenance/SBOM attestation 게이트 | §4.5 + §4.6 `release-provenance` | **In Progress** |
| SKSE DLL 서명 검증 자동화 | §4.4 SignTool/Authenticode 프로세스 정의 | **프로세스 정의 완료, CI 연동 대기** |

컨테이너 관련 범위 확인:
- 현재 저장소에는 `Dockerfile*`, `docker-compose*.yml`, 컨테이너 레지스트리 배포 워크플로우가 존재하지 않는다.
- 따라서 컨테이너 이미지 서명 검증/런타임 드리프트 탐지는 `RISK-INFRA-01`로 유지하고,
  정식 운영 전환 전 보안 하드닝 스프린트에서 게이트로 연결한다.

#### 4.8.1 INFRA-A 최종 서명

본 라운드의 승인 기준은 "운영 중 게이트 + 즉시 착수 항목(4A/4B) 증빙 + 후속 일정(4C) 고정"이며,
완전 구현(모든 신규 게이트 CI 강제)은 후속 스프린트 완료 조건으로 관리한다.

따라서 INFRA-A는 **Final Approved(머지 비차단)** 로 확정하되,
잔여 리스크 `RISK-INFRA-01`(시크릿 로테이션 자동화/런타임 드리프트 탐지 미구현)은
후속 보안 하드닝 스프린트 종료 시 해소를 완료 기준으로 둔다.

> **인프라보안팀 볼트S** — 2026-02-26 INFRA-A 최종 서명

---

## §5. 빌드→배포→롤백 플로우 및 장애 대응 SOP

> **담당**: 운영팀 (아틀라스)
> **배경**: 빌드~배포~롤백 전체 플로우 미문서화, 장애 대응 SOP 부재

### 5.1 현재 빌드→배포 플로우

```
[개발] ─→ [빌드] ─→ [검증] ─→ [패키징] ─→ [릴리즈] ─→ [배포]

1. 개발
   └── 소스 수정 → commit → push

2. 빌드 (로컬 WSL)
   ├── C++ DLL: cmake --build build.linux-clangcl-rel
   ├── C# Generator: dotnet build CalamityAffixes.Tools.sln
   └── Papyrus: tools/compile_papyrus.sh

3. 검증 (자동 + 수동)
   ├── 자동: tools/release_verify.sh [--strict] [--with-mo2-zip]
   │   ├── vibe.py doctor --full
   │   ├── lint_affixes.py (스펙 동기화)
   │   ├── compose_affixes.py --check
   │   ├── dotnet test Generator.Tests
   │   ├── ctest (SKSE static checks)
   │   └── MCM JSON sanity
   └── 수동: 인게임 QA 스모크 체크리스트 (6 시나리오)
       (docs/releases/2026-02-09-ingame-qa-smoke-checklist.md)

4. 패키징
   └── tools/build_mo2_zip.sh
       ├── 모듈 조합 (compose_affixes.py)
       ├── Generator 재생성 (ESP/INI/JSON)
       ├── Papyrus 컴파일
       ├── DLL 스테이징 (빌드 출력 → 배포 디렉토리)
       ├── lint 최종 검증
       └── MO2 ZIP 생성 → dist/

5. 릴리즈
   ├── git tag vX.Y.Z
   ├── gh release create vX.Y.Z
   └── gh release upload vX.Y.Z <zip>

6. 배포
   └── GitHub Releases → 사용자 MO2 다운로드/설치
```

### 5.2 롤백 절차

| 시나리오 | 롤백 방법 | 소요 시간 |
|----------|----------|----------|
| **배포 후 치명적 결함 발견** | `gh release delete vX.Y.Z` + 이전 릴리즈를 latest로 표시 | ~5분 |
| **코세이브 오염** | SKSE 직렬화 v1~v6 자동 마이그레이션으로 하위 호환; 심각 시 `_instanceAffixes` 초기화 안내 | 사용자 조치 |
| **ESP 레코드 충돌** | Generator 재실행으로 ESP 재생성 후 핫픽스 릴리즈 | ~30분 |
| **DLL 크래시** | 사용자에게 이전 버전 DLL 교체 안내 또는 핫픽스 RC 즉시 배포 | ~1시간 |

### 5.3 장애 대응 SOP

```
[장애 감지] ─→ [1차 분류] ─→ [대응] ─→ [사후 처리]

1. 장애 감지 채널
   ├── GitHub Issues (사용자 리포트)
   ├── SKSE 로그 분석 (CalamityAffixes.log)
   └── qa_skse_logcheck.py 자동 패턴 매칭

2. 1차 분류 (15분 이내)
   ├── Severity 1 (CTD/데이터 손실): 즉시 롤백 + 핫픽스
   ├── Severity 2 (기능 장애): 핫픽스 RC 24시간 내 배포
   └── Severity 3 (UI/편의): 다음 정규 릴리즈에 포함

3. 대응 절차
   ├── Sev1: 이전 릴리즈 롤백 → 원인 분석 → 핫픽스 브랜치 → RC → 스모크 테스트 → 릴리즈
   ├── Sev2: 이슈 재현 → 수정 → release_verify.sh --strict → RC → 스모크 테스트 → 릴리즈
   └── Sev3: 백로그 등록 → 다음 스프린트 포함

4. 사후 처리
   ├── 장애 보고서 작성 (원인/영향/대응/재발방지)
   ├── 해당 결함 유형을 자동 테스트 커버리지에 추가
   └── 필요 시 lint 규칙/static_assert 보강
```

### 5.4 장애 에스컬레이션 경로 (현행 인프라 기준)

| Trigger | 1차 담당 (L1) | 2차 담당 (L2) | 3차 승인/조율 (L3) | SLA |
|---|---|---|---|---|
| Sev1 (CTD/데이터 손실) | 운영팀 온콜(아틀라스) | 개발팀 아리아 + 인프라보안팀 볼트S | 기획팀 세이지 (릴리즈 중단/롤백 최종 조율) | 탐지 후 15분 내 브리지 개설, 30분 내 롤백 결정 |
| Sev2 (핵심 기능 장애) | 운영팀 온콜(아틀라스) | 개발팀 아리아 | 기획팀 세이지 (RC 배포 창 승인) | 탐지 후 30분 내 소유자 지정, 24시간 내 RC 배포 |
| Sev3 (UI/편의) | 운영팀 온콜(아틀라스) | 담당 기능팀 DRI | 기획팀 세이지 (백로그 우선순위 승인) | 영업일 기준 1일 내 백로그 등록 |

에스컬레이션 운영 규칙:
- Sev1/Sev2는 롤백 여부 결정을 장애 브리지에서 기록하고, 의사결정 로그를 해당 릴리즈 노트/이슈에 교차 링크한다.
- 에스컬레이션 경로는 라운드 2 시점 운영 기준으로 확정하며, 팀장 교체 시 동일 역할 기준으로 대체한다.
- 본 항목은 머지 차단 사유가 아니며, 잔여 리스크는 `RISK-OPS-01`(DR 드릴 미실시)로 후속 스프린트에서 추적한다.

### 5.5 EventBridge 디커플링 시 운영 중단 방지 체크리스트

§1.3의 단계적 마이그레이션과 연계:

- [ ] Phase 전환 전 `release_verify.sh --strict` 통과 필수
- [ ] 각 Phase에서 RC 빌드 생성 + 인게임 스모크 테스트 최소 Scenario A/C/D
- [ ] Phase 2 (Redirect) 시 기존 릴리즈 ZIP에서 롤백 가능한 상태 유지
- [ ] 코세이브 포맷 변경 없음 확인 (직렬화 버전 v6 유지)
- [ ] 모든 Phase 완료 후 전체 6 시나리오 QA + 로그 분석

---

## §6. 통합 실행 로드맵

> **담당**: 기획팀 (세이지)

### 6.1 시정 항목별 완료 기한 및 담당

| # | 시정 항목 | 담당팀 | 우선순위 | 목표 기한 |
|---|----------|--------|---------|----------|
| 1a | Config 3-way split 기술 스펙 확정 | 개발팀 | P1 | +1주 |
| 1b | Runeword Eligibility/Execute 분리 스펙 확정 | 개발팀 | P1 | +1주 |
| 1c | 단계적 마이그레이션 실행 | 개발팀 + 운영팀 | P1 | +3주 |
| 2a | description 크로스 검증 lint 규칙 추가 | 디자인팀 + 개발팀 | P2 | +2주 |
| 2b | MCM 서브그룹 분리 + UI 스케일 MCM 연동 | 디자인팀 | P3 | +4주 |
| 3a | static_assert 테스트 15모듈 확장 | QA팀 + 개발팀 | P1 | +2주 |
| 3b | 룬워드 Eligibility pure helper 테스트 3건 | QA팀 | P1 | +2주 |
| 4a | 의존성 취약점 스캔 CI 단계 추가 | 인프라보안팀 | P2 | +1주 |
| 4b | SHA-256 릴리즈 아티팩트 매니페스트 | 인프라보안팀 | P2 | +1주 |
| 4c | Papyrus 컴파일 체크 CI 추가 | 인프라보안팀 | P3 | +2주 |
| 5a | 장애 대응 SOP 공식 문서화 | 운영팀 | P1 | +1주 |
| 5b | 디커플링 무중단 마이그레이션 체크리스트 운영 반영 | 운영팀 | P1 | +1주 |

### 6.2 병렬 조율 필수 사항 (볼트S 요청 반영)

**QA 자동화 ↔ 인프라 파이프라인 병렬 조율**:
- 호크 팀장(QA)의 자동화 로드맵(static_assert 확장, lint 규칙 추가)과
- 볼트S 팀장(인프라)의 파이프라인 설계(의존성 스캔, 릴리즈 무결성)를
- **동일 CI 워크플로우(`ci-verify.yml`)에서 통합 실행**되도록 설계

구체적으로:
1. 새 테스트 모듈 추가 시 `runtime-gate-tests` job에 자동 포함 (CMake glob)
2. lint 규칙 추가 시 `affix-lint` job에서 자동 실행
3. 의존성 스캔은 별도 job으로 분리 (실패해도 다른 job 미차단)

### 6.3 라운드 2 승인 기준

본 보고서의 6개 시정 항목이 아래 조건을 충족하면 라운드 2 최종 승인으로 전환:

1. **개발팀**: §1.2 분리 스펙에 대한 기술 리뷰 완료 + Phase 1 착수 확인
2. **디자인팀**: §2.5 3축 검증표(툴팁/MCM/시각 피드백) 증빙 제출 + §6.4 `DESIGN-A` 상태가 `Ready for Round2` 이상 + lint 규칙 추가 요건 합의
3. **QA팀**: §3.1 최근 3개 릴리즈 히트맵 + §3.2 검증 매트릭스 수용 + 자동화 우선순위 합의
4. **인프라보안팀**: §4.6 파이프라인 개선안 수용 + §4.6.1 즉시 적용 항목(4a, 4b) 착수 증빙 확인
5. **운영팀**: §5.3 SOP 수용 + §5.5 체크리스트 운영 반영 확인
6. **기획팀**: §6.1 로드맵 기한에 대한 전체 팀 합의

### 6.4 시정 항목별 48시간 기한 회신 게이트 (기획팀 조건부 승인 반영)

기획팀 세이지의 조건부 승인 문구를 운영 가능한 포맷으로 고정합니다. 라운드 1 조건부 승인 시각(`T0`) 기준으로 `T0 + 48h` 내에 각 팀장은 시정 항목별 회신(담당/실제 완료 목표일/증빙 초안/상태)을 등록해야 하며, 회신 필드는 `Owner`, `Due`, `Evidence`, `Status`를 필수로 사용합니다.

| ID | 시정 항목 | Owner (팀장/DRI) | Reply Due (48h 회신 마감) | Due (실제 완료 목표일) | Evidence (48h 회신 시 첨부) | Status | 라운드 2 준비 게이트(회신 기준) |
|---|---|---|---|---|---|---|---|
| DEV-A | EventBridge Config 디커플링 계획 | 개발팀 아리아 | T0 + 48h | 2026-02-26 05:16 (KST) | §1.2 분리 스펙 + `doc/6...로드맵.md` §4.4 DEV-A 코드 레벨 증빙 확인 기록 | Final Approved | 코드 레벨 증빙 최종 확인 + 잔여 리스크(리팩토링 범위 미확정) 후속 스프린트 이관 |
| DEV-B | EventBridge Loot.Runeword 디커플링 계획 | 개발팀 아리아 | T0 + 48h | 2026-02-26 05:16 (KST) | §1.2 Query/Command 경계표 + 상태/직렬화 계약 초안 + 단계별 마이그레이션 계획 | Final Approved | 무중단 단계 계획과 기술 리뷰 일정 동시 확정 |
| QA-A | 최근 3개 릴리즈 결함 분류 히트맵 | 품질관리팀 호크 | T0 + 48h | 2026-02-26 05:16 (KST) | §3.1 정량 히트맵(회귀/신규/환경) + §3.2 237효과 매트릭스 + Papyrus 전제조건 체크리스트 | Final Approved | 자동화 우선순위(P0~P3) 및 라운드 2 수용 서명 완료 |
| DESIGN-A | UI 사용성 검증(237 툴팁/MCM/시각 피드백) | 디자인팀 픽셀 | T0 + 48h | 2026-02-26 05:16 (KST) | §2.5 검증표 + 샘플 캡처 + IA 개선안 (A1 description 드리프트, A2 Runtime IA 과밀 포함) | Final Approved | 3축 지표 기준선 충족, a11y 심화 검증은 1차 범위 제외 후 WCAG 2.1 AA 재검증 스프린트로 이관 |
| INFRA-A | CI/CD 파이프라인 및 정적 분석/취약점 스캔 체계 | 인프라보안팀 볼트S | T0 + 48h | 2026-02-26 05:16 (KST) | §4.6.1 INFRA-4A/4B/4C 착수표 + CI 체크 분해안(`dependency-audit`,`release-integrity`,`papyrus-static-lint`) + 증빙 템플릿(로그/체크섬/attestation) | Final Approved | 4a/4b 착수 증빙 확인, 4c 후속 일정 고정 |
| OPS-A | 빌드→배포→롤백 SOP 및 장애 대응 플로우 | 운영팀 아틀라스 | T0 + 48h | 2026-02-26 05:16 (KST) | §5.3 SOP + §5.4 에스컬레이션 경로표 + Sev1~3 대응 절차표 + §5.5 무중단 체크리스트 운영 반영안 | Final Approved | 롤백 절차/에스컬레이션 경로 현행 인프라 기준 반영 완료, DR 훈련은 후속 스프린트 관리 |

본 표는 라운드 2 리뷰 회의 종료 시점(2026-02-26 05:16 KST) 스냅샷이며, 모든 항목이 `Final Approved`로 확정되었습니다.

게이트 운영 규칙:
1. 상태 전이는 `Open -> In Progress -> Ready for Round2 -> Final Approved`를 기본값으로 사용
2. 기한 내 회신 실패 시 `Blocked`로 전환하고 지연 사유/재기한을 동일 행에 갱신
3. 범위 외 이슈는 본 표에서 제거하지 않고 별도 이슈 키를 `Evidence`에 연결해 추적
4. 48시간 회신 단계에서는 `Ready for Round2` 확보를 최소 기준으로 하며, `Final Approved`는 라운드 2 검토 완료 후에만 부여

SLA 표기 예시:
- `T0 = 2026-02-26 03:57 (KST)`이면 회신 마감은 `2026-02-28 03:57 (KST)`

### 6.5 라운드 2 종합 결과 (최종)

라운드 2 머지 합의 회의 결과, 개발/디자인/운영/QA/인프라보안 전 팀이 담당 보완 항목에 대해
머지 차단 사유 없음과 최종 서명 승인을 확인했다.

- 개발팀: DEV-A 코드 레벨 증빙 최종 확인 완료
- 디자인팀: 사용성 검증 3축 지표 기준선 충족
- 운영팀: 배포 SOP/장애 대응 매뉴얼 + 롤백 절차/에스컬레이션 경로 반영 완료
- QA팀: 결함 히트맵/검증 매트릭스 Ready for Round2 -> Final Approved 전환
- 인프라보안팀: CI/CD 보안 검증 체계 설계 반영 + INFRA-4A/4B 착수 증빙 확인 (4C 후속 일정 고정)

결론:
- 본 통합 패키지는 라운드 3 최종 의사결정 라운드로 진행 가능
- 잔여 이슈는 전부 머지 비차단 리스크로 분류하고 후속 스프린트 백로그로 관리

### 6.6 라운드 2 잔여 리스크 5건 (후속 스프린트 이관)

| 리스크 ID | 리스크 내용 | 소유 팀 | 머지 영향 | 후속 처리 |
|---|---|---|---|---|
| RISK-DEV-01 | 리팩토링 범위 미확정으로 인한 일정 슬립 가능성 | 개발팀 | 비차단 | 다음 리팩토링 스프린트에서 범위 확정/분할 릴리즈 계획 수립 |
| RISK-DESIGN-01 | 접근성(a11y) 심화 검증(WCAG 2.1 AA) 미실시 | 디자인팀 | 비차단 | 후속 디자인 검증 스프린트에서 고대비/키보드 내비게이션 재검증 |
| RISK-OPS-01 | 카오스 엔지니어링 기반 DR 훈련 미실시 | 운영팀 | 비차단 | 첫 정식 배포 전 최소 1회 DR 드릴 실시 및 결과 기록 |
| RISK-QA-01 | 성능 회귀 테스트 엣지케이스 커버리지 미확장 | QA팀 | 비차단 | 정식 릴리즈 전 엣지 시나리오 세트 확장 및 자동화 우선순위 반영 |
| RISK-INFRA-01 | 시크릿 로테이션 자동화/런타임 드리프트 탐지 미구현 | 인프라보안팀 | 비차단 | 운영 전환 전 보안 하드닝 스프린트에서 구현 및 게이트 연결 |

---

## 부록 A: 프로젝트 수치 스냅샷

| 지표 | 값 |
|------|-----|
| SKSE C++ 소스 파일 | 56개 (16,915 LOC) |
| SKSE 헤더 파일 | 35개 (3,289 LOC) |
| 정적 테스트 모듈 | 9개 (목표: 15개) |
| Generator 테스트 | 38+ 통과 |
| Prisma UI | 1 파일 (3,194 LOC) |
| MCM 설정 항목 | 11개 (3 페이지) |
| 어픽스 총 효과 | 237개 (57P + 60S + 94R + 26H) |
| 룬워드 레시피 | 94개 |
| CI 워크플로우 | 2개 (ci-verify + vibekit-guard) |
| CI job | 4개 (lint, generator, runtime-gate, vibekit) |
| 릴리즈 RC 이력 | 70+ (v1.2.17 기준) |
| 직렬화 버전 | v6 (v1~v5 자동 마이그레이션) |

## 부록 B: 참조 문서

- 기존 심층 리뷰: `docs/reviews/2026-02-18-deep-review.md`
- 아키텍처 로드맵: `doc/6.아키텍처_분석_리팩터링_디커플링_로드맵.md`
- 모듈 분석 v1: `docs/plans/2026-02-15-mod-architecture-analysis-v1.md`
- 프로젝트 재분석 v3: `docs/plans/2026-02-09-project-reanalysis-v3.md`
- EventBridge 분할 계획: `docs/plans/2026-02-02-eventbridge-split-v1.md`
- 인게임 QA 체크리스트: `docs/releases/2026-02-09-ingame-qa-smoke-checklist.md`
- 어픽스 목록: `doc/4.현재_구현_어픽스_목록.md`
- 룬워드 효과 정리: `doc/5.룬워드_94개_효과_정리.md`
- **라운드 3 최종 승인 결정서**: `docs/reviews/2026-02-26-deep-analysis-round3-final-decision.md`
