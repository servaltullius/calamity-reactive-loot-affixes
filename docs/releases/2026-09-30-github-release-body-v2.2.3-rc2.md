# Calamity - Reactive Loot & Affixes v2.2.3-rc2

2.2.3-rc2는 **스카이림 AE 1.7.x에서 모드가 로드되지 않던 문제**를 고친 두 번째 공개 테스트 빌드입니다. rc1로 첫 번째 오류는 사라졌지만 그다음 단계에서 다른 오류가 나서, 그 부분을 고쳤습니다. 게임 동작, ESP, 코세이브, 게임 데이터는 [2.2.2](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.2.2)와 같습니다. 1.7.x에서 확인되면 같은 내용으로 2.2.3 정식을 냅니다. 그때까지 정식 버전은 v2.2.2입니다.

*English summary: second test build for Skyrim AE 1.7.x (e.g. 1.7.104). rc1 fixed the runtime detection, which got past `failed to open address library file` but then stopped at `Unsupported address library format: 5`. The 1.7.x Address Library uses a new file format (5); rc2 reads it. SE 1.5.97 and AE 1.6.x load exactly as before. Install over 2.2.2 or rc1 with Replace, no new game needed.*

## rc1 이후 고친 점

- **AE 1.7.x에서 rc1로 설치했을 때 나던 이 오류를 고쳤습니다.**

  ```
  REL/Relocation.h(1139): Unsupported address library format: 5
  ```

  1.7.x용 Address Library는 새 형식(format 5, ID 순서대로 주소를 나열한 표)으로 배포됩니다. DLL에 들어간 CommonLibSSE-NG는 기존 형식 1·2만 읽을 수 있었습니다. rc1에서 버전 인식을 고치자 올바른 파일(`versionlib-1-7-104-0.bin`)은 열었지만, 이 단계에서 멈췄습니다. 이제 형식 5도 읽습니다. 파일 구조는 유지 관리되는 포크 alandtse/CommonLibSSE-NG의 PR 299와 같습니다.

## rc1에서 고친 점 (그대로 포함)

- AE 1.7.x를 SE로 잘못 판단해 `REL/Relocation.h(1104): failed to open address library file`로 멈추던 문제. 이제 게임 버전의 두 번째 자리가 6 이상이면 AE로 인식합니다.

## 호환성

- SE 1.5.97과 AE 1.6.x에서는 달라지는 것이 없습니다. 이 버전들이 쓰는 형식 1·2는 전과 똑같이 읽습니다.
- 기존 세이브를 그대로 쓸 수 있습니다. 코세이브 형식은 2.2.0과 같습니다.
- AE 1.7.x에서는 그 버전에 맞는 SKSE와 Address Library(`versionlib-1-7-xxx-0.bin`)가 필요합니다.

## 확인한 범위

- 수정은 CommonLibSSE-NG 로컬 패치로 넣었고, 벤더링한 코드가 원본 커밋과 패치 4개로 정확히 재현되는 것을 확인했습니다.
- 형식 5 읽기는 1.7.99 Address Library 파일로 파일 구조와 주소 값을 대조했습니다. Calamity가 훅을 거는 Actor·Character·PlayerCharacter 가상 함수 표는 1.6.1170과 1.7.99에서 크기가 같습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.
- AE 1.6.1170에서 정상 로드를 확인했습니다. **AE 1.7.x에서의 실제 실행은 아직 확인 전**이며, 이 테스트 빌드로 확인을 받습니다. 문제가 남아 있으면 `skse64.log`와 `CalamityAffixes.log`를 함께 알려 주세요.

## 설치

1. **`CalamityAffixes_MO2_v2.2.3-rc2_2026-09-30.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
