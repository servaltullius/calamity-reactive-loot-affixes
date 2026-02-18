# CalamityAffixes Nexus 업로드 플레이북 (2026-02-09)

이 문서는 `Calamity - Reactive Loot & Affixes`를 Nexus Mods에 올릴 때 필요한 실무 절차를 한 번에 수행하기 위한 가이드입니다.

적용 대상:
- 메인 배포 파일(MO2 ZIP)
- 모드 페이지 본문/요약/의존성/스크린샷
- 정책 준수(검역, 성인 태그, 권한/크레딧)

기준 버전:
- Git tag: `v0.1.0-beta.1`
- 현재 배포 산출물 예시: `dist/CalamityAffixes_MO2_2026-02-09.zip`
- 붙여넣기 완성본: `docs/releases/2026-02-09-nexus-publish-copy-v0.1.0-beta.1.md`
- 릴리즈 본문 템플릿: `docs/releases/release-note-template.md`
- 인게임 QA 스모크 체크리스트: `docs/releases/2026-02-09-ingame-qa-smoke-checklist.md`

---

## 1) 업로드 전 준비 (로컬)

아래 순서대로 실행하면 생성/검증/압축까지 한 번에 끝납니다.

```bash
cd "/home/kdw73/projects/Calamity - Reactive Loot & Affixes"
tools/build_mo2_zip.sh
```

수동으로 DLL만 재빌드해야 할 때(WSL/Linux cross):

```bash
cd "/home/kdw73/projects/Calamity - Reactive Loot & Affixes/skse/CalamityAffixes"
cmake --build build.linux-clangcl-rel --target CalamityAffixes
ctest --test-dir build.linux-clangcl-rel --output-on-failure
cp -f build.linux-clangcl-rel/CalamityAffixes.dll ../../Data/SKSE/Plugins/CalamityAffixes.dll
sha256sum build.linux-clangcl-rel/CalamityAffixes.dll ../../Data/SKSE/Plugins/CalamityAffixes.dll
```

주의:
- DLL 빌드는 `build.linux-clangcl-rel` lane을 사용합니다.
- `linux-release-commonlibsse`는 테스트/정적체크 lane(g++)이며 DLL 산출 lane이 아닙니다.
- `fatal error: Windows.h: No such file or directory`는 대체로 lane 선택 오류 신호입니다.

스크립트가 자동 수행하는 항목:
- 생성기 실행: `affixes/affixes.json` -> `Data/*` 산출물 동기화
- Papyrus 컴파일: `Data/Scripts/*.pex`
- 스펙/생성물 정합 검사: `tools/lint_affixes.py`
- MO2 배포 ZIP 생성: `dist/CalamityAffixes_MO2_<YYYY-MM-DD>.zip`

업로드 직전 확인:
- ZIP 내부 루트가 `CalamityAffixes/`인지 확인 (`Data/` 이중 중첩 금지)
- 최신 DLL 포함 확인: `Data/SKSE/Plugins/CalamityAffixes.dll`
- 최신 런타임 설정 포함 확인: `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- 인게임 QA 스모크 체크리스트 1회 수행: `docs/releases/2026-02-09-ingame-qa-smoke-checklist.md`

---

## 2) Nexus 페이지 등록 체크리스트

필수 입력(권장값):
1. Mod name: `Calamity - Reactive Loot & Affixes`
2. Version: `0.1.0-beta.1` (태그와 동일하게 유지 권장)
3. Summary(짧은 소개): 아래 템플릿 사용
4. Description(상세): 아래 한국어 본문 템플릿 사용
5. Category/Tags: 어픽스/루트/프로크/SKSE 성격이 드러나게 설정
6. Requirements: 필수/권장/선택 의존성 링크 추가
7. Main file 업로드: `CalamityAffixes_MO2_<YYYY-MM-DD>.zip`

강력 권장:
1. 스크린샷 3~6장 (인벤 툴팁, Prisma 패널, 룬워드 결과)
2. 변경 로그(Changelog) 한글 작성
3. 알려진 제한(플레이어 중심 런타임) 명시
4. 버그 리포트 양식(재현 단계/로드오더/로그) 명시

---

## 3) 의존성 섹션 (붙여넣기용)

### 필수
- SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
- Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
- Prisma UI (툴팁/조작 패널): https://www.nexusmods.com/skyrimspecialedition/mods/148718

### 권장
- SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
- KID (Keyword Item Distributor): https://www.nexusmods.com/skyrimspecialedition/mods/55728
- SPID (Spell Perk Item Distributor): https://www.nexusmods.com/skyrimspecialedition/mods/36869

### 선택
- MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
- Inventory Interface Information Injector (I4): https://www.nexusmods.com/skyrimspecialedition/mods/85702

---

## 4) Nexus 설명 본문 템플릿 (한국어, 붙여넣기용)

아래 블록을 Nexus Description에 그대로 붙여넣고 버전만 바꿔 사용하세요.

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
- SKSE64 (런타임 일치)
- Address Library for SKSE Plugins
- Prisma UI

### 권장
- SkyUI
- KID
- SPID

### 선택
- MCM Helper
- I4

## 설치 방법 (MO2 권장)
1. Main Files에서 최신 ZIP 다운로드
2. MO2에서 ZIP 설치
3. 로드오더에서 `CalamityAffixes.esp` 활성화
4. SKSE로 게임 실행

## 업데이트 방법
1. 기존 버전 위에 새 버전 덮어 설치
2. 세이브 로드 후 인벤/전투에서 동작 확인
3. 문제 발생 시 새 세이브/클린 테스트 프로필로 재현 확인

## 호환성/주의
- Prisma UI가 없으면 툴팁/패널 UI가 표시되지 않습니다.
- KID의 DoT 태그를 과도하게 넓게 분배하면 부작용이 발생할 수 있습니다.
- 현재 버전은 팔로워/NPC 독립 어픽스 운영을 지원하지 않습니다.

## 버그 제보 양식
- 게임 런타임(SE 1.5.97 / AE 1.6.x):
- 로드오더(관련 모드):
- 재현 단계:
- 기대 동작:
- 실제 동작:
- 로그/스크린샷:
```

