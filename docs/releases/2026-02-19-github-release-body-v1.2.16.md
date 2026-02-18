## CalamityAffixes v1.2.16

이번 빌드는 생성기/검증 경로의 안정성을 강화하고, 런타임 리버트 상태 초기화의 누락을 보완한 안정화 릴리즈입니다.

### 핵심 변경
- 생성기 출력 경로 안전화
  - `modKey`를 파일명 전용으로 강제해 경로 탈출(`../`, 절대경로) 입력을 차단했습니다.
- 런타임 리버트 상태 초기화 대칭성 보강
  - summon corpse-explosion 관련 캐시/상태를 `Load`/`Revert` 모두에서 정리하도록 맞췄습니다.
- 검증 규칙 단일화
  - `tools/affix_validation_contract.json`을 도입해 C# 로더와 Python lint가 동일한 trigger/action 타입 집합을 사용합니다.
- 런타임 shape 하드 검증 추가
  - 생성기 진입점에서 `runtime.trigger`, `runtime.action.type`, `procChancePercent`, `icdSeconds`, `perTargetICDSeconds`를 즉시 검증해 실패를 조기 감지합니다.
- 회귀 테스트 보강
  - `modKey` 안전성 및 runtime 경계값(누락/지원되지 않는 타입/범위 오류/비숫자)을 커버하는 테스트를 추가했습니다.

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release --nologo` 통과 (38/38)
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과 (2/2)
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` 통과
- `tools/build_mo2_zip.sh` 통과

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.16_2026-02-19.zip`

### 비고
- 이 릴리즈 노트 본문은 한국어 기준으로 작성되었습니다.
