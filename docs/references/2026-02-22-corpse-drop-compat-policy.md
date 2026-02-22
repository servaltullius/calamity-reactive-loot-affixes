# Corpse Loot Compatibility Policy (2026-02-22)

## 목적

이 문서는 “시체 루팅 UX 유지 + 타 모드 드랍 공존”을 만족하는 드랍 아키텍처를 고정하기 위한 운영 기준입니다.  
핵심은 **시체 드랍은 루팅(시체 인벤토리)으로 보이되**, 다른 모드의 드랍을 막지 않는 것입니다.

## 요구사항(고정)

- 플레이어 체감은 반드시 **시체 루팅 기반 획득**이어야 함
- 타 모드(예: 모험가 길드/HDA)의 시체 드랍과 **공존**해야 함
- 하이브리드 정책에서:
  - 시체는 SPID 기반
  - 상자/월드는 런타임 기반
- 바닥 월드스폰(플레이어 발밑 스폰, 훔치기 판정 유발)은 금지

## Skyrim 드랍 구현 방식 비교

| 방식 | 구현 난이도 | 호환성 | 루팅 UX | 관찰된 리스크 | 권장도 |
|---|---|---|---|---|---|
| NPC `Death Item(DEAT/INAM)` 직접 편집 | 중 | 낮음(필드 충돌) | 높음 | 다른 모드와 레코드 충돌 쉬움 | 낮음 |
| `DeathItem_DISTR.ini` (SPID DeathItem) | 낮음 | 중 | 높음 | On-Death 분배 이슈/버전 민감 사례 존재 | 중 |
| `Perk_DISTR.ini` + `AddLeveledListOnDeath` | 중 | 높음 | 높음 | 설정 복잡도 증가 | **높음(채택)** |
| Papyrus `OnDeath` + `AddItem` | 중~높음 | 중 | 낮음(즉시지급으로 오해 가능) | 스크립트 의존/디버깅 비용 증가 | 중~낮음 |

## 최종 채택안 (고정)

### 1) 시체 드랍 경로

- SPID는 **DeathItem 직접 분배를 사용하지 않고**, 대상 액터에 **Perk를 분배**한다.
- 해당 Perk는 EntryPoint `AddLeveledListOnDeath`로 LVLI를 시체 인벤토리에 주입한다.
- 현재 생성 대상 Perk:
  - `CAFF_Perk_DeathDropRunewordFragment`
  - `CAFF_Perk_DeathDropReforgeOrb`

### 2) 상자/월드 경로

- `loot.currencyDropMode=hybrid`에서 상자/월드는 SKSE 런타임 롤만 사용한다.
- 시체 경로와 상자/월드 경로를 분리해, 회귀 원인을 빠르게 좁힐 수 있게 유지한다.

## 구현 위치(변경 시 필수 확인)

- SPID 규칙 소스: `affixes/modules/keywords.spidRules.json`
- SPID 생성물: `Data/CalamityAffixes_DISTR.ini`
- Perk/LVLI 생성 코드: `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`
- 정책 문서 링크(README): `README.md`의 “드랍 경로 정책” 섹션

## 안티-회귀 체크리스트

드랍 경로를 수정한 PR/릴리즈 전, 아래를 반드시 점검합니다.

1. `_DISTR.ini`에 `DeathItem = ...` 라인이 재유입되지 않았는가
2. ESP에 아래 체인이 유지되는가
   - `CAFF_Perk_DeathDropRunewordFragment` -> `AddLeveledListOnDeath` -> `CAFF_LItem_RunewordFragmentDrops`
   - `CAFF_Perk_DeathDropReforgeOrb` -> `AddLeveledListOnDeath` -> `CAFF_LItem_ReforgeOrbDrops`
3. 실플레이 루팅에서 타 모드 드랍 회귀가 없는가
4. 바닥 월드스폰 경로가 재활성화되지 않았는가

## 운영 메모

- MCM 슬라이더(`loot.runewordFragmentChancePercent`, `loot.reforgeOrbChancePercent`)는
  하이브리드 정책에서도 동일한 드랍 체계에 반영된다.
- 시체 루팅 결과가 “0개”일 때는 먼저 **분배(Perk 배포)**와 **사망 실행(EntryPoint 실행)**을 분리해 진단한다.

## 참고 근거(외부)

- SPID 최신 릴리스(7.2.0 계열):  
  https://api.github.com/repos/powerof3/Spell-Perk-Item-Distributor/releases/latest
- SPID On-Death 분배 이슈 사례 및 해결 코멘트:  
  https://api.github.com/repos/powerof3/Spell-Perk-Item-Distributor/issues/80  
  https://api.github.com/repos/powerof3/Spell-Perk-Item-Distributor/issues/80/comments
- UESP 레코드 포맷 참고(NPC/PERK/LVLI):  
  https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format/NPC  
  https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format/PERK  
  https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format/LVLI
