# Code Review / Refactor / Decoupling Notes (2026-02-02)

## 0) TL;DR

초기에는 `EventBridge.cpp`에 책임이 과도하게 몰려 유지보수 리스크가 컸습니다. 현재는 `EventBridge`를 “파사드”로 남기고 구현을 **Config/Loot/Triggers/Actions/Serialization**로 분리하여(행동 변화 없이) 구조 리스크를 크게 낮췄습니다. 다음 설계 점검 포인트는 **(1) 훅 의존 기능(Conversion/CoC)의 실패 모드**, **(2) 장기 세이브에서 `_instanceAffixes` 누적**, **(3) UI/선행모드/표시 경로의 명시화**입니다.

---

## 1) 현재 아키텍처(요약)

### Runtime 데이터/설정
- 런타임 설정: `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- 인스턴스 어픽스 저장: `ExtraUniqueID`(아이템 인스턴스) → 플러그인 내부 `_instanceAffixes` 맵에 `affixId` 저장 + 세이브 직렬화

### 트리거 공급(“저발동” 정책 포함)
- 기본 히트 파이프라인: `Actor::HandleHealthDamage` vfunc hook → `EventBridge::OnHealthDamage(...)`
- 보조(폴백) 파이프라인: `TESHitEvent` (훅이 덮였을 때만)
- DoT는 tick이 아니라 **apply/refresh 이벤트만** 취급: `TESMagicEffectApplyEvent` + `CAFF_TAG_DOT` 필터 + ICD

### UI/표시
- 기본 UX: Prisma UI 오버레이(`Data/PrismaUI/views/CalamityAffixes/index.html`)로 “선택된 아이템”의 어픽스 설명 표시
- Scaleform(ItemCard) 기반 `UiHooks`는 제거되었고, 현재는 Prisma UI 단일 경로로 운영

---

## 2) 좋은 점(유지해야 할 설계)

- **Prisma UI 오버레이**: UI 스킨(Norden 등)과 decouple 되어 툴팁 호환성이 좋음
- **DoT apply 기반 트리거**: 이벤트 폭주를 피하는 PoE식 설계(틱당 발동 금지)
- **중앙 집중형 “프로크/ICD/가드”**: proc storm 방지에 유리(중복 히트 억제/ICD 등)
- **직렬화로 인스턴스 보존**: 세이브/로드 후에도 어픽스가 유지됨

---

## 3) 리스크/개선 포인트(설계 점검)

### (A) 훅 의존 기능의 실패 모드
- Conversion / Cast-on-Crit(CoC)는 `Actor::HandleHealthDamage` vfunc hook 경로에 의존합니다.
  - 훅이 다른 모드에 의해 덮이거나 우회되는 환경에서는 “프로크 일부는 폴백(TESHitEvent)로 동작”하더라도, Conversion/CoC는 동일하게 보장하기 어렵습니다.
  - 개선 방향: 훅 미관측 시 1회 경고 로그/인게임 메시지(디버그 모드 한정), 또는 대체 구현 가능성 검토(다만 완전 동일 구현은 어려움).

### (B) 테스트 가능성 낮음
- 현재 tests는 **컴파일-타임 static_assert** 위주.
- SKSE/RE 타입 의존이 강한 로직은 테스트하기 어려움 → 순수 로직(파싱/결정/룰)을 분리하면 테스트가 쉬워짐.

### (C) 이벤트/상태 누수 가능성
- 인스턴스 맵 `_instanceAffixes`는 아이템이 사라져도 엔트리가 남을 수 있음(메모리 누적 가능).
  - 당장 크리티컬은 아니지만, 장기 플레이에서 추적/프루닝 전략이 있으면 더 안정적.

### (D) UI 경로 단순화 상태
- 과거 Scaleform(ItemCard) 훅 불확실성은 `UiHooks` 제거로 해소됨.
- 현재 리스크는 “Prisma UI 미설치 시 툴팁 오버레이 비활성”으로 단순화됨.

### (E) “프로크가 프로크를 부르는” 루프 가능성(특히 폴백 경로)
- 기본 경로(HealthDamage 훅)에서는 re-entrancy 가드로 “자기 자신이 뿌린 즉시타격/즉시 주문”이 다시 프로크 파이프라인을 깨우는 루프를 상당 부분 막습니다.
- 하지만 훅이 덮여 폴백(TESHitEvent)만 동작하는 환경에서는, **우리 `CAFF_` 계열 proc spell**이 다시 Hit 이벤트로 들어와 체인/폭주를 만들 가능성이 상대적으로 큽니다.
  - 개선 방향: “우리 proc spell은 트리거 입력으로 취급하지 않음” 가드(예: `CAFF_` EditorID prefix 필터)를 트리거 진입부에 추가(옵션화 가능).

---

## 4) 이번 패치에서 실제로 한 일(행동 변화 없음 위주)

- 중복 제거/디커플링:
  - `skse/CalamityAffixes/include/CalamityAffixes/HitDataUtil.h` 추가
  - `EventBridge.cpp` / `Hooks.cpp`의 `GetLastHitData` 중복 제거
- 안정성:
  - 드랍-줍기 리롤 가드(`LootRerollGuard`)에 `Reset()` 추가 + 테스트 추가
  - `EventBridge::LoadConfig()` / `Revert()`에서 가드 상태 초기화
- 가독성:
  - `skse/CalamityAffixes/src/Hooks.cpp`, `skse/CalamityAffixes/src/main.cpp`, `EventBridge.cpp` 일부 구간 포맷/정리

- 구조(모듈 분리):
  - `EventBridge` 구현을 `EventBridge.Config.cpp` / `EventBridge.Loot.cpp` / `EventBridge.Triggers.cpp` / `EventBridge.Actions.cpp` / `EventBridge.Serialization.cpp`로 분리
  - `EventBridge.cpp`는 `GetSingleton` / `Register`만 남김(파사드)

---

## 5) 추천 리팩터/디커플링 로드맵(다음 단계)

### 옵션 1) “안전/점진” (완료)
`EventBridge` 파사드 + 책임별 TU 분리는 완료했습니다.

### 옵션 2) “Prisma-only 유지”
- 현행 구조를 유지하고, Prisma UI 선행모드 요구사항을 문서/배포 안내에서 명확히 유지

### 옵션 3) “장기 안정성”
- `_instanceAffixes` 프루닝 전략(예: 일정 주기/이벤트에서 인벤토리 기준 정리)
- 디버그 모드에서만 프루닝 로그 활성

### 옵션 4) “콘텐츠 작성자 경험 개선”
- `affixes.json` 로딩 시 검증 강화:
  - 중복 `id` / `editorId` 검출
  - `CastOnCrit`/`ConvertDamage`/`Archmage` 같은 특수 액션의 “의미 있는 trigger만 허용”하고 불일치 시 경고 로그
  - 누락된 spell 참조를 “스킵+명확한 로그”로 처리
