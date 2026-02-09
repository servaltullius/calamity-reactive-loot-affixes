# Event Signature References (2026-02-09)

## 목적

- 런타임 이벤트 바인딩/훅 변경 시, 시그니처 불일치로 인한 무응답 회귀를 줄이기 위한 내부 레퍼런스입니다.
- 본 문서는 `EventBridge`와 Papyrus 브리지에서 사용하는 핵심 이벤트 시그니처만 최소 요약합니다.

## Papyrus (CK/UESP)

### OnHit - ObjectReference

- 참조: https://ck.uesp.net/wiki/OnHit_-_ObjectReference
- 시그니처:
  - `Event OnHit(ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)`
- 주의:
  - 파라미터 순서/개수가 다르면 이벤트가 조용히 바인딩 실패할 수 있습니다.

### OnObjectEquipped - Actor

- 참조: https://ck.uesp.net/wiki/OnObjectEquipped_-_Actor
- 시그니처:
  - `Event OnObjectEquipped(Form akBaseObject, ObjectReference akReference)`
- 주의:
  - `akReference`는 `None`일 수 있으므로, 실제 판별은 `akBaseObject` 기준 체크를 기본으로 둡니다.

### OnObjectUnequipped - Actor

- 참조(원문 URL): https://ck.uesp.net/wiki/OnObjectUnequipped_-_Actor
- 접근성 메모:
  - 2026-02-09 기준 자동화 환경에서 해당 페이지가 간헐적으로 `403`을 반환함.
- 현재 코드베이스 가정:
  - `OnObjectEquipped`와 동일한 `(Form akBaseObject, ObjectReference akReference)` 파라미터 형태를 전제로 운용합니다.

## SKSE / CommonLibSSE-NG

### TESHitEvent

- 참조: https://ng.commonlib.dev/TESHitEvent_8h_source.html
- 확인 필드:
  - `cause`
  - `target`
  - `source`

### TESContainerChangedEvent

- 참조: https://ng.commonlib.dev/TESContainerChangedEvent_8h_source.html
- 확인 필드:
  - `oldContainer`
  - `newContainer`
  - `baseObj`
  - `itemCount`
  - `reference`
  - `uniqueID`

## 프로젝트 매핑 포인트

- `skse/CalamityAffixes/src/EventBridge.Triggers.cpp`
  - `TESHitEvent`, `TESDeathEvent`, `TESEquipEvent`, `TESMagicEffectApplyEvent`
- `skse/CalamityAffixes/src/EventBridge.Loot.cpp`
  - `TESContainerChangedEvent`
- `README.md`의 `Papyrus ↔ SKSE 브리지` 섹션과 함께 유지/갱신합니다.
