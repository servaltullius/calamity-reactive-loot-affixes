# Calamity — Reactive Loot & Affixes

Skyrim SE/AE SKSE64 플러그인. D2/D3 스타일 어픽스 시스템으로, 드롭 아이템에 1~4개의 Prefix(proc)/Suffix(패시브 스탯) 효과를 부여한다 (regular 3 + runeword 1).

## Quick Start

```bash
# 초기 셋업 (최초 1회)
python3 scripts/vibe.py configure --apply

# 상태 확인
python3 scripts/vibe.py doctor --full

# C++ 빌드 (Linux WSL → Windows DLL 크로스 컴파일)
cd skse/CalamityAffixes
cmake --build build.linux-clangcl-rel --config Release

# 테스트
cd skse/CalamityAffixes/build.linux-clangcl-rel
ctest --output-on-failure

# C# Generator 빌드 & 테스트
cd tools
dotnet build CalamityAffixes.Tools.sln
dotnet test CalamityAffixes.Generator.Tests
```

## 코드 규칙

- **탭 들여쓰기** (스페이스 아님). Edit 전에 반드시 `cat -A`로 실제 whitespace 확인
- 소스 파일 도메인 분할: `EventBridge.Loot.*.cpp`, `EventBridge.Triggers.*.cpp` 등
- C++23, clang-cl 크로스 컴파일 (타겟: Windows x86_64)
- MSVC `std::discrete_distribution`은 `uint8_t` 미지원 — `int` 이상 사용
- 리포 전체 포맷팅/무관한 리팩터링 금지

## 아키텍처

### 핵심 데이터 구조

```
_instanceAffixes: unordered_map<uint64_t, InstanceAffixSlots>
                                 ^key              ^tokens[4] + count
                          (FormID<<16|UniqueID)    (FNV-1a 64bit)
```

- `InstanceAffixSlots` (max 4 토큰/아이템: regular 3 + runeword 1) — 유일한 진실의 원천
- `_instanceSupplementalAffixes` — v1.1.0에서 제거됨 (InstanceAffixSlots로 통합)

### Prefix/Suffix 시스템 (v1.2.0)

| 구분 | Prefix | Suffix | Runeword |
|------|--------|--------|----------|
| 개수 | 83개 | 60개 (20 패밀리 × 3 티어) | 94개 (룬워드 결과) |
| 효과 | proc (발동형) | 패시브 스탯 (Ability Spell) | proc (발동형) |
| 예시 | Firestorm, Thunderbolt | Max Health +50, Fire Resist +10% | Enigma, Grief, Spirit |

**롤링 규칙**:
- 1 어픽스 (70%): P 또는 S 50/50
- 2 어픽스 (22%): 1P + 1S 고정
- 3 어픽스 (8%): 2P+1S 또는 1P+2S 랜덤

**Suffix 티어**: Minor(60%), Major(30%), Grand(10%)

**아이템 네이밍**: `★ 철검`, `★★ 철검`, `★★★ 철검` (어픽스 라벨 없음, 별 개수 = 어픽스 수)

**Proc 패널티** ("Best Slot Wins"): 1어픽스=1.0, 2어픽스=0.8, 3어픽스=0.65, 4어픽스=0.5

### 직렬화

- **현재 버전**: v7 (co-save IAXF 레코드)
- v1~v6 자동 마이그레이션 지원
- 포맷: `[entryCount] per entry: [baseID(u32) uniqueID(u16) count(u8) tokens[4](u64)]`

### 룬워드

- `ReplaceAll()` — 기존 슬롯 전체 교체 (D2 스타일)
- 94개 레시피 (`RunewordCatalogRows.inl`) + 94개 결과 어픽스 (`runeword_*_final`)
- 결과 어픽스 정의: `affixes/modules/keywords.affixes.runewords.json`
- 재련(Reforge) 시 룬워드 보존 — 일반 어픽스만 재롤, 룬워드 슬롯 유지
- 룬워드 재변환 허용 — 기존 룬워드를 제거하고 새 룬워드로 교체 가능

## 주요 파일

### C++ 플러그인 (`skse/CalamityAffixes/`)

