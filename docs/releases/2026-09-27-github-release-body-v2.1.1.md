# Calamity - Reactive Loot & Affixes v2.1.1

v2.1.1은 2.1.0의 패널 표시를 고친 업데이트입니다. 게임 동작, ESP, 코세이브는 2.1.0과 같습니다. 확인 스크롤 교환은 [2.1.0 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/releases/2026-09-27-github-release-body-v2.1.0.md), 2.0의 제작 흐름 전체는 [2.0.0 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/releases/2026-09-26-github-release-body-v2.0.0.md)를 확인하세요.

## 고친 점

- **선택 베이스 어픽스:** 패널 "작업 베이스 어픽스 및 재련"의 이 상자가 92px 안에서 따로 스크롤되어, 첫 어픽스 설명이 길면 나머지 어픽스가 아래로 숨었습니다. 이제 모든 어픽스가 펼쳐지고, 길어지면 오른쪽 열 전체가 스크롤됩니다.
- **교환 줄:** DLL이 교환 정보를 보내지 않을 때(DLL만 구버전으로 남은 경우) 확인 스크롤 교환 줄의 제목만 버튼 없이 남던 문제를 고쳤습니다.

## 확인한 범위

- 2026-09-27 사용자가 인게임에서 확인했습니다.
- 실제 길이의 어픽스 4줄로 Chromium에서 측정했습니다. 수정 전에는 36개 조합 모두에서 상자 안에 46~170px가 가려졌고, 수정 후에는 0px입니다. 전체 패널 레이아웃 점검(108개 조합: 크기 × 언어 × 접힘 상태 × 탭)에서 잘림·스크롤 문제 0건입니다.
- CSS 계약 테스트 두 개를 추가했고(수정 전 CSS에서 실패 확인), Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.1.1_2026-09-27.zip`** 전체를 설치합니다. MO2에서는 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다. 1.x에서 올라오는 경우의 안내는 2.0.0 노트와 같습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
