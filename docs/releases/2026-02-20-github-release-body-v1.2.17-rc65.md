## Calamity Reactive Loot & Affixes v1.2.17-rc65 (Pre-release)

이번 RC는 **SPID 시체 드랍 경로의 룬워드 조각 분포를 가중치 기반으로 개선**한 업데이트입니다.

### 핵심 변경
- **SPID 시체 드랍의 룬 조각 분포 개선 (중점)**
  - `CAFF_LItem_RunewordFragmentDrops` 구성 방식을 단순 균등에서 **룬 티어 가중치 기반**으로 변경
  - 저티어 룬은 더 자주, 고티어 룬은 더 희귀하게 나오도록 조정
  - 기존 런타임 가중치 철학과 시체(SPID) 드랍 체감을 맞춤
- **드랍 확률 자체는 유지**
  - 룬워드 조각 `16%`
  - 재련 오브 `10%`

### 구현 메모
- 가중치 소스는 기존 공유 런워드 계약(`RunewordContractCatalog`)을 사용
- leveled list 엔트리 티켓 수를 가중치 비율로 구성(캡 적용)
- 오토루팅 대응 범위 확장은 이번 RC에 포함하지 않음

### 테스트/검증
- Generator unit tests: PASS (49/49)
- `tools/release_verify.sh --fast`: PASS
- `CAFF_PACKAGE_VERSION=1.2.17-rc65 tools/build_mo2_zip.sh`: PASS

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc65_2026-02-20.zip`
