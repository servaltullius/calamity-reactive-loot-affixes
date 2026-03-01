# docs/ 디렉토리 인덱스

## 루트 문서

| 파일 | 설명 |
|------|------|
| `NEXUS_DESCRIPTION.md` | Nexus Mods 배포 페이지 설명 (한국어/영어) |
| `NEXUS_DESCRIPTION_BBCODE.txt` | Nexus Mods BBCode 포맷 설명 |
| `스카이림 바닐라 효과 ARPG 변환 아이디어.md` | 바닐라 효과 → ARPG 어픽스 변환 기획 노트 |

## adr/ — Architecture Decision Records

프로젝트 핵심 아키텍처 결정 기록.

| 파일 | 설명 |
|------|------|
| `0001-instance-affix-storage-extrauniqueid-serialization.md` | 인스턴스 어픽스 저장 방식 (ExtraUniqueID + 직렬화) |
| `0002-prisma-ui-tooltip-overlay.md` | Prisma UI 기반 툴팁 오버레이 결정 |
| `0003-trigger-pipeline-healthdamage-hook-hit-event-dot-apply.md` | 전투 트리거 파이프라인 (HealthDamage 훅 + HitEvent + DoT) |

## plans/ — 기획 및 설계 문서

기능 기획, 코드 리뷰, 리팩터링 계획 등.

주요 문서:
- `2026-02-12-prefix-suffix-system-v1.md` — Prefix/Suffix 분류 시스템
- `2026-02-07-runeword-physical-fragments.md` — 룬워드 물리 조각 시스템
- `2026-02-13-bilingual-i18n.md` — 한영 이중 언어 지원
- `2026-02-02-eventbridge-split-v1.md` — EventBridge 도메인 분할
- `2026-02-09-eventbridge-actions-decomposition-v1.md` — EventBridge 액션 분해
- `2026-01-31-autogen-spell-mgef.md` — ESP 자동 생성 (MagicEffect/Spell)

## references/ — 참조 문서

외부 아키텍처 분석 및 호환성 정책.

| 파일 | 설명 |
|------|------|
| `2026-02-09-event-signatures.md` | SKSE 이벤트 시그니처 참조 |
| `2026-02-15-calamityaffixes-architecture-snapshot-v1.2.14.md` | v1.2.14 아키텍처 스냅샷 |
| `2026-02-22-corpse-drop-compat-policy.md` | 시체 드랍 호환성 정책 |

## releases/ — 릴리즈 기록

GitHub 릴리즈 본문, Nexus 배포 카피, QA 체크리스트 등 (78개 파일).

주요 유형:
- `github-release-body-*.md` — GitHub 릴리즈 노트
- `ingame-qa-smoke-checklist.md` — 인게임 QA 스모크 테스트
- `nexus-*` — Nexus Mods 배포 관련

## reviews/ — 코드 리뷰 및 분석

| 파일 | 설명 |
|------|------|
| `2026-02-18-deep-review.md` | 심층 코드 리뷰 |
| `2026-02-21-runeword-94-diff-rc2-to-rc6.md` | 룬워드 94종 RC2→RC6 변경 diff |
| `2026-02-26-deep-analysis-comprehensive.md` | 종합 심층 분석 |
| `2026-02-26-deep-analysis-round2-supplement.md` | 심층 분석 2차 보충 |
| `2026-02-26-deep-analysis-round3-final-decision.md` | 심층 분석 3차 최종 결정 |
