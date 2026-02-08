# CalamityAffixes 릴리즈 노트 (2026-02-09)

## 선행 의존성

- 필수
  - SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
  - Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
  - Prisma UI (툴팁/조작 패널 프레임워크): https://www.nexusmods.com/skyrimspecialedition/mods/148718
- 권장
  - SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
  - Keyword Item Distributor (KID): https://www.nexusmods.com/skyrimspecialedition/mods/55728
  - Spell Perk Item Distributor (SPID): https://www.nexusmods.com/skyrimspecialedition/mods/36869
- 선택
  - MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
  - Inventory Interface Information Injector (I4): https://www.nexusmods.com/skyrimspecialedition/mods/85702

## 변경 요약

- 런타임 상태 키를 인스턴스 단일 키에서 복합 키 `(instanceKey, affixToken)`로 리팩터링해, 어픽스 간 상태 섞임을 방지했습니다.
- 직렬화 v5를 추가하고 per-affix 런타임 상태 레코드(`IRST`)를 명시적으로 저장하도록 변경했습니다.
- 기존 세이브(v1~v4) 로드 호환(마이그레이션) 경로를 유지했습니다.
- 드랍 아이템 인스턴스 삭제 시 해당 인스턴스의 모든 런타임 상태가 함께 정리되도록 보강했습니다.
- 배포 산출물(`CalamityAffixes.esp`, `*_DISTR.ini`, `*_KID.ini`, Papyrus `.pex`, `CalamityAffixes.dll`)을 재생성하고 MO2 ZIP을 재빌드했습니다.

## 플레이어 영향

- 룬워드 결과 어픽스와 inherited/supplemental 어픽스가 공존할 때도 진화/모드 상태가 서로 간섭하지 않습니다.
- 선택한 아이템 인스턴스의 어픽스 토큰 기준으로 툴팁 런타임 정보(진화/모드)가 정확히 표시됩니다.

## 기술 메모

- `unordered_map` 런타임 상태 저장을 위해 `InstanceStateKey` + 해시 유틸을 추가했습니다.
- 런타임 상태 맵 키는 raw `instanceKey`가 아니라 `InstanceStateKey`로 변경되었습니다.
- 저장/로드는 별도 인스턴스 런타임 상태 레코드를 사용하며, 구버전 세이브를 안전하게 이관합니다.

## 검증

- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`
- `tools/build_mo2_zip.sh`
- 생성 산출물: `dist/CalamityAffixes_MO2_2026-02-09.zip`