---

## 5) Nexus 요약 문구 템플릿

### 한국어 요약 (짧은 소개)
`플레이어 중심 인스턴스 어픽스 + 프로크/ICD 시스템. Prisma UI 기반 툴팁/조작 패널과 룬워드 제작을 제공합니다.`

### 영어 요약 (짧은 소개)
`Player-centric instance affix system with proc/ICD guards, Prisma UI tooltip/control panel, and runeword crafting for Skyrim SE/AE.`

---

## 6) 변경 로그 템플릿 (한국어)

```md
## vX.Y.Z (YYYY-MM-DD)
- [추가] ...
- [개선] ...
- [수정] ...
- [호환] ...
- [문서] ...

### 영향 범위
- 플레이어 영향: ...
- 세이브 호환성: ...
- 알려진 이슈: ...
```

---

## 7) 스크린샷/미디어 체크리스트

최소 3장 권장:
1. 인벤토리 선택 아이템 + 우측 툴팁 오버레이
2. Prisma 조작 패널(룬워드/어픽스 탭)
3. 룬워드 완성 후 아이템명/효과 반영 화면

추가 권장:
1. MCM 옵션(런타임 범위 안내 포함)
2. 전투 중 프로크 확인 장면

---

## 8) 검역(Quarantine) 방지/대응 SOP

업로드 전에:
1. 압축 포맷은 `ZIP` 또는 `7z` 사용
2. 내부에 또 다른 압축파일(nested archive) 넣지 않기
3. 암호 ZIP 사용 금지
4. EXE/자기추출형 아카이브 필요 시 사유를 설명문에 명시

검역 발생 시:
1. 파일을 즉시 삭제하지 말고 상태 유지
2. 링크 포함하여 support@nexusmods.com 또는 모더레이터에 문의
3. 필요하면 재압축 후 재업로드

---

## 9) 정책 준수 체크리스트 (발행 전 최종)

1. 권한 없는 타 모드 자산 미포함
2. 크레딧/출처 명시
3. 파일 설명이 실제 기능과 일치
4. Placeholder/비기능 파일 업로드 금지
5. 성인/민감 콘텐츠가 있으면 정확한 태그 설정
6. 파일명/태그/카테고리 오남용 금지
7. Requirements(선행 모드) 링크 제공

---

## 10) 발행 당일 운영 체크리스트

1. Main File 업로드 + 버전 입력
2. Description/Requirements/Changelog 갱신
3. 스크린샷/썸네일 적용
4. 공개 후 파일 다운로드 테스트(MO2 설치 테스트)
5. 첫 댓글에 알려진 제한/문의 양식 고정
6. 24시간 동안 버그 리포트 모니터링

---

## 11) 공식 참고 문서 (Nexus Mods)

아래 정책은 주기적으로 바뀔 수 있으므로 업로드 직전에 재확인하세요.

- File Submission Guidelines: https://help.nexusmods.com/article/28-file-submission-guidelines
- Best Practices for Mod Authors: https://help.nexusmods.com/article/136-best-practices-for-mod-authors
- Adult Content Guidelines: https://help.nexusmods.com/article/19-adult-content-guidelines
- Why has my mod been quarantined?: https://help.nexusmods.com/article/117-why-has-my-mod-been-quarantined
- Virus Scanning at Nexus Mods: https://help.nexusmods.com/article/128-anti-virus-false-positives
- How can I add a new game to Nexus Mods?: https://help.nexusmods.com/article/104-how-can-i-add-a-new-game-to-nexus-mods
