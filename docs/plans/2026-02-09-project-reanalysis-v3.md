# Project Reanalysis v3 (2026-02-09)

## 0) TL;DR

- v2에서 제시한 **Week 1/Week 2 실행안은 완료**되었습니다.
- 핵심 성과:
  - `EventBridge.Loot.cpp`/`EventBridge.Triggers.cpp`의 고위험 분기 로직을 pure helper로 분리
  - static-check 테스트 범위 확장(9개 모듈)
  - CK/CommonLib 이벤트 시그니처 내부 레퍼런스 문서화
- 현재 남은 우선 리스크는 “구조 대분할”보다 **인게임 동작 회귀를 더 잘 잡는 실행형 테스트 전략**입니다.

---

## 1) 이번 업데이트 범위

1. v2 실행안(Week 1/2) 이행 결과 점검
2. 테스트/문서 동기화 상태 재확인
3. 다음 실행 우선순위(v4 후보) 정리

---

## 2) v2 실행안 이행 결과

## [완료] Week 1

1. `EventBridge.Loot.cpp` 내부 로직 helper 분리
   - 추가: `skse/CalamityAffixes/include/CalamityAffixes/LootUiGuards.h`
   - 테스트: `skse/CalamityAffixes/tests/test_loot_ui_guards.cpp`
2. 툴팁 후보 모호성/룬워드 베이스 선택 가드 규칙을 pure helper로 정적 검증
3. Papyrus 활성 경로 명시
   - `tools/compile_papyrus.sh` 주석 보강
   - `README.md` 활성/레거시 스크립트 구분 명시

관련 커밋:
- `ea8f664` Refactor loot UI guards and document active Papyrus path

## [완료] Week 2

1. trigger suppression/ICD 판정 pure helper 추출
   - 추가: `skse/CalamityAffixes/include/CalamityAffixes/TriggerGuards.h`
   - 테스트: `skse/CalamityAffixes/tests/test_trigger_guards.cpp`
2. CK/CommonLib 이벤트 시그니처 레퍼런스 문서 추가
   - `docs/references/2026-02-09-event-signatures.md`
3. README 브리지 섹션에 근거 문서 링크 연결

관련 커밋:
- `9a2650a` Add trigger guard tests and event signature references

---

## 3) 검증 스냅샷 (v3 작성 시점)

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
- SKSE static-check 모듈: 9개
  - `test_affix_token`, `test_adaptive_element`, `test_magnitude_scaling`, `test_loot_reroll_guard`, `test_loot_ui_guards`, `test_instance_state_key`, `test_resync_scheduler`, `test_runeword_util`, `test_trigger_guards`

---

## 4) 리스크 재평가 (v3)

## [MEDIUM] R1. 대형 파일 잔존

현황:
- `EventBridge.Loot.cpp` 2522 LOC
- `EventBridge.Triggers.cpp` 1139 LOC

평가:
- v2 대비 “분기 규칙 가시성”은 좋아졌지만, 파일 자체는 여전히 큼.

권장:
1. 다음 단계에서 `Loot.Assign`/`Loot.Runeword`/`Loot.Tooltip` 단위 물리 분할(별도 `.cpp`) 검토
2. Triggers도 health-damage route / TESHit fallback / ProcessTrigger core를 별도 단위로 분리 검토

## [MEDIUM] R2. 실행형(in-game) 회귀 탐지 부족

현황:
- static_assert 기반 검증은 확대되었으나, 실제 이벤트 타이밍 경합은 인게임에서만 확인 가능

권장:
1. 최소 재현 시나리오(수동 QA 템플릿) 문서화
2. 가능하면 로그 시그니처 기반 “smoke checklist” 자동화(패턴 grep) 도입

## [LOW] R3. CK 원문 접근 재현성

현황:
- `OnObjectUnequipped` 문서 접근이 환경에 따라 간헐적으로 제한됨(403)

완화:
- `docs/references/2026-02-09-event-signatures.md`로 내부 근거 고정
- README에서 레퍼런스 문서를 직접 참조하도록 연결

---

## 5) 다음 1주 권장안 (v4 후보)

1. `EventBridge.Loot.cpp`의 물리 분할(동작 불변) 1차
2. 인게임 QA 체크리스트 문서화(전투/루팅/룬워드/도트)
3. 릴리즈 노트 템플릿에 “테스트 범위(정적/인게임)” 고정 섹션 추가

---

## 6) 참고 소스

- CommonLibSSE-NG event structures
  - https://ng.commonlib.dev/TESHitEvent_8h_source.html
  - https://ng.commonlib.dev/TESContainerChangedEvent_8h_source.html
- CK 이벤트 페이지
  - https://ck.uesp.net/wiki/OnHit_-_ObjectReference
  - https://ck.uesp.net/wiki/OnObjectEquipped_-_Actor
  - https://ck.uesp.net/wiki/OnObjectUnequipped_-_Actor *(환경별 접근 제한 가능)*
- Nexus 문서
  - https://help.nexusmods.com/article/136-best-practices-for-mod-authors
  - https://help.nexusmods.com/article/28-file-submission-guidelines