| 파일 | 역할 |
|------|------|
| `include/CalamityAffixes/EventBridge.h` | 메인 클래스, 상수, 타입 정의 |
| `include/CalamityAffixes/InstanceAffixSlots.h` | 다중 어픽스 구조체 (핵심) |
| `src/EventBridge.Config.*.cpp` | affixes.json 파싱 (AffixParsing/AffixTriggerParsing/AffixActionParsing 등으로 분할) |
| `src/EventBridge.Config.Shared.h` | JSON 유틸 (Trim, StripNullValues, LookupSpellFromSpec) |
| `src/EventBridge.Loot.Assign.cpp` | 롤링, P/S 분배, 네이밍 |
| `src/EventBridge.Triggers.*.cpp` | 트리거 이벤트 (HitEvent/DeathEvent/EquipEvent 등으로 분할) |
| `src/EventBridge.Triggers.ActiveCounts.cpp` | RebuildActiveCounts, 패시브 스펠 관리 |
| `src/EventBridge.Serialization.cpp` | Save/Load/Revert (v7) |
| `src/EventBridge.Loot.Runtime.cpp` | 런타임 뮤테이션 |
| `src/EventBridge.Loot.TooltipResolution.cpp` | 인벤토리 툴팁 생성 |
| `src/EventBridge.Loot.Runeword.*.cpp` | 룬워드 (Transmute/Reforge/BaseSelection/Policy 등으로 분할) |

### C# Generator (`tools/CalamityAffixes.Generator/`) — xEdit/CK 불필요

**Mutagen 라이브러리 기반**으로 ESP를 코드에서 직접 생성한다. Creation Kit이나 xEdit 없이 CLI만으로 게임 레코드 생성 가능.

```bash
# ESP + INI + JSON 전체 재생성 (반드시 리포 루트에서 실행)
dotnet run --project tools/CalamityAffixes.Generator
# → Data/CalamityAffixes.esp (Keyword, MagicEffect, Spell, MISC 레코드)
# → Data/CalamityAffixes_KID.ini, CalamityAffixes_DISTR.ini
# → Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json
```

**⚠️ ESP FormID 안정성**: Mutagen은 `AddNew()` 호출 순서대로 FormID를 할당한다. 세이브 파일은 인벤토리 아이템을 FormID로 참조하므로, **MISC 아이템(오브, 룬 조각)은 반드시 MGEF/SPEL 앞에 생성**해야 한다. 현재 `KeywordPluginBuilder.Build()` 순서:
1. MCM Quest (고정)
2. **MISC 아이템** — 오브, 룬 조각 (고정, 어픽스 추가 영향 없음)
3. Keywords
4. MagicEffects / Spells (가변 — 어픽스 추가 시 증가)
5. LeveledItems

> 이 순서를 변경하면 기존 세이브의 인벤토리 아이템이 소실된다 (v1.2.19-rc34 사고 교훈).

**ESP 레코드 추가 방법**: `affixes.json`의 각 어픽스에 `records` 필드 작성 → Generator가 파싱하여 ESP에 반영.

```jsonc
// affixes.json 예시 — Suffix용 Ability Spell
{
  "id": "suffix_max_health_t2",
  "records": {
    "magicEffect": {
      "editorId": "CA_MGEF_SuffixMaxHealthT2",
      "name": "Calamity Suffix: Max Health (Major)",
      "actorValue": "Health",
      "magnitude": 50.0
    },
    "spell": {
      "editorId": "CA_SPEL_SuffixMaxHealthT2",
      "name": "Calamity Suffix: Max Health (Major)",
      "spellType": "Ability",        // Ability = 패시브 상시 적용
      "castType": "ConstantEffect",  // ConstantEffect = 장비 중 유지
      "magicEffectId": "CA_MGEF_SuffixMaxHealthT2"
    }
  }
}
```

**지원 SpellType**: `Spell` (기본, FireAndForget), `Ability` (패시브, ConstantEffect)

| 파일 | 역할 |
|------|------|
| `Spec/AffixSpec.cs` | 어픽스 오브젝트 모델 (Slot, Family, SpellType 등) |
| `Writers/KeywordPluginBuilder.cs` | ESP 빌드 (MISC, Keyword, MagicEffect, Spell) — **생성 순서 중요** |
| `Writers/McmPluginBuilder.cs` | MCM Helper Quest 생성 |
| `Writers/KidIniWriter.cs` | KID 배포 INI |

### 데이터 파일

| 파일 | 역할 |
|------|------|
| `Data/SKSE/Plugins/CalamityAffixes/affixes.json` | 런타임 어픽스 정의 (6057 LOC) |
| `affixes/affixes.json` | 소스 어픽스 카탈로그 (Generator 입력) |
| `Data/CalamityAffixes.esp` | 게임 플러그인 (Keywords, MagicEffects, Spells) |

## 릴리즈 프로세스

- **릴리즈 노트/패치노트는 반드시 한국어로 작성**
- DLL 바이너리를 MO2 zip에 포함했는지 반드시 확인 후 릴리즈
- **프리릴리즈/정식릴리즈 모두 MO2 zip을 첨부해야 함** — 릴리즈 노트만 올리면 사용자가 적용 불가

