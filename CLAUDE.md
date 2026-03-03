# Calamity — Reactive Loot & Affixes

Skyrim SE/AE SKSE64 플러그인. D2/D3 스타일 어픽스 시스템 (Prefix proc + Suffix 패시브 + Runeword).

## Quick Start

```bash
# C++ 빌드 & 테스트
cd skse/CalamityAffixes
cmake --build build.linux-clangcl-rel --config Release
cd build.linux-clangcl-rel && ctest --output-on-failure

# C# Generator 빌드 & 테스트
cd tools && dotnet test CalamityAffixes.Generator.Tests
```

## 코드 규칙

- **탭 들여쓰기** (스페이스 아님). Edit 전에 반드시 `cat -A`로 실제 whitespace 확인
- 소스 파일 도메인 분할: `EventBridge.Loot.*.cpp`, `EventBridge.Triggers.*.cpp` 등
- C++23, clang-cl 크로스 컴파일 (타겟: Windows x86_64)
- MSVC `std::discrete_distribution`은 `uint8_t` 미지원 — `int` 이상 사용
- 리포 전체 포맷팅/무관한 리팩터링 금지

## 아키텍처 요약

- `InstanceAffixSlots` (max 4 토큰/아이템) — 유일한 진실의 원천
- 237 어픽스: Prefix 83 + Runeword 94 + Suffix 60
- 직렬화 v7 (co-save IAXF), v1~v6 자동 마이그레이션
- Proc 패널티 "Best Slot Wins": 1=1.0, 2=0.8, 3=0.65, 4=0.5

## Generator 규칙

- ESP FormID 순서 **절대 변경 금지**: MCM → MISC → Keywords → MGEF/SPEL → LeveledItems
- MISC를 MGEF/SPEL 뒤로 옮기면 세이브 인벤토리 아이템 소실 (rc34 사고 교훈)
- Generator는 반드시 **리포 루트**에서 실행: `dotnet run --project tools/CalamityAffixes.Generator`
- `affixes.json` records 필드 수정 시 Generator 재실행 필수

## 릴리즈 규칙

- 릴리즈 노트/패치노트는 **반드시 한국어**로 작성
- 프리릴리즈/정식릴리즈 모두 **MO2 zip 첨부 필수** — 노트만 올리면 사용자 적용 불가
- DLL 바이너리를 MO2 zip에 반드시 포함해야 모드 작동
- 자동 파이프라인: `python3 tools/release_build.py` (build → test → zip → verify → SHA256)
- 프리릴리즈: `gh release create vX.Y.Z-rcNN --prerelease` + zip 첨부
- 정식릴리즈: `gh release create vX.Y.Z` + zip 첨부
- ZIP 검증: `python3 tools/verify_mo2_zip.py <zip>`

## 주의사항

- **JSON null**: `json::value(key, default)`는 key=null 시 `type_error::302` — `StripNullValues` 사전 제거 필수
- **count==0 가드**: `PromoteTokenToPrimary` count==0 시 existingIndex 오판 — `count > 0u` 조건 유지 필수
- **vanilla hostile spell 금지**: `applyTo: "Self"`에 적대 스펠 사용 시 전투 상태 전환됨
- **Suffix 동일 스펠 비중첩**: 같은 Suffix는 여러 장비에도 1회만 적용 (다른 티어는 중첩 가능)
- **`procChancePct` 이중 의미**: Prefix = proc 확률, Suffix = 드롭 가중치
- **CoC에서 ResolveHitWeapon 폴백 금지**: `EvaluateCastOnCrit`는 `hitData->weapon` 직접 확인만 사용. 폴백이 장착 활을 반환하면 주문 투사체가 무한 체인 발동
- **passiveSpellEditorId 전 슬롯 지원**: prefix/suffix/runeword 모두 사용 가능 (v1.2.19.2~)
