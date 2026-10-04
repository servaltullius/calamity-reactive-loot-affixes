# Calamity - Reactive Loot & Affixes v2.2.3-rc1

2.2.3-rc1은 **스카이림 AE 1.7.x에서 모드가 로드되지 않던 문제**를 고친 공개 테스트 빌드입니다. 게임 동작, ESP, 코세이브, 게임 데이터는 [2.2.2](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.2.2)와 같습니다. 1.7.x에서 확인되면 같은 내용으로 2.2.3 정식을 냅니다. 그때까지 정식 버전은 v2.2.2입니다.

*English summary: test build for Skyrim AE 1.7.x (e.g. 1.7.104), which failed at startup with `REL/Relocation.h(1104): failed to open address library file`. The runtime detection now treats 1.6 and newer as AE. Nothing else changes; install over 2.2.2 with Replace, no new game needed.*

## 고친 점

- **AE 1.7.x(예: 1.7.104)에서 게임 시작 시 이 오류로 멈추던 문제를 고쳤습니다.**

  ```
  REL/Relocation.h(1104): failed to open address library file
  ```

  DLL에 들어간 CommonLibSSE-NG는 게임 버전의 두 번째 자리가 **정확히 6**일 때만 AE로 인식했습니다. 1.7.x는 7이라 SE로 잘못 판단했고, 그래서 Address Library의 AE용 파일 `versionlib-1-7-104-0.bin` 대신 존재하지 않는 SE식 파일 `version-1-7-104-0.bin`을 찾다가 실패했습니다. Address Library 설치에는 문제가 없었습니다.

  이제 **6 이상을 AE로** 인식합니다. 이 라이브러리를 이어서 관리하는 포크(alandtse/CommonLibVR)와 같은 수정입니다.

## 호환성

- SE 1.5.97과 AE 1.6.x에서는 달라지는 것이 없습니다.
- 기존 세이브를 그대로 쓸 수 있습니다. 코세이브 형식은 2.2.0과 같습니다.
- AE 1.7.x에서는 그 버전에 맞는 SKSE와 Address Library(`versionlib-1-7-xxx-0.bin`)가 필요합니다.

## 확인한 범위

- 수정은 CommonLibSSE-NG 로컬 패치로 넣었고, 벤더링한 코드가 원본 커밋과 패치 4개로 정확히 재현되는 것을 확인했습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.
- AE 1.6.1170에서 정상 로드를 확인했습니다. **AE 1.7.x에서의 실제 실행은 아직 확인 전**이며, 이 테스트 빌드로 확인을 받습니다. 문제가 남아 있으면 `skse64.log`와 `CalamityAffixes.log`를 함께 알려 주세요.

## 설치

1. **`CalamityAffixes_MO2_v2.2.3-rc1_2026-09-30.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