### 프리릴리즈 (rc)

```bash
# 1. 빌드 & 테스트
cd skse/CalamityAffixes
cmake --build build.linux-clangcl-rel --config Release
cd build.linux-clangcl-rel && ctest --output-on-failure

# 2. 커밋 & 푸시
git add <files> && git commit && git push

# 3. MO2 zip 생성 (release_build.py 또는 수동)
python3 tools/release_build.py          # 자동: build→test→zip→verify→SHA256
# 또는 수동: DLL 복사 후 zip 패키징

# 4. 프리릴리즈 생성 + zip 첨부 (반드시 zip 첨부!)
gh release create vX.Y.Z-rcNN --prerelease --title "..." --notes "..."
gh release upload vX.Y.Z-rcNN CalamityAffixes_MO2_vX.Y.Z.zip
```

### 정식 릴리즈

```bash
# 1. 빌드 & 테스트 확인
cd skse/CalamityAffixes
cmake --build build.linux-clangcl-rel --config Release
cd build.linux-clangcl-rel && ctest --output-on-failure

# 2. Generator 테스트
cd tools && dotnet test CalamityAffixes.Generator.Tests

# 3. 커밋
git add <files> && git commit

# 4. MO2 zip 패키징 (아래 구조)
# CalamityAffixes/
#   CalamityAffixes.esp
#   CalamityAffixes_DISTR.ini
#   CalamityAffixes_KID.ini
#   SKSE/Plugins/CalamityAffixes.dll
#   SKSE/Plugins/CalamityAffixes/affixes.json
#   SKSE/Plugins/InventoryInjector/CalamityAffixes.json
#   MCM/Config/CalamityAffixes/{settings.ini, config.json, keybinds.json}
#   Scripts/*.pex + Source/*.psc
#   PrismaUI/views/CalamityAffixes/index.html

# 5. GitHub 릴리즈 + zip 첨부
gh release create vX.Y.Z --title "..." --notes "..."
gh release upload vX.Y.Z CalamityAffixes_MO2_vX.Y.Z.zip
```

## 주의사항 / 함정

- **빌드 후 릴리즈**: DLL 바이너리를 MO2 zip에 반드시 포함해야 모드 작동
- **Suffix 동일 스펠 비중첩**: 같은 Suffix가 여러 장비에 있어도 1회만 적용 (RE::SpellItem* 중복 방지). 다른 티어는 다른 스펠이므로 중첩 가능
- **`procChancePct` 이중 의미**: Prefix=proc 확률, Suffix=드롭 가중치 (리팩터링 후보)
- **Edit 전 whitespace**: 반드시 `cat -A`로 탭/스페이스 확인 후 편집
- **ESP 재생성**: affixes.json `records` 필드 수정 시 Generator 재실행 필요 (리포 루트에서 실행)
- **ESP FormID 순서 금지 변경**: `KeywordPluginBuilder.Build()`에서 MISC 아이템 생성 순서를 MGEF/SPEL 뒤로 옮기면 세이브 파일의 인벤토리 아이템 소실. 절대 변경 금지
- **JSON null 값**: `nlohmann::json::value(key, default)`는 key가 null이면 default 반환이 아니라 `type_error::302` 발생. `StripNullValues`로 사전 제거 필요
- **PromoteTokenToPrimary count==0**: count가 0인 아이템에 토큰 추가 시 existingIndex==0으로 오판하는 버그 수정됨 (rc39). 관련 수정 시 count>0 조건 유지 필수

## 버전 히스토리

| 버전 | 주요 변경 |
|------|----------|
| v1.0.0~v1.0.3 | 기본 단일 어픽스 시스템 |
| v1.1.0 | 다중 어픽스 (1~3개, 70/22/8%) |
| v1.1.1 | 아이템 이름 줄바꿈 버그 수정 |
| v1.2.0 | Prefix/Suffix 분류, 60개 패시브 Suffix 추가 |
| v1.2.19-rc33 | 재련 롤링 균등 분포 (lootWeight 1.0 통일) |
| v1.2.19-rc34 | 78개 룬워드 결과 어픽스 추가 (94개 전체 구현) |
| v1.2.19-rc35 | Generator MISC FormID 안정화 (생성 순서 수정) |
| v1.2.19-rc38 | JSON null 파싱 크래시 수정 (StripNullValues), MISC 자동복구 |
| v1.2.19-rc39 | PromoteTokenToPrimary count==0 룬워드 적용 실패 수정 |
