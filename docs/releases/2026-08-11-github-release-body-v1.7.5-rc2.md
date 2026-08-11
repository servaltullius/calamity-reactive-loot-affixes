# Calamity - Reactive Loot & Affixes v1.7.5-rc2

v1.7.5-rc2는 rc1의 함정 표시 동작을 유지하면서, **진단 버튼 자체도 실제 함정과 동일한 월드 레퍼런스 생성·애니메이션·정리 경로를 사용하도록 바꾼 QA·릴리스 강화 빌드**입니다.

사용자 인게임 검증에서 production 프로브의 함정 6종이 모두 표시됐고, 곰덫을 포함한 표시물이 정상적으로 정리되는 것을 확인했습니다. rc2에서 새 게임플레이 수치나 드랍 확률은 변경하지 않았습니다.

## 실제 production 함정 경로를 검사합니다

디버그 토글이 켜진 Prisma 패널의 `Production Trap World-Ref Probe`는 런타임 레지스트리에서 다음 6종의 실제 `trapFeedback` 정의를 해석합니다.

- 곰덫
- 룬 함정
- 역병 포자
- 역청 황폐
- 흡수 포자
- 혼돈의 룬

여섯 표시물을 플레이어 앞 두 줄에 배치하고, 라이브 함정과 같은 월드 레퍼런스 생성·곰덫 애니메이션 재시도·TrapSystem 정리 경로를 통과시킵니다. 약 3초 뒤 발동 전에 만료되므로 진단 과정에서 피해·감속·저주 주문은 적용하지 않습니다.

중복 실행, 분리된 플레이어 셀, 비활성 TrapSystem, 논리 함정 또는 물리 마커 상한 부족은 안전하게 거부됩니다. 로그 검사기는 여섯 레퍼런스가 엔진 경로에서 해석됐는지 `trap world marker probe contracts: OBSERVED 6/6`으로 집계하지만, 이 값만으로 화면 표시를 성공으로 판정하지는 않습니다.

## 저장·런타임 회귀 안전망

- 현재 코세이브 reader 중 LSBG v2(전리품 셔플 백)와 MFLG v1(마이그레이션 플래그)이 production load 경로와 golden roundtrip 테스트에서 같은 payload reader를 사용합니다.
- LSBG의 부분 복구·크기 상한·커서 보정과 MFLG의 truncation 처리를 유지했습니다.
- 시체 폭발과 소환 시체 폭발의 속도 제한·연쇄·처리 완료 상태를 `CombatRuntimeState` 소유로 이동하고 설정 재적용·로드·Revert 초기화를 한 경로로 통합했습니다.
- 저장 바이트, 레코드 버전, 시체 폭발 판정 순서와 발동 수치는 변경하지 않았습니다.

## 릴리스 재현성

- 공개 효과 문서 4종을 CMake 버전과 CHANGELOG 날짜에서 결정적으로 생성하고, 일반 CI와 태그 릴리스가 stale 문서를 게시 전에 거부합니다.
- 최종 DLL과 같은 링크에서 생성된 PDB는 공개 자산과 분리된 GitHub Actions 심볼 산출물로 90일간 보존합니다.
- 플레이어용 공개 릴리스 자산은 기존과 같이 DLL·ESP·MO2 ZIP·ZIP SHA256 네 개뿐입니다.

## 확인된 범위

- **인게임:** production 프로브의 함정 6종 표시, 곰덫을 포함한 표시물 자동 정리
- **자동 검증:** production 함정 정책과 cap, 6종 로그 집계, 코세이브 reader roundtrip, 공개 문서 drift, 릴리스 자산 분리 계약

로그의 엔진 수락 결과는 육안 가시성 자체를 증명하지 않으므로, 화면 표시는 위 인게임 확인을 별도 근거로 기록합니다. 적대 대상 최대 2명 적용, 동료·중립 NPC 비적용, 장시간 셀 전환·저장/불러오기 뒤 잔류 없음은 계속 회귀 확인 대상입니다.

## Compatibility / 설치

- 기존 세이브와 호환되며 새 게임이 필요하지 않습니다.
- rc1의 ESP FormID, 코세이브 wire format, 게임플레이 수치와 드랍 확률을 변경하지 않았습니다.
- MO2에서는 이전 버전에 **Merge가 아니라 Replace**로 설치하세요.

문제 보고 시 `Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log`를 함께 첨부해 주세요.
