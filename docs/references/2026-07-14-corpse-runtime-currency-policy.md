# Corpse Runtime Currency Policy (2026-07-14)

## 결론

룬워드 조각과 재련 오브의 현재 유일한 실제 드랍 권한은 **SKSE `TESDeathEvent` 기반 eligible hostile corpse-only 경로**다. 설정의 `loot.currencyDropMode=hybrid`는 기존 설정 파일과 MCM 계약을 깨지 않기 위한 레거시 호환 토큰일 뿐, 상자·픽업·월드·SPID를 다시 활성화하지 않는다.

성공한 통화는 바닥에 생성하거나 플레이어에게 바로 지급하지 않고, 사망한 적의 인벤토리에 조용히 직접 추가한다. 따라서 시체를 열었을 때 기존 전리품처럼 보이며, 무관한 상자에서 나오거나 월드에 갑자기 튀어나오는 경로는 없다.

## 변경 이유

이전 하이브리드 설계는 SPID 선분배와 컨테이너 활성화 런타임 롤을 함께 사용했다. 이 방식은 다음 문제가 있었다.

- 적 처치와 관계없는 일반 상자/컨테이너에서도 통화가 판정될 수 있었다.
- 월드 생성 폴백은 아이템이 갑자기 튀어나오는 시각적 이질감을 만들었다.
- SPID 선분배와 런타임 판정이 같은 시체에 겹칠 때 중복 여부를 설명하고 추적하기 어려웠다.
- 런타임 피티와 시체 처리 상태가 저장되지 않아 로드 경계에서 일관성이 약했다.

새 정책은 판정 시점을 적대 대상의 사망으로 고정하고, 결과를 해당 시체에만 넣어 원인과 결과를 일치시킨다.

## 권한과 소스 매트릭스

| 소스/행동 | 통화 판정 | 설명 |
|---|---:|---|
| 플레이어가 적대 대상을 처치 | 예 | SKSE death event에서 1회 판정 |
| player-owned summon/proxy가 적대 대상을 처치 | 예 | `IsPlayerOwned(killer)`로 플레이어 측 처치에 포함 |
| 일반 상자/보스 상자 활성화 | 아니요 | 컨테이너 종류와 무관하게 판정 없음 |
| 시체 활성화/열기 | 아니요 | 사망 시 이미 처리하므로 열 때 재판정하지 않음 |
| 아이템 픽업/인벤토리 이동 | 아니요 | 통화 획득을 다시 트리거하지 않음 |
| 월드/바닥 생성 | 아니요 | `PlaceAtMe` 및 월드 폴백 없음 |
| 플레이어 인벤토리 직접 지급 | 아니요 | 성공 결과는 사망한 적의 인벤토리에만 추가 |
| 새 SPID Item/DeathItem/Perk 분배 | 아니요 | `CalamityAffixes_DISTR.ini`는 빈 호환 산출물 |
| LVLI 주입/오버라이드 | 아니요 | 기존 호환 레코드는 유지할 수 있으나 새 주입 경로는 없음 |

## 적격 판정

다음 조건을 모두 만족해야 룬 조각/재련 오브 판정을 수행한다.

1. 이벤트가 실제 사망 상태이고 대상이 죽어 있다.
2. 처치자가 플레이어 또는 player-owned summon/proxy로 귀속된다.
3. 대상과 플레이어 소유자 사이에 적대 관계가 확인된다.
4. 대상이 플레이어 소유, 플레이어 동료/팔로워, 소환체, 지휘 상태, 아동이 아니다.
5. 같은 시체의 해당 통화 카테고리가 이미 처리되지 않았다.

환경 오브젝트가 낸 사망과 player-owned가 아닌 독립 NPC/팔로워의 처치는 제외한다. 여기서 “플레이어 측 처치”는 플레이어 본인뿐 아니라 **player-owned summon/proxy**의 처치까지 포함하지만, 일반 팔로워나 우연히 전투에 참여한 독립 NPC의 처치는 포함하지 않는다.

## 실행 순서

1. death event의 killer reference가 actor이면 직접 사용하고, projectile이면 shooter/actor cause를 해석한다. Hazard·Explosion 등 일반 환경 reference는 처치자로 승격하지 않는다.
2. 플레이어 소유 귀속, 적대성, 피해자 제외 조건을 검증한다.
3. 시체의 기존 룬 조각/재련 오브와 구 SPID 레벨드 리스트 존재 여부를 카테고리별로 스냅샷한다.
4. 시체 ledger와 전환 스냅샷을 합쳐 룬 조각·재련 오브 각각의 판정 허용 여부를 정한다.
5. 허용된 카테고리만 확률 판정한다.
6. 성공한 아이템을 시체 인벤토리에 직접 추가하고 실제 수량 증가를 확인한다.
7. 시체 FormID·게임 날짜·처리 카테고리 mask와 피티 상태를 코세이브에 기록한다.

월드 생성, 플레이어 직접 지급 또는 일반 컨테이너 폴백은 어느 단계에도 없다.

## 확률과 룬 분포

