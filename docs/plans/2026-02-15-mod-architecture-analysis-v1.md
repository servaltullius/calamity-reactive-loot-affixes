# Mod 코드분석 / 리팩터링 / 모듈화 / 디커플링 설계분석 (2026-02-15)

## 0) TL;DR

- 이번 턴에서 **분석 + 실제 리팩터링 1~2단계**를 함께 수행했다.
- 분석 결과, 결합도 상위는 여전히 `EventBridge` 축에 집중되어 있고, UI 쪽은 `PrismaTooltip.cpp` 단일 파일 집중도가 높았다.
- 즉시 적용 가능한 저위험 분리로 `PrismaTooltip.cpp`의 레이아웃 파싱/영속화 책임을
  `PrismaLayoutPersistence` 모듈로 추출해 결합을 낮췄다.
- 추가로 `EventBridge.Config.cpp`의 `LoadConfig()`에서
  인덱싱/합성 책임을 `IndexConfiguredAffixes`, `RegisterSynthesizedAffix`,
  `SynthesizeRunewordRuntimeAffixes`로 분리했다.
- 분리 후 `PrismaTooltip.cpp`는 약 `1776 -> 1562 LOC`로 감소했다.

## 1) 현재 구조 진단 (스냅샷)

### 1.1 크기/복잡도 관찰

- `python3 scripts/vibe.py doctor --full` 기준 주요 대형 파일:
  - `skse/CalamityAffixes/src/EventBridge.Config.cpp` (2065 LOC)
  - `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.cpp` (1659 LOC)
  - `skse/CalamityAffixes/src/PrismaTooltip.cpp` (리팩터링 후 1562 LOC)

### 1.2 결합도 관찰

- 변경 결합(최근 커밋 기반) 상위:
  - `EventBridge.h <-> EventBridge.Config.cpp`
  - `EventBridge.h <-> EventBridge.Loot.*`
  - `affixes.json <-> EventBridge.Config.cpp`
- 의미:
  - 런타임 핵심 정책/파서/상태가 `EventBridge` 축에 응집되어 있고,
  - 스펙 데이터 변경이 런타임 코어 변경과 함께 자주 이동한다.

## 2) 이번 리팩터링 (실제 반영)

### 2.1 목표

- `PrismaTooltip.cpp`의 혼합 책임 중
  - UI 이벤트 처리/브리지 로직
  - 레이아웃 CSV 파싱
  - 레이아웃 JSON 파일 I/O
  - JSON 직렬화
  가 한 파일에 공존하던 문제를 줄인다.

### 2.2 적용 내용

- 신규 모듈:
  - `skse/CalamityAffixes/include/CalamityAffixes/PrismaLayoutPersistence.h`
  - `skse/CalamityAffixes/src/PrismaLayoutPersistence.cpp`
- 분리된 책임:
  - `ParsePanelLayoutPayload`
  - `ParseTooltipLayoutPayload`
  - `Load*LayoutFile`
  - `Save*LayoutFile`
  - `Encode*LayoutJson`
- `PrismaTooltip.cpp`는 해당 모듈을 호출해
  - 커맨드 파싱 결과 검증
  - 로드/저장 트리거
  - JS 인터롭 push
  에 집중하도록 정리.

### 2.3 추가 분리 (Config 2차)

- 분리 메서드:
  - `LoadRuntimeConfigJson(...)`
  - `ApplyLootConfigFromJson(...)`
  - `ResolveAffixArray(...)`
  - `IndexConfiguredAffixes()`
  - `RegisterSynthesizedAffix(...)`
  - `SynthesizeRunewordRuntimeAffixes()`
- 목적:
  - `LoadConfig()`의 후반부(인덱스 구성 + 룬워드 합성)를 독립 경계로 이동해
    파싱 단계와 적용 단계를 구분.
- 효과:
  - `LoadConfig()`에서 핵심 흐름이
    `파싱 -> 인덱싱 -> 합성 -> 상태확정`으로 읽히도록 개선.
  - 이후 `SynthesizeRunewordRuntimeAffixes()`를 별도 파일/전략 테이블 기반으로
    추가 분해할 수 있는 발판 확보.

### 2.4 추가 분리 (Runeword 튜닝 테이블화)

- 기존 `if/else` 레시피 튜닝 체인을 `RecipeTuning`/`StyleTuning` 테이블로 전환.
- 효과:
  - 튜닝 규칙 변경 시 조건문 추가 없이 데이터 행만 수정하면 됨.
  - 룰 충돌/누락을 정적 데이터로 검토하기 쉬워짐.
### 2.5 효과

- UI 런타임 로직과 파일 포맷/파싱 로직의 경계가 명확해졌다.
- 레이아웃 포맷 변경 시 수정 지점이 단일 모듈로 모인다.
- 추후 `Prisma` 외 UI 채널(예: 다른 오버레이)로 확장 시 재사용 기반이 생겼다.

## 3) 설계 분석 (문제와 권장 경계)

### 3.1 현재 리스크

- `EventBridge` 축의 대형 파일/강결합이 아직 핵심 리스크다.
- 인게임 경로는 정적 테스트 중심이라, 기능 회귀 탐지는 상대적으로 약하다.

### 3.2 권장 모듈 경계 (다음 우선순위)

1. `EventBridge.Config` 분리:
   - JSON 파싱/검증 계층과 런타임 적용 계층 분리.
2. `EventBridge.Loot` 분리:
   - 드롭 결정(롤링) / 인스턴스 상태 반영 / 툴팁 텍스트 구성 분리.
3. `EventBridge.Triggers` 분리:
   - 이벤트 수집(sink) / 필터링 / 액션 디스패치 경계 명시.

### 3.3 분리 원칙

- 파일 I/O와 게임 상태 변경 경로를 분리한다.
- 문자열 커맨드 파싱과 도메인 명령 실행을 분리한다.
- 상태 저장 포맷(JSON)과 UI 전송 포맷(JS payload)을 분리한다.

## 4) 단계별 디커플링 실행안 (v2 제안)

1. `LoadConfig()` 본문의 액션 타입 파싱(`CastSpell`, `SpawnTrap` 등)을 타입별 파서로 추가 분리.
2. `SynthesizeRunewordRuntimeAffixes()`를 별도 파일/모듈로 이동해 `EventBridge.Config.cpp` 부하 축소.
3. 최소 단위 static test를 모듈별로 추가(로더/loot 정규화/룬워드 튜닝 테이블/툴팁 포맷).
4. `vibe.py coupling` 재측정으로 결합도 감소 확인.

## 5) 참고 자료 (설계 근거)

- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
- C++ `std::from_chars` (저비용 파싱): https://en.cppreference.com/w/cpp/utility/from_chars
- Pointer Events / capture (UI 입력 경계): https://developer.mozilla.org/en-US/docs/Web/API/Pointer_events
