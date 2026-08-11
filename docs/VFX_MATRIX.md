# VFX/SFX Matrix / 시각·음향 피드백 매트릭스

This document lists the always-on, data-driven P0–P1 feedback added without changing proc chance, ICD, damage, target selection, budgets, or serialization.

이 문서는 발동률, ICD, 피해, 대상 선정, 예산, 직렬화를 변경하지 않고 추가된 상시 활성 데이터 주도형 P0–P1 피드백을 정리합니다.

## Traps / 함정

| Effects / 효과 | Persistent marker / 지속 마커 | Trigger burst / 발동 폭발 | Positional sounds / 위치 음향 |
|---|---|---|---|
| `bear_trap` | Scriptless world reference using animated vanilla `BearTrap01` metal jaws | Frost burst | Explicit placement cue; close/rearm sounds from the behavior graph |
| `rune_trap` | Scriptless world reference using the vanilla stone pressure plate (`TrapStonePressurePlate01.nif`) | Frost burst | Magic place/rune trigger |
| `plague_spore` | Scriptless world reference using the vanilla poison-spider sack (`spidersackdead.nif`) | Gas blast | Magic place/poison trigger |
| `tar_blight` | Scriptless world reference using the vanilla oil puddle (`OilTrapPuddle01.nif`) | Gas pulse | Magic place/poison trigger |
| `siphon_spore` | Scriptless world reference using vanilla albino exploding-spider eggs (`ExpSpiderEggsAlbino.nif`) | Absorb hit | Magic place/absorb trigger |
| `chaos_rune` | Scriptless world reference using the vanilla metal pressure plate (`TrapPressurePlateMetal01.nif`) | Shock burst | Magic place/shock trigger |

Markers show location and armed state; they are not exact radius telegraphs. All six persistent markers are scriptless, non-persistent, collision- and activation-blocked Calamity-owned `MSTT` world references with no `VMAD`. The five non-bear traps keep their existing short armed, triggered, and natural-expiration particle/sound cues as overlays; those cues no longer serve as the persistent marker. Their spell effects use `Effect.Area=0`, and the runtime directly casts only on up to two hostile actors inside the configured trap radius so a spell-area splash cannot reach companions. The bear contract remains distinct: `CAFF_MSTT_TRAP_BEAR_VISUAL` sends `StartOpen` after placement, `Trigger01` when the logical trap fires, and `Reset01` when it rearms. A 900 ms open gate follows every accepted open event, and only its placement sound remains explicit because the behavior graph supplies the close and reset sounds. A pre-save hook cancels active traps and retires their world references before the engine save starts. Cap eviction, invalidation, reload, disable, save, load, and revert cleanup are silent.

마커는 위치와 무장 상태를 나타내며 정확한 판정 반경 원이 아닙니다. 6종 모두 `VMAD`가 없는 Calamity 소유 `MSTT`를 스크립트 없는 비영속·충돌 차단·활성화 차단 월드 레퍼런스로 배치합니다. 곰덫 외 5종은 기존의 짧은 무장·발동·자연 만료 파티클/음향 큐를 상태 오버레이로 유지하지만, 더 이상 그 임시 이펙트를 지속 마커로 사용하지 않습니다. 주문 효과는 `Effect.Area=0`이며 런타임이 설정 반경 안의 적대 액터만 직접 골라 최대 2명에게 시전하므로 주문 자체의 범위 splash가 동료에게 번지지 않습니다. 곰덫 계약은 별도로 `CAFF_MSTT_TRAP_BEAR_VISUAL`에 설치 후 `StartOpen`, 논리 덫 발동 시 `Trigger01`, 재무장 시 `Reset01`을 보내고 수락된 열림 이벤트마다 900ms open gate를 적용합니다. 닫힘·재장전 음향은 behavior graph가 재생하므로 명시적 사운드는 설치음만 유지합니다. 엔진 저장이 시작되기 전 활성 덫과 월드 레퍼런스를 취소하며, cap 축출·잘못된 상태·설정 재적용·비활성화·저장·로드·Revert 정리는 조용히 수행합니다.

