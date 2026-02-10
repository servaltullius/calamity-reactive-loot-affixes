# CalamityAffixes v1.0.2 (2026-02-10)

## 선행 의존성

- 필수
  - SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
  - Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
  - Prisma UI (툴팁/조작 패널): https://www.nexusmods.com/skyrimspecialedition/mods/148718
- 권장
  - SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
  - KID: https://www.nexusmods.com/skyrimspecialedition/mods/55728
  - SPID: https://www.nexusmods.com/skyrimspecialedition/mods/36869
- 선택
  - MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
  - I4: https://www.nexusmods.com/skyrimspecialedition/mods/85702

## 변경 요약

- (중요) 특정 룬워드/프로크에서 전투 시작 시 크래시(CTD) 가능성이 있던 문제를 수정했습니다.
  - 원인: Skyrim 1.6.629+ 이후 Actor 다중상속 레이아웃 변경으로 인해, `ActorValueOwner` 가상함수를 Actor에서 직접 호출할 때 잘못된 vtable로 점프할 수 있음
  - 수정: `actor->AsActorValueOwner()`를 통해 레이아웃-독립적으로 `GetActorValue/RestoreActorValue`를 호출하도록 변경
  - 포함: 번들된 CommonLibSSE-NG를 v3.5.4로 업데이트 (Actor vtable/부모 클래스 오프셋 관련 수정 포함)
- 인벤토리에서 간헐적으로 떠 있는 “패널 열기/닫기” 퀵런치 pill을 강제 숨김 처리했습니다. (HTML에서 `hidden` + inline `display:none`)

## 포함 파일

- `CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `CalamityAffixes_MO2_2026-02-10.zip`

## 검증

- `./tools/release_verify.sh --with-mo2-zip`

## SHA256

- `CalamityAffixes_MO2_2026-02-10.zip`
  - `abcd68d293c5d5ef1dbf64b2016a398dd37de8213f9e28bda2a335df19f04adc`

