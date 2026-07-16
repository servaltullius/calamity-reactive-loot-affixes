# VFX/SFX Matrix / 시각·음향 피드백 매트릭스

This document lists the always-on, data-driven P0–P1 feedback added without changing proc chance, ICD, damage, target selection, budgets, or serialization.

이 문서는 발동률, ICD, 피해, 대상 선정, 예산, 직렬화를 변경하지 않고 추가된 상시 활성 데이터 주도형 P0–P1 피드백을 정리합니다.

## Traps / 함정

| Effects / 효과 | Persistent marker / 지속 마커 | Trigger burst / 발동 폭발 | Positional sounds / 위치 음향 |
|---|---|---|---|
| `bear_trap` | Bear trap model | Frost burst | Bear trap place/trigger |
| `rune_trap` | Frost rune | Frost burst | Magic place/rune trigger |
| `plague_spore` | Dark poison gas | Gas blast | Magic place/poison trigger |
| `tar_blight` | Oil puddle | Gas pulse | Magic place/poison trigger |
| `siphon_spore` | Soul trap point | Absorb hit | Magic place/absorb trigger |
| `chaos_rune` | Lightning rune | Shock burst | Magic place/shock trigger |

Markers show location and armed state; they are not exact radius telegraphs. Placement and trigger use one-shot positional sound. Arming and natural expiration use visual pulses only. Cap eviction, invalidation, reload, disable, load, and revert cleanup are silent.

마커는 위치와 무장 상태를 나타내며 정확한 판정 반경 원이 아닙니다. 설치와 발동에는 위치 기반 일회성 사운드를 사용하고, 무장과 자연 만료에는 시각 pulse만 사용합니다. cap 축출, 잘못된 상태, 설정 재적용, 비활성화, 로드, Revert 정리는 조용히 수행합니다.

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

Automated validation covers data shape, append-only allocation, generated ESP round-trip, lint, runtime gates, and SKSE compilation. In-game validation remains required for fixed world position, save/load ghost cleanup, vanilla NIF ground readability, sound direction, and the 48-marker density case.

자동 검증은 데이터 형식, append-only 할당, 생성 ESP round-trip, lint, 런타임 gate, SKSE 컴파일을 다룹니다. 고정 세계 좌표, 저장/로드 ghost 정리, 바닐라 NIF의 지면 가독성, 사운드 방향, 48개 마커 밀도는 별도 인게임 검증이 필요합니다.
