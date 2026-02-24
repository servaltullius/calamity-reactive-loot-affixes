# Affix Data Guide

## Scope

- 대상: `affixes/**` (모듈 스펙, 조합 결과, 스키마/런워드 계약)
- 핵심 파일:
  - `affixes/affixes.modules.json`
  - `affixes/modules/*.json`
  - `affixes/affixes.json` (compose 결과)

## Edit Flow (필수)

1. `affixes/modules/*.json` 수정
2. `python3 tools/compose_affixes.py`로 `affixes/affixes.json` 재조합
3. `python3 tools/lint_affixes.py --spec ... --manifest ... --generated ...` 실행
4. 필요 시 `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

## Conventions

- 모듈 manifest 순서/구조는 `affixes/affixes.modules.json`을 따릅니다.
- 스펙 구조는 `affixes/affixes.schema.json` 규칙을 만족해야 합니다.
- 허용 trigger/action 타입은 `tools/affix_validation_contract.json` 검증을 통과해야 합니다.
- runeword 계약은 `affixes/runeword.contract.json`을 우선 SSOT로 사용합니다.

## Anti-Patterns

- `affix.id`를 기존 값에서 rename하지 마세요(세이브 토큰 매칭 파손 위험).
- `affixes/affixes.json`을 수동 편집하지 마세요. modules 수정 후 compose로 갱신하세요.
- lint 오류/경고를 무시한 채 generator 산출물만 갱신하지 마세요.
- broad한 KID/SPID 규칙(무필터/과도한 범위)을 그대로 추가하지 마세요.

## Where To Look

- 모듈 조합 규칙: `affixes/affixes.modules.json`
- 루트 스펙 기본값: `affixes/modules/spec.root.json`
- 핵심 어픽스 모듈: `affixes/modules/keywords.affixes.core.json`
- 런워드/접미 모듈: `affixes/modules/keywords.affixes.runewords.json`, `affixes/modules/keywords.affixes.suffixes.json`
