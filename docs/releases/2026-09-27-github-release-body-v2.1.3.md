# Calamity - Reactive Loot & Affixes v2.1.3

v2.1.3은 배포 ZIP에 서드파티 라이선스 고지를 더한 릴리스입니다. 게임 동작, ESP, 코세이브, 게임 데이터는 2.1.2와 같습니다. 룬워드 이름 통일은 [2.1.2 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/releases/2026-09-27-github-release-body-v2.1.2.md), 2.0의 제작 흐름 전체는 [2.0.0 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/releases/2026-09-26-github-release-body-v2.0.0.md)를 확인하세요.

## 바뀐 점

- ZIP의 `Docs/THIRD_PARTY_NOTICES.txt`에 DLL에 컴파일되는 오픈소스 라이브러리의 라이선스 고지를 담았습니다: CommonLibSSE-NG, spdlog, fmt, nlohmann/json(MIT), rapidcsv, Xbyak(BSD 3-Clause). 이 라이선스들은 바이너리를 배포할 때 고지를 함께 넣도록 요구합니다.
- 배포 패키지 검사가 이 파일을 필수로 확인하고, 새 라이브러리를 들여오면서 고지를 빠뜨리면 테스트가 실패합니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.1.3_2026-09-27.zip`** 전체를 설치합니다. MO2에서는 기존 Calamity 모드를 **Replace**하세요. 2.1.2에서 올라오는 경우 DLL의 버전 표기 말고는 게임 파일이 같습니다.
2. 새 게임은 필요하지 않습니다. 1.x에서 올라오는 경우의 안내는 2.0.0 노트와 같습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
