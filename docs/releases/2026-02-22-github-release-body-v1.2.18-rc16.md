## Calamity Reactive Loot & Affixes v1.2.18-rc16 (Pre-release)

이번 RC는 드로거 같은 언데드 대상 전투에서 **CoC(Thunderbolt 등) 같은 Hit 기반 발동이 전투 중에도 막히던 케이스**를 수정했습니다.

### 핵심 변경
- **언데드 전투 시 “적대” 판정 폴백 추가**
  - 일부 언데드가 전투 중에도 `IsHostileToActor` 판정이 false로 남아,
    CoC/ConvertDamage 등 Hit 기반 프로크가 차단되던 케이스를 완화했습니다.
  - 두 액터가 모두 전투 중이고 서로를 전투 타겟으로 잡은 경우,
    `(언데드 || CombatantFaction)` 캐시 플래그가 참이면 적대로 간주합니다.

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc16 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (53/53)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc16_2026-02-22.zip`
