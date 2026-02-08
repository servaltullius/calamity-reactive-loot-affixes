# ADR-0001: 인스턴스 어픽스 저장(ExtraUniqueID + SKSE Serialization)

## Status

Accepted

## Context

우리는 Diablo/PoE 스타일의 “어픽스(확률 발동 효과)”를 **모든 모드템(무기/방어구)**에 적용하고 싶다.

핵심 요구사항:

- **CK 작업 최소화**(가능하면 “0 CK”에 가깝게)
- 유저가 파밍/제작으로 얻는 아이템이 “유니크처럼” **그 인스턴스만의 어픽스**를 가지게
- 세이브/로드 이후에도 어픽스가 유지되어야 함
- 특정 아이템(유니크/모드 아이템)과 충돌 없이 동작해야 함

제약:

- Keyword/KID 기반만으로는 “설명 표시”는 가능하지만, “획득 시 리롤 + 인스턴스 고정”을 엔진 구조상 깔끔하게 처리하기가 어렵다.
- 바닐라 Enchantment 레코드를 매번 CK로 찍는 방식은 제작/유니크/모드 호환 측면에서 작업량과 유지비가 너무 크다.

## Decision

- 아이템 인스턴스 식별은 `RE::ExtraUniqueID`를 기준으로 한다.
- 플러그인 내부에 인스턴스 어픽스를 저장하는 맵을 유지한다:
  - key = `(baseFormID << 16) | uniqueID` (`EventBridge::MakeInstanceKey`)
  - value = `affixToken`(64-bit) = `FNV-1a(affixId)` (`CalamityAffixes::MakeAffixToken`)
- 이 맵은 SKSE Serialization으로 저장/복원한다.
  - v1(과거): `affixId` 문자열을 그대로 저장
  - v2(현재): `affixToken`(64-bit)만 저장 (세이브 크기/메모리 최적화)
  - 로드 시 v1 → v2 자동 마이그레이션(문자열 → 토큰) 지원

## Consequences

### Positive

- CK 없이도 **모드템 포함 모든 장비**에 “획득 시 어픽스 부여” 가능
- 세이브/로드 후에도 인스턴스 어픽스가 유지됨(컨테이너/바닥 드랍 포함)
- “어픽스가 붙은 아이템만 그 효과를 가짐”이라는 ARPG 감성을 가장 자연스럽게 구현
- 코세이브(“.skse”) 저장 크기/로드 시간/메모리 사용량을 크게 줄일 수 있음(문자열 → 64-bit 토큰)

### Negative

- `_instanceAffixes`가 장기 플레이에서 누적될 수 있음(아이템이 파기/판매/디스폰되어도 엔트리가 남을 수 있음)
- Serialization 크기가 아이템 수에 비례해 증가
- `affixId`를 바꾸면(=리네임) 기존 세이브의 토큰이 매칭되지 않음 → **ID는 절대 리네임하지 말고, 새 ID를 추가**하는 방식으로 버전업해야 함

### Neutral

- “인벤토리 기반 프루닝”을 넣으면, 플레이어가 보관 중인(컨테이너) 아이템의 어픽스가 사라질 위험이 있어 신중해야 함
- 대신 비교적 안전한 완화책으로, **플레이어가 월드에 드랍한 레퍼런스가 삭제될 때만**(SKSE form delete callback) 해당 인스턴스 엔트리를 정리하는 방식이 가능

## Alternatives Considered

- Keyword-only(키워드가 곧 어픽스): 구현은 쉽지만 “획득 시 리롤 + 인스턴스 고정”이 약함
- Enchantment 레코드 부여: 바닐라 친화적이지만 CK 작업량/호환 비용이 큼(유저/모드템 다양성에 취약)
- Papyrus-only 저장(퀘스트/알리아스/Global): 데이터 구조/성능/충돌 관리가 어려움

## References

- CommonLibSSE: `RE::TESContainerChangedEvent`의 `uniqueID` 필드(문서/헤더)
- SKSE: Serialization Interface 개요
