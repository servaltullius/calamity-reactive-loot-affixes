# Nexus 게시용 완성본 (v0.1.0-beta.1)

이 문서는 Nexus Mods 업로드 화면에 그대로 붙여넣기 위한 최종 문안입니다.

기준:
- 버전: `v0.1.0-beta.1`
- 배포 파일: `dist/CalamityAffixes_MO2_2026-02-09.zip`

---

## 1) 페이지 기본값 (입력용)

1. Mod name: `Calamity - Reactive Loot & Affixes`
2. Version: `0.1.0-beta.1`
3. Main file name: `CalamityAffixes_MO2_2026-02-09.zip`
4. Main file description:
`v0.1.0-beta.1 (runtime-state v5). Player-centric instance affix runtime with Prisma UI tooltip/control panel and runeword flow.`
5. Summary (KR):
`플레이어 중심 인스턴스 어픽스 + 프로크/ICD 시스템. Prisma UI 기반 툴팁/조작 패널과 룬워드 제작을 제공합니다.`
6. Summary (EN):
`Player-centric instance affix system with proc/ICD guards, Prisma UI tooltip/control panel, and runeword crafting for Skyrim SE/AE.`

---

## 2) Description 본문 (한국어, 붙여넣기용)

```md
## 모드 개요
Skyrim SE/AE용 Diablo/PoE 스타일 어픽스 시스템입니다.
아이템 인스턴스(ExtraUniqueID) 기준으로 어픽스를 부여하고, 프로크(확률 발동) + ICD(내부 쿨다운) + 안전 가드를 런타임에서 처리합니다.

## 현재 적용 범위 (중요)
- 현재 런타임은 **플레이어 중심**입니다.
- 플레이어 장비/인벤토리 기준으로 어픽스가 동작합니다.
- 전투 트리거는 플레이어 본인 + 플레이어 지휘 소환체(player-owned summon/proxy)까지 지원합니다.
- 일반 팔로워/NPC 장비를 독립 어픽스 소유자로 추적/운영하는 기능은 현재 버전에 포함되지 않습니다.

## 핵심 기능
- Loot-time 인스턴스 어픽스 롤링 (획득/제작 시점)
- 프로크/ICD/중복 방지 가드
- Prisma UI 기반 툴팁/조작 패널
- 룬워드 제작/상태 표시
- 코세이브 직렬화 기반 인스턴스 상태 유지

## 선행 의존성
### 필수
- SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
- Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
- Prisma UI: https://www.nexusmods.com/skyrimspecialedition/mods/148718

### 권장
- SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
- KID (Keyword Item Distributor): https://www.nexusmods.com/skyrimspecialedition/mods/55728
- SPID (Spell Perk Item Distributor): https://www.nexusmods.com/skyrimspecialedition/mods/36869

### 선택
- MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
- Inventory Interface Information Injector (I4): https://www.nexusmods.com/skyrimspecialedition/mods/85702

## 설치 방법 (MO2 권장)
1. Main Files에서 최신 ZIP 다운로드
2. MO2에서 ZIP 설치
3. 로드오더에서 `CalamityAffixes.esp` 활성화
4. SKSE로 게임 실행

## 업데이트 방법
1. 기존 버전 위에 새 버전을 덮어 설치
2. 세이브 로드 후 인벤/전투에서 동작 확인
3. 문제 발생 시 새 세이브/클린 테스트 프로필에서 재현 확인

## 호환성/주의
- Prisma UI가 없으면 툴팁/패널 UI가 표시되지 않습니다.
- KID의 DoT 태그를 과도하게 넓게 분배하면 부작용이 발생할 수 있습니다.
- 현재 버전은 팔로워/NPC 독립 어픽스 운영을 지원하지 않습니다.

## 버그 제보 템플릿
- 게임 런타임(SE 1.5.97 / AE 1.6.x):
- 로드오더(관련 모드):
- 재현 단계:
- 기대 동작:
- 실제 동작:
- 로그/스크린샷:
```

---

## 3) Changelog (한국어, 붙여넣기용)

```md
## v0.1.0-beta.1 (2026-02-09)
- [개선] 런타임 상태 키를 인스턴스 단일 키에서 `(instanceKey, affixToken)` 복합 키로 리팩터링해 어픽스 간 상태 간섭을 방지
- [개선] 직렬화 v5 추가 및 per-affix 런타임 상태 레코드(`IRST`) 저장 방식 도입
- [호환] v1~v4 세이브 로드 마이그레이션 경로 유지
- [수정] 드랍 아이템 인스턴스 삭제 시 해당 인스턴스 런타임 상태가 함께 정리되도록 보강
- [UX] 플레이어 중심 런타임 범위를 문서/MCM에 명확히 표기
- [배포] 생성 산출물 + DLL 갱신 후 MO2 ZIP 재빌드

### 영향 범위
- 플레이어 영향: 룬워드/보조 어픽스 공존 시 런타임 상태 정확도 향상
- 세이브 호환성: 기존 세이브 유지 (자동 마이그레이션)
- 알려진 제한: 일반 팔로워/NPC 독립 어픽스 운영 미지원
```

---

## 4) Requirements 섹션 (Nexus 입력용)

아래 링크를 Nexus Requirements에 각각 등록하세요.

1. SKSE64: https://skse.silverlock.org/
2. Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
3. Prisma UI: https://www.nexusmods.com/skyrimspecialedition/mods/148718
4. SkyUI (recommended): https://www.nexusmods.com/skyrimspecialedition/mods/12604
5. KID (recommended): https://www.nexusmods.com/skyrimspecialedition/mods/55728
6. SPID (recommended): https://www.nexusmods.com/skyrimspecialedition/mods/36869
7. MCM Helper (optional): https://www.nexusmods.com/skyrimspecialedition/mods/53000
8. I4 (optional): https://www.nexusmods.com/skyrimspecialedition/mods/85702

---

## 5) 첫 고정 댓글 템플릿 (한국어)

```md
v0.1.0-beta.1 업로드 완료했습니다.

핵심 변경:
- runtime-state v5 적용 (어픽스별 상태 분리/저장 정확도 개선)
- 기존 세이브(v1~v4) 호환 마이그레이션 유지
- 적용 범위 표기 정리: 현재 플레이어 중심 런타임

중요 안내:
- 일반 팔로워/NPC 독립 어픽스 운영은 현재 미지원입니다.
- Prisma UI는 필수 의존성입니다.

버그 제보 시 아래를 함께 남겨주세요:
1) 게임 런타임 버전(SE/AE)
2) 로드오더 핵심 모드
3) 재현 단계
4) 스크린샷/로그
```

---

## 6) 업로드 직전 5분 체크

1. `tools/build_mo2_zip.sh` 재실행
2. `dist/CalamityAffixes_MO2_2026-02-09.zip` 파일 선택
3. Version = `0.1.0-beta.1` 입력
4. Description/Changelog는 위 템플릿 붙여넣기
5. Requirements 링크 등록 확인
6. 공개 후 모드 페이지에서 Main file 다운로드 버튼 정상 동작 확인

---

## 7) 참고 문서

- 업로드 플레이북(전체 가이드): `docs/releases/2026-02-09-nexus-upload-playbook.md`
- 릴리즈 노트 원본: `docs/releases/2026-02-09-runtime-state-v5.md`

