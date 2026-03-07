# Pre-commit Hook Environment Fix Plan

Date: 2026-03-07
Status: completed

## Goal

커밋을 막던 `pre-commit` 훅 오류를 정리한다.

현재 훅은 gitignored 경로인 `.vibe/brain/precommit.py` 를 직접 실행한다. 이 파일이 없는 새 환경이나 정리된 환경에서는 훅이 항상 실패하므로, 레포에 실제로 추적되는 runner 경로를 기준으로 훅을 복구하는 것이 목표다.

## Non-Goals

- vibe-kit 전체 도구 체인을 복원하거나 재배포하기
- `.vibe/brain/*.py` 전체를 레포에 포함시키기
- 기존 커밋/브랜치 전략 변경

## Affected Files

- Add: `scripts/precommit_runner.py`
- Modify: `scripts/install_hooks.py`
- Modify: `scripts/vibe.py`

## Milestones

### 1. Stable runner path

- 레포 추적 경로에 `precommit_runner.py` 추가
- legacy `.vibe/brain/precommit.py` 가 있으면 그대로 위임
- 없으면 안내 메시지만 출력하고 성공 종료

### 2. Hook install fix

- `.git/hooks/pre-commit` 템플릿이 `scripts/precommit_runner.py` 를 실행하도록 변경
- `python3 scripts/vibe.py precommit` 도 같은 runner 를 사용하도록 정리

### 3. Validation

- 훅 재설치
- `.git/hooks/pre-commit` 직접 실행
- lint / build / tests 로 기존 개발 플로우가 깨지지 않았는지 확인

## Validation

1. `python3 scripts/install_hooks.py --force`
2. `.git/hooks/pre-commit`
3. Lint command
4. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
5. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
6. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: 기존에 로컬 `.vibe/brain/precommit.py` 를 사용하던 사용자는 새 runner 를 거쳐 실행하게 된다.
- Risk: fallback 성공 종료 때문에 vibe-kit 검사가 없는 환경에서는 훅이 사실상 no-op 이 된다.
- Rollback: `scripts/install_hooks.py` 템플릿과 `scripts/vibe.py precommit` 경로를 이전 `.vibe/brain/precommit.py` 직접 호출 방식으로 되돌리면 된다.
