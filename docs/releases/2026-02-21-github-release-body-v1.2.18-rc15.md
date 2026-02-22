## Calamity Reactive Loot & Affixes v1.2.18-rc15 (Pre-release)

이번 RC는 **룬워드 효과 설명 가독성 개선**에 집중했습니다.

### 핵심 변경
- **룬워드 설명 UI 통일**
  - 룬워드 탭 미리보기, 패널 상태 영역, 레시피 리스트 hover 텍스트를
    어픽스 툴팁과 동일한 3줄 흐름(이름/효과 요약/상세 설명)으로 통일했습니다.
- **용어 정리 및 문구 개선**
  - 상세 설명에서 `프로파일` 표현을 제거했습니다.
  - 지나치게 축약된 문구를 풀어써, 전투 중에도 의미가 바로 읽히도록 조정했습니다.
  - ARPG 톤은 유지하되(짧고 명확), 효과 의도가 분명히 보이도록 다듬었습니다.
- **회귀 방지 가드 보강**
  - 런타임 게이트 테스트에 룬워드 툴팁 조합 함수/패널 반영 경로 존재 검증을 추가했습니다.

### 검증
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure -R CalamityAffixesRuntimeGateStoreChecks` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure -R CalamityAffixesStaticChecks` 통과

### 변경 파일
- `Data/PrismaUI/views/CalamityAffixes/index.html`
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`