The markers reuse vanilla models by path only. Their original Skyrim activators, scripts, damage logic, and collision behavior are not spawned, and the vanilla NIFs are not copied into the mod package.

마커는 바닐라 모델 경로만 재사용합니다. Skyrim 원본 액티베이터·스크립트·피해 로직·충돌 동작은 소환하지 않으며, 바닐라 NIF도 모드 패키지에 복사하지 않습니다.

## Corpse explosions / 시체 폭발

| Palette / 계열 | Effects / 효과 | Art / 시각 | Duration / 지속 |
|---|---|---|---:|
| Fire / 화염 | `ember_pyre`, `death_pyre_t1`, `death_pyre_t2`, `death_pyre_t3`, `conjured_pyre_t1`, `conjured_pyre_t2` | Fireball explosion | 0.55 s |
| Plague / 역병 | `runeword_breath_of_the_dying_final`, `runeword_plague_final` | Poison gas blast | 0.70 s |

The custom effect and positional sound play once at the corpse per accepted explosion, independent of the number of targets hit. The legacy common shader remains a visual fallback when the custom ArtObject cannot be resolved.

custom 효과와 위치 음향은 피격 대상 수와 무관하게 승인된 폭발마다 시체 위치에서 한 번 재생됩니다. custom ArtObject를 해석하지 못하면 기존 공통 셰이더를 시각 fallback으로 유지합니다.

## Defensive and emergency procs / 방어·긴급 발동

| Palette / 계열 | Effects / 효과 | Duration / 지속 |
|---|---|---:|
| Emergency heal / 긴급 회복 | `runeword_lionheart_final`, `runeword_last_wish_final` | 0.55 s |
| Phase / 은신·위상 | `runeword_metamorphosis_final`, `shadow_veil`, `runeword_enigma_final`, `runeword_stealth_final`, `runeword_chains_of_honor_final` | 0.45 s |
| Physical bulwark / 물리 방벽 | `runeword_exile_final`, `runeword_fortitude_final`, `stone_ward`, `runeword_bulwark_final` | 0.55 s |
| Magic ward / 마법·원소 방벽 | `arcane_ward`, `runeword_rhyme_final`, `runeword_principle_final`, `runeword_splendor_final` | 0.55 s |
| Reflect / 반사·가시 | `runeword_eternity_final`, `runeword_bramble_final` | 0.60 s |
| Radiance / 광휘 | `runeword_radiance_final` | 0.75 s |
| Dragon scale / 용의 비늘 | `runeword_dragon_final` | 0.70 s |

These effects attach once to the owner on a successful proc and use a one-shot positional sound. They do not refresh or alter the duration of the gameplay spell.

이 효과들은 발동 성공 시 소유자에게 한 번 부착되고 위치 기반 일회성 사운드를 재생합니다. 게임플레이 주문의 지속시간을 갱신하거나 변경하지 않습니다.

## Validation status / 검증 상태

Automated validation covers data shape, animation-event completeness and marker authority, append-only allocation, generated ESP round-trip, zero-area hostile-direct targeting, forbidden vanilla-asset bundling, lint, runtime gates, and SKSE compilation. Bear animation has been observed in game, but the five new physical marker models remain unverified in game. Each still requires visual confirmation, ground placement and orientation checks, blocked collision/activation, consumed/expired/save-load cleanup, companion immunity, sound direction, and density testing.

자동 검증은 데이터 형식, 애니메이션 이벤트 완전성·월드 마커 권한, append-only 할당, 생성 ESP round-trip, `Area=0` 적대 대상 직접 시전, 바닐라 자산 번들 금지, lint, 런타임 gate, SKSE 컴파일을 다룹니다. 곰덫 애니메이션은 인게임에서 확인됐지만 새 물리 마커 5종은 아직 인게임 미검증입니다. 각 모델의 실제 가시성·지면 배치와 방향, 충돌·활성화 차단, 발동 소모·만료·저장/로드 정리, 동료 무영향, 사운드 방향, 밀도 검증이 별도로 필요합니다.
