# ADR-0003: 트리거 파이프라인(HandleHealthDamage Hook + TESHitEvent 폴백 + DoT Apply-only)

## Status

Accepted

## Context

어픽스 프로크는 “다양한 입력(근접/원거리/마법/소환수)”에서 일관되게 발동해야 한다.

또한 PoE식 설계 기준:

- DoT는 “틱마다 발동”이 아니라 **적용/갱신 시점만** 트리거로 취급
- 전투 중 이벤트 폭주(초당 수십~수백 회)로 인한 CTD/프레임 저하를 피해야 함

실무 제약:

- 일부 모드리스트는 `TESHitEvent`가 지연되거나, 다른 훅/패치로 기대보다 부정확할 수 있다.
- 반대로 vfunc 훅이 덮이면(후킹 충돌) 특정 기능이 무력화될 수 있다.

## Decision

- 기본(우선) 히트 파이프라인: `Actor::HandleHealthDamage` vfunc hook
  - `EvaluateConversion`(피해 전환)과 `EvaluateCastOnCrit`(CoC)는 이 훅에서 처리한다.
  - 마지막에 `EventBridge::OnHealthDamage(...)`로 “Hit/IncomingHit” 프로크 라우팅.
- 폴백 히트 파이프라인: `TESHitEvent`
  - “훅이 실제로 호출되는지”가 관측되지 않을 때만(=훅이 덮였을 가능성) 폴백으로 프로크를 라우팅한다.
- DoT 트리거: `TESMagicEffectApplyEvent` + `CAFF_TAG_DOT` 키워드 필터
  - 대상+MGEF 단위 ICD로 레이트리밋(기본 1.5초)

## Consequences

### Positive

- 일반적인 환경에서 “히트 트리거”가 매우 안정적(특히 근접/원거리/소환수)
- DoT는 apply-only라 이벤트 볼륨이 낮아 안정적
- 훅이 덮인 환경에서도 최소한의 폴백 경로로 “아무것도 안 되는 상태”를 피함

### Negative

- vfunc 훅이 완전히 덮인 환경에서는 Conversion/CoC가 동작하지 않을 수 있음(폴백으로 완전 동일 구현이 어렵다)
- 폴백(`TESHitEvent`)은 정보(히트데이터) 가용성이 환경에 따라 달라 정확도가 낮을 수 있음

### Neutral

- 특정 액션(예: Chain/CorpseExplosion)의 설계는 별도의 안전장치(ICD, dedupe, rate-limit)에 의존한다.

## Alternatives Considered

- Perk EntryPoint(Apply Combat Hit Spell): 구현 속도는 빠르지만 퍼크 충돌/우선순위 이슈 가능
- Papyrus `OnHit`만 사용: 이벤트 신뢰성/성능/중복호출 문제가 커서 SKSE 기준으로는 비권장
- Weapon Enchantment 기반: 충돌/충전/세부 제약이 있어 “범용 트리거”로는 불리

## References

- CommonLibSSE-NG / RE 이벤트 싱크(`TESHitEvent`, `TESMagicEffectApplyEvent`)
- SKSE Serialization / Messaging(kDataLoaded) 패턴