- 룬워드 조각 기본 판정: `8%`
- 재련 오브 기본 판정: `5%`
- 룬 조각 피티: 99회 연속 실패 뒤 다음 적격 판정을 보장하는 기존 계약 유지
- 룬 가중치:
  - `El-Amn = 4`
  - `Sol-Um = 3`
  - `Mal-Lo = 2`
  - `Sur-Zod = 1`
- 최고/최저 가중치 비율: `4:1`

MCM에서 확률을 바꾸면 다음 적격 death event부터 적용된다. 일반 상자/픽업/월드에는 적용할 런타임 판정 자체가 없다.

## 피티와 시체 ledger 직렬화

새 코세이브 레코드 `CCRT v1`은 다음을 저장한다.

- 룬워드 조각 fail streak
- 재련 오브 fail streak
- 시체 FormID
- 처리한 게임 날짜 stamp
- 룬 조각/재련 오브 카테고리별 processed mask

로드 시 FormID를 현재 세이브에 맞게 해석하고, 유효한 ledger만 복구한다. 기존 코세이브 레코드의 타입·버전·내용은 변경하지 않고 `CCRT`를 추가하므로 구 세이브는 새 레코드가 없는 상태로 정상 로드된다. 새 게임은 필요하지 않다.

## 구 SPID 전환 호환

업데이트 전에 이미 SPID로 선분배된 액터/시체에는 실제 통화 아이템 또는 해당 통화 레벨드 리스트가 남아 있을 수 있다. 새 런타임은 이를 룬 조각과 재련 오브로 나눠 확인한다.

- 룬 조각 흔적이 있으면 룬 조각 런타임 롤만 생략한다.
- 재련 오브 흔적이 있으면 재련 오브 런타임 롤만 생략한다.
- 한 카테고리만 있으면 다른 카테고리는 정상 판정할 수 있다.
- 처리 결과는 ledger에 기록해 같은 세션과 저장/로드 뒤의 반복 death event를 막는다.

구 SPID 판정이 “무드롭”이었던 액터에는 남은 아이템/리스트 흔적이 없을 수 있으므로, 그런 전환 액터는 새 런타임에서 최초 1회 판정될 수 있다. 이는 기존 무드롭 결과를 사후 식별할 수 없는 한계이며, 실제 보유 통화가 있는 카테고리의 중복은 차단한다.

## SPID와 배포 산출물

- SPID는 현재 통화 드랍의 필수·권장 의존성이 아니다.
- `affixes/modules/keywords.spidRules.json`은 새 통화 분배 규칙을 제공하지 않는다.
- 생성된 `Data/CalamityAffixes_DISTR.ini`는 경로/업데이트 호환을 위한 빈 산출물이다.
- SPID가 설치되어 있어도 본 모드가 일반 상자, NPC 전체 또는 월드 픽업으로 통화 범위를 확장하지 않는다.

### MO2 업그레이드 주의

새 버전을 기존 버전과 별도 MO2 모드로 겹쳐 활성화하면, 모드 충돌 우선순위 또는 수동 병합 결과에 따라 구 `CalamityAffixes_DISTR.ini`가 새 빈 호환 파일보다 우선할 수 있다. 업그레이드할 때는 다음 중 하나를 사용한다.

1. 기존 Calamity 모드를 새 버전으로 교체/덮어쓴다.
2. 구 버전 모드를 비활성화한다.
3. 구 `CalamityAffixes_DISTR.ini`를 명시적으로 비활성화한다.

## 저장 및 콘텐츠 호환성

- 기존 장비와 인스턴스 어픽스 상태 유지
- 기존 보유 룬 조각/재련 오브 유지
- 완성 룬워드와 룬워드 재련 시 보존 규칙 유지
- 기존 룬워드/어픽스 ID와 기존 직렬화 레코드 유지
- 기존 룬 가중치 `4/3/2/1`, 기본 `8%/5%`, 99회 룬 피티 유지
- 새 `CCRT` 레코드만 추가

## English summary

`loot.currencyDropMode=hybrid` remains only as a legacy compatibility token. Actual currency authority is the SKSE death-event eligible-hostile-corpse-only path. A hostile victim killed by the player or a player-owned summon/proxy can receive a successful fragment/orb roll directly in its corpse inventory. Generic containers, corpse activation, pickup rolls, world spawning, direct player grants, and new SPID currency distribution are disabled.

Follower/teammate, summoned/commanded, child, player-owned, or non-hostile victims are excluded. Environmental-object kills and kills by independent non-player-owned NPCs/followers are also excluded. Defaults remain 8% fragments, 5% orbs, 4/3/2/1 rune weights, and the 99-failure fragment pity. `CCRT v1` persists pity counters and the per-corpse category ledger. Existing gear, currencies, completed runewords, and prior serialization records remain compatible.

When upgrading in MO2, replace/overwrite the existing mod or disable the old DISTR file. Enabling the new build as a separate layer can let the older `CalamityAffixes_DISTR.ini` override the new empty compatibility file when conflict priority is wrong.
