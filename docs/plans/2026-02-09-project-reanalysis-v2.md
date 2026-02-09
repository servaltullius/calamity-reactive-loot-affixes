# Project Reanalysis v2 (2026-02-09)

## 0) TL;DR

- 현재 프로젝트는 **플레이어 중심 런타임**(인벤/장비 + 플레이어 소유 소환체)으로 의도된 범위를 안정적으로 유지하고 있습니다.
- 검증 기준으로는 통과 상태입니다:
  - Generator tests: 21/21 pass
  - `lint_affixes.py`: warning 0
  - CMake ctest: `CalamityAffixesStaticChecks` pass
- 다음 병목은 기능 결함보다 **유지보수성/회귀 방지 체계**입니다.
  - `EventBridge.Loot.cpp` 단일 파일 2,500+ LOC
  - SKSE 쪽 테스트가 compile-time static checks 위주

---

## 1) 이번 재분석 범위

1. 런타임 아키텍처/책임 분리 상태
2. 저장/마이그레이션 안정성
3. 테스트/검증 체계
4. 빌드·배포·문서 동기화 상태
5. 단기(1~2주) 개선 우선순위

---

## 2) 검증 스냅샷

실행 커맨드:

```bash
dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release
python3 tools/lint_affixes.py --spec affixes/affixes.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure
```

결과:
- Generator 테스트 통과: 21/21
- lint 통과: warning 0
- CTest 통과: 1/1 (`CalamityAffixesStaticChecks`)

---

## 3) 현재 구조 평가

### 3.1 런타임 파이프라인

- 진입점/등록:
  - `skse/CalamityAffixes/src/main.cpp`에서 `kDataLoaded` 시점에 `LoadConfig -> Register -> Hooks::Install -> PrismaTooltip::Install -> TrapSystem::Install`
- 이벤트 싱크:
  - `TESHitEvent`, `TESDeathEvent`, `TESEquipEvent`, `TESMagicEffectApplyEvent`, `TESContainerChangedEvent`
  - 등록 위치: `skse/CalamityAffixes/src/EventBridge.cpp`
- 저장/복구:
  - serialization unique id `'CAFX'`
  - 버전: v5 + 하위 호환(v1~v4) 로드 경로 유지
  - 구현: `skse/CalamityAffixes/src/EventBridge.Serialization.cpp`

### 3.2 책임 분리 상태

- 이전의 단일 대형 TU에서 분리된 점은 긍정적:
  - `EventBridge.Config/Loot/Triggers/Actions/Traps/Serialization.cpp`
- 그러나 여전히 `Loot` 단일 파일 비중이 큼:
  - `skse/CalamityAffixes/src/EventBridge.Loot.cpp` = 2,521 LOC
  - 전체 `skse/.../src/*.cpp` 합계 9,710 LOC 중 단일 파일 비중이 높음

### 3.3 적용 범위 명시

- 코드/문서 기준으로 플레이어 중심 범위가 일치:
  - 플레이어 인벤 기준 active affix 집계
  - `IsPlayerOwned`(player + commanding actor) 기반 전투 트리거
  - README/MCM 문구로 제한사항 노출

---

## 4) 주요 리스크 (우선순위)

## [MEDIUM] R1. 유지보수성 리스크: `EventBridge.Loot.cpp` 과대화

증상:
- `Loot` 파일에 인스턴스 부여, 툴팁 후보 매칭, 룬워드 베이스 선택/삽입, 컨테이너 이벤트 처리 로직이 밀집

영향:
- 작은 수정도 회귀 반경이 커짐
- 리뷰/온보딩 비용 증가

권장:
1. `Loot.Assign`, `Loot.Runeword`, `Loot.TooltipResolve` 식 내부 분할(행동 불변)
2. 문자열/선택 규칙 pure helper 추출 후 단위 테스트 추가

---

## [MEDIUM] R2. 런타임 테스트 한계: SKSE 경로는 static check 중심

현황:
- CTest는 `CalamityAffixesStaticChecks`(object compile + static_assert) 위주
- 이벤트 경합/중복 억제/ICD 시간창 같은 동작적 회귀를 잡기 어려움

영향:
- 복잡한 트리거 조건 변경 시 “컴파일은 통과했지만 인게임 회귀” 가능성

권장:
1. deterministic helper 계층을 늘려 로직 테스트 가능 영역 확대
2. 최소 3개 우선 추가:
   - duplicate-hit suppression key 규칙
   - per-target ICD block/commit 규칙
   - tooltip candidate disambiguation 규칙

---

## [LOW] R3. Papyrus 소스 대비 컴파일 산출물 범위가 기여자에게 혼동 가능

현황:
- `Data/Scripts/Source`에는 `AffixManager/PlayerAlias/SkseBridge`가 남아 있으나,
- 빌드 스크립트는 현재 `ModeControl/ModEventEmitter/MCMConfig`만 컴파일

영향:
- 신규 기여자가 “왜 특정 .psc가 .pex로 안 나오는지” 혼동할 수 있음

권장:
1. `tools/compile_papyrus.sh` 상단에 “현재 활성 Papyrus 경로” 주석 2~3줄 추가
2. README `Papyrus ↔ SKSE 브리지` 섹션에 활성/레거시 구분 한 줄 명시

---

## [LOW] R4. 공식 CK 문서 접근성(봇 차단)으로 외부 검증 재현성 저하

현황:
- `ck.uesp.net` 본문 추출이 환경에 따라 제한됨

영향:
- 리뷰 문서에서 외부 근거 링크의 재현성이 떨어질 수 있음

권장:
1. 레포 내부 `docs/references/`에 핵심 이벤트 시그니처 요약(출처 URL 포함) 보관
2. 코드 주석의 이벤트 시그니처 근거를 내부 문서 링크와 함께 유지

---

## 5) 긍정 요소 (유지 권장)

1. 플레이어 중심 범위 정의가 코드/문서/MCM에서 일치하기 시작함
2. v5 직렬화 + 하위 호환 로드 경로 유지
3. drop-delete 기반 프루닝 + 재획득 안전장치(`PlayerHasInstanceKey`)가 추가되어 장기 세이브 누적 리스크 완화
4. 배포 파이프라인(`build_mo2_zip.sh`)이 생성/컴파일/lint를 선행해 “stale artifact 배포” 가능성이 낮음

---

## 6) 2주 실행안 (권장)

### Week 1
1. `EventBridge.Loot.cpp` 내부 2~3개 helper unit으로 분할 (행동 변화 없음)
2. pure helper 테스트 2개 추가 (tooltip disambiguation, runeword selection guard)
3. Papyrus 활성 경로 주석/README 보강

### Week 2
1. trigger suppression/ICD pure logic 테스트 1~2개 추가
2. docs/references에 CK 이벤트 시그니처 요약 추가
3. 재분석 리포트 업데이트(v3) + 릴리즈 노트에 “테스트 범위 확장” 항목 반영

---

## 7) 참고 소스

- GitHub release management docs:
  - https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository
- CommonLibSSE-NG event structures:
  - https://ng.commonlib.dev/TESHitEvent_8h_source.html
  - https://ng.commonlib.dev/TESContainerChangedEvent_8h_source.html
- Nexus authoring best practices/policies:
  - https://help.nexusmods.com/article/136-best-practices-for-mod-authors
  - https://help.nexusmods.com/article/28-file-submission-guidelines

보충:
- CK 이벤트 원문(환경에 따라 접근 제한 가능):
  - https://ck.uesp.net/wiki/OnHit_-_ObjectReference
  - https://ck.uesp.net/wiki/OnObjectEquipped_-_Actor

