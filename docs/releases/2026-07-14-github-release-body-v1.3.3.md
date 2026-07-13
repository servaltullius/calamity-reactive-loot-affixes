# Calamity - Reactive Loot & Affixes v1.3.3

> Pre-release / Recipe Explorer scroll stability hotfix
> 날짜: 2026-07-14

## 요약

v1.3.3은 v1.3.2의 Prisma 패널 성능 개선을 유지하면서, 레시피 탐색기의 마우스 wheel과 네이티브 scrollbar가 서로 위치를 빼앗던 회귀를 수정하는 후속 공개 테스트 hotfix입니다.

v1.3.2에서 wheel animation이 이전 목표 위치를 계속 소유한 상태로 scrollbar thumb를 움직이면, 다음 RAF가 사용자가 고른 위치를 다시 이전 목표 쪽으로 끌어당길 수 있었습니다. 일부 CEF 입력 경로에서는 일반 마우스 휠이 작은 pixel delta로 전달되어 지나치게 느리게 이동하기도 했습니다.

수정 후보 패키지를 실제 게임에서 확인한 결과 레시피 탐색기 스크롤이 부드러워진 것을 확인했습니다.

## 수정 사항

- pointerdown과 구형 CEF용 mousedown에서 진행 중인 wheel RAF를 즉시 취소하고 실제 scrollTop과 동기화합니다.
- thumb, track click, 키보드 또는 외부 scrollTop 변경을 capture scroll 이벤트로 감지해 이전 wheel 목표를 버립니다.
- native scroll 통지가 한 RAF 늦게 도착해도 다음 RAF가 위치를 쓰기 전에 실제 위치와 마지막 자체-write 위치를 비교합니다.
- 명확한 120단위 일반 마우스 휠 노치만 한 노치당 64px 목표로 정규화합니다.
- 트랙패드의 작은 연속 delta는 강제 증폭하지 않습니다.
- 감쇠 시간상수를 72ms에서 48ms로 줄여 wheel 반응을 빠르게 합니다.
- 0.75px 도달 거리와 0.5px 최소 유효 이동량으로 정수형 CEF의 마지막 1px 부근에서도 RAF를 확실히 끝냅니다.
- 커스텀 frame-paced wheel 처리는 명시된 레시피 탐색기에만 적용하며 다른 패널 scroller는 네이티브 동작을 사용합니다.

## 호환성 및 변경하지 않은 항목

- 새 게임은 필요하지 않습니다.
- 기존 장비, 보유 파편, 재련 오브와 완성된 룬워드는 그대로 유지됩니다.
- 저장 및 어픽스·룬워드 직렬화 형식은 변경하지 않았습니다.
- 어픽스 및 룬워드 효과 정의는 변경하지 않았습니다.
- 룬 가중치와 드롭 분포는 변경하지 않았습니다.
- 기본 파편 드롭률 8%와 99회 무드롭 피티는 변경하지 않았습니다.
- Prisma UI, SKSE64와 Address Library 요구 사항은 기존과 같습니다.
- v1.3.2의 Tick 병합, 정적 레시피 카탈로그 분리, keyed DOM 재사용, 패널 telemetry와 View 복구 경로를 유지합니다.

## 실제 테스트 요청

다음 동작을 중심으로 확인해 주세요.

1. 마우스 휠 한 노치 속도가 적당한지
2. 휠 직후 scrollbar thumb를 빠르게 이동해도 놓은 위치에 남는지
3. scrollbar track click과 빠른 연속 wheel 입력이 이전 위치로 되돌아가지 않는지
4. 패널을 30초 이상 연 상태에서도 지속적인 RAF 또는 프레임 저하가 없는지
5. 트랙패드를 사용하는 경우 작은 연속 이동이 과도하게 증폭되지 않는지

## 자동 검증

최종 릴리스 빌드에서 다음 검증을 통과했습니다.

- 실제 HTML scroll controller Node 동작 테스트 통과
- Python 도구 및 Prisma 성능 계약 테스트: 67/67
- Generator 테스트: 78/78
- C++ static/runtime-gate/분포 테스트: 3/3
- Papyrus 컴파일: 3/3, 오류·경고 없음
- affix compose/lint/MCM JSON/runtime contract 동기화 통과
- SKSE DLL 전체 컴파일·링크 통과
- MO2 ZIP 구조 검증 통과
- ZIP 내부 룬 33종과 4/3/2/1 분포 확인
- 빌드 DLL, Data DLL과 ZIP 내부 DLL SHA256 일치
- 수정 후보의 실제 Skyrim 테스트에서 레시피 탐색기 스크롤이 부드러워진 것을 확인

## 알려진 제한

- 실제 입력 지연과 frame pacing은 해상도, CEF, GPU 드라이버와 다른 UI 모드 조합에 따라 달라질 수 있습니다.
- 레시피 목록은 overscroll containment를 유지하므로 목록 경계에서 wheel을 부모 workspace로 자동 전달하지 않습니다.
- Node 테스트는 정수형 CEF, 지연 native event와 wheel 입력을 재현하지만 실제 Skyrim 전체 렌더 파이프라인을 완전히 복제하지는 않습니다.

## 배포 자산

- CalamityAffixes_MO2_v1.3.3_2026-07-14.zip
- CalamityAffixes_MO2_v1.3.3_2026-07-14.zip.sha256
- SHA256: b958e38671a3617c27961b36a33558a7838682448fa98d35514797063167141b
