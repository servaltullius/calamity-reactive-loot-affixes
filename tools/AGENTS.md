# Generator/Tools Guide

## Scope

- 대상: `tools/**` (Generator, 검증 스크립트, 릴리즈 검증/패키징)
- 핵심 경로:
  - `tools/CalamityAffixes.Generator/`
  - `tools/CalamityAffixes.Generator.Tests/`
  - `tools/compose_affixes.py`, `tools/lint_affixes.py`, `tools/release_verify.sh`

## Structure

- `CalamityAffixes.Generator/Spec/`: affix spec 로딩/검증 계약
- `CalamityAffixes.Generator/Writers/`: KID/SPID/ESP/runtime JSON 출력
- `CalamityAffixes.Generator.Tests/`: xUnit 기반 회귀/계약/출력 검증
- `tests/test_compose_affixes.py`: compose 워크플로우 검증

## Commands

```bash
dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release
python3 tools/compose_affixes.py --check
python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
tools/release_verify.sh --fast
```

## Conventions

- Generator 입력 SSOT는 `affixes/affixes.json`입니다(모듈 직접 참조 금지).
- 허용 trigger/action 타입은 `tools/affix_validation_contract.json`으로 검증됩니다.
- runeword 계약은 `affixes/runeword.contract.json` 우선, 필요 시 fallback 체인을 사용합니다.
- 생성 출력은 staging 또는 `Data/`로 재생성하며 수동 동기화에 의존하지 않습니다.

## Anti-Patterns

- `tools/**/bin/**`, `tools/**/obj/**`를 수정하지 마세요(빌드 산출물).
- 생성된 INI/JSON를 hand-edit하지 마세요. 소스(spec/modules) 수정 후 재생성하세요.
- lint를 건너뛰고 generator 출력만 갱신하지 마세요(계약 불일치 위험).
- release 전 `release_verify.sh` 또는 동등 검증 체인을 생략하지 마세요.

## Where To Look

- spec 파싱/검증 실패: `CalamityAffixes.Generator/Spec/AffixSpecLoader.cs`
- 출력 파이프라인 문제: `CalamityAffixes.Generator/Writers/GeneratorRunner.cs`
- ESP/KID/SPID 생성 문제: `Writers/KeywordPluginBuilder.cs`, `Writers/KidIniWriter.cs`, `Writers/SpidIniWriter.cs`
- 계약/회귀 테스트: `CalamityAffixes.Generator.Tests/RuntimeContractSyncTests.cs`, `RepoSpecRegressionTests.cs`
