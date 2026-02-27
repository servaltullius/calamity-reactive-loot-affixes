## Calamity Reactive Loot & Affixes v1.2.19-rc13 (Pre-release)

이번 RC는 활/원거리 전투에서 간헐적으로 어픽스가 미발동하던 경로를 줄이는
런타임 이벤트 해석 보강에 초점을 맞춘 안정화 업데이트입니다.

---

### 변경 사항

- `TESHitEvent`에서 `cause`가 Actor가 아닐 때(Projectile/Hazard 등)도
  shooter/actorCause를 통해 공격자를 복원하도록 보강했습니다.
- `TESDeathEvent`의 killer 복원 경로를 공통 함수로 정리해
  원거리/마법 킬 트리거 해석 일관성을 높였습니다.
- `HitData->weapon`가 비어 있는 케이스에서도 공격 무기를 추론할 수 있도록
  `HitDataUtil::ResolveHitWeapon`/`IsWeaponLikeHit` 공통 유틸을 추가했습니다.
- `ConvertDamage`, `CastOnCrit`, `SpawnTrap(trapRequireWeaponHit)`가
  공통 무기 판정 유틸을 사용하도록 바꿔 활 히트 누락 가능성을 줄였습니다.

### 검증

- `CAFF_PACKAGE_VERSION=1.2.19-rc13 tools/release_verify.sh --fast --strict --with-mo2-zip` 통과
  - compose/lint/json sanity
  - Generator 테스트 71/71
  - SKSE ctest 2/2
  - MO2 패키징 생성 완료

### 배포 파일

- `CalamityAffixes_MO2_v1.2.19-rc13_2026-02-27.zip`
- SHA256: `530c75095031e5bcbb08e9d5a3b7f67f462408dc3c600089221e6b00cca83601`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.19-rc12...v1.2.19-rc13
