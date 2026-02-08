# ADR-0002: 어픽스 설명 표시(Prisma UI 오버레이)

## Status

Accepted

## Context

유저 편의성을 위해 “어픽스가 붙었는지” 뿐 아니라 **무슨 효과인지**를 즉시 알 수 있어야 한다.

경험적으로 다음 문제가 반복되었다:

- UI 오버홀(Norden UI 등) 조합에서 Scaleform(ItemCard) 구조가 달라져
  - `_global.skse`가 없거나
  - `ItemCard`/`prototype`가 다르게 로드되어
  - 툴팁/설명 주입이 깨질 수 있다.

따라서 “UI 테마와 decouple”된 표시 방식이 필요하다.

## Decision

- 기본 UX는 Prisma UI 기반 “오버레이 뷰”로 제공한다.
  - 주기적으로(기본 200ms) “현재 선택된 아이템”을 읽고
  - 해당 아이템이 인스턴스 어픽스를 가지면 오버레이에 표시하고
  - 아니면 자동으로 숨긴다.
- Scaleform(ItemCard) 기반 `UiHooks`는 현재 런타임에서 제거되었고, 핵심 UX는 Prisma UI 오버레이 단일 경로로 운영한다.

## Consequences

### Positive

- UI 테마/스킨과 무관하게 **일관된 설명 표시** 확보
- LoreBox/SWF 패치 의존도를 줄여 모드리스트 호환성이 좋아짐
- “템 이름 라벨(짧게)” + “오버레이(길게)” 조합으로 가독성이 좋음

### Negative

- Prisma UI가 선행 모드가 됨
- 폴링 기반이므로 극단적으로 보수적인 환경에서는 부담이 될 수 있음(다만 200ms + TaskInterface로 완화)

### Neutral

- SkyUI/기타 인벤 UI가 없어도(바닐라 인벤) 동작 가능하도록 설계할 수 있지만, “선택된 아이템 접근”은 메뉴별 분기 유지가 필요

## Alternatives Considered

- LoreBox: 강력하지만 SWF/패치/UI 조합에 따른 문제 발생 가능
- Scaleform 직접 주입(`ItemCard` hook): UI마다 구조가 달라 불안정할 수 있음
- Inventory Injector(I4) 아이템 카드 규칙: 설명 표현은 가능하지만 “인스턴스 어픽스” 표시에는 추가 설계가 필요

## References

- Prisma UI 공식 문서(기본 사용법)
- Prisma UI API 예제/템플릿(모드 개발 레퍼런스)
