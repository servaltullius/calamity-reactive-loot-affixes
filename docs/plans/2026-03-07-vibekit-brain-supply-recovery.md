# Vibe-kit Brain Supply Recovery Plan

Date: 2026-03-07
Status: completed

## Goal

삭제된 `.vibe/brain` 원본 때문에 깨진 vibe-kit 전체 기능을 다시 살린다.

이번 복구의 핵심은 gitignored 런타임 디렉터리인 `.vibe/brain` 을 더 이상 유일한 소스 원천으로 두지 않고, 레포에 추적되는 vendored 소스를 `scripts/vibe_kit/brain` 아래에 두는 것이다. `bootstrap` 과 `precommit` 은 이 vendored 소스를 `.vibe/brain` 으로 동기화한 뒤 실행한다.

## Non-Goals

- vibe-kit 도구의 기능 자체를 새로 설계하거나 재작성하기
- 네트워크 다운로드 기반 설치기 추가
- 기존 Calamity 게임 모드 런타임 동작 변경

## Affected Files

- Add: `scripts/vibe_kit/brain/*.py`
- Add: `scripts/vibe_kit/brain/requirements.txt`
- Add: `scripts/vibe_kit/templates/README.md`
- Add: `scripts/vibe_bootstrap.py`
- Modify: `scripts/setup_vibe_env.py`
- Modify: `scripts/vibe.py`
- Modify: `scripts/precommit_runner.py`

## Milestones

### 1. Vendored source recovery

- 과거 커밋에 있던 `.vibe/brain` 원본을 추적 경로로 복원
- README 템플릿도 함께 복원

### 2. Bootstrap flow repair

- `setup_vibe_env.py` 가 vendored brain 을 `.vibe/brain` 으로 설치
- `vibe.py` 가 config 존재 여부와 상관없이 brain 동기화를 보장

### 3. Seed / hook / runtime alignment

- `seed` export 가 vendored brain 소스를 포함
- `precommit_runner.py` 가 vendored brain 설치 후 실제 precommit 실행
- `.git/hooks/pre-commit` 이 새 경로와 일관되게 동작

## Validation

1. `python3 scripts/setup_vibe_env.py`
2. `python3 scripts/vibe.py precommit`
3. `python3 scripts/vibe.py search "EventBridge"`
4. `python3 scripts/vibe.py doctor`
5. Lint command
6. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
7. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
8. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: 과거 커밋의 vendored brain 을 복원하므로, 당시 가정한 repo layout 과 현재 repo layout 이 어긋난 기능이 있을 수 있다.
- Risk: `.vibe/brain` 은 여전히 런타임 복사본이므로, vendored source 와 수동 수정본이 어긋날 수 있다.
- Rollback: `scripts/vibe_kit` 와 `scripts/vibe_bootstrap.py` 를 제거하고, 기존 `.vibe/brain` 직접 의존 방식으로 되돌리면 된다.
