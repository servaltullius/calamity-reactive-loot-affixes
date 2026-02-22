## Calamity Reactive Loot & Affixes v1.2.18-rc19 (Pre-release)

rc18에서 CoC(썬더볼트/체라 등) 프로크의 “히트 이펙트 아트(ARTO) 잔상 고착”을 막기 위해 `noHitEffectArt=true`로 안전장치를 넣었습니다.
다만 그 결과, **프로크가 시각적으로 덜 보이는** 문제가 생길 수 있어 이번 rc19에서는 **안전한 대체 VFX**를 추가했습니다.

### 핵심 변경
- **CoC 프로크(히트 이펙트 아트 억제 시)에도 시각적 피드백 제공**
  - `noHitEffectArt=true`인 CoC 프로크가 발생하면 대상에게 **짧은 Hit Shader(원소별: 화염/냉기/번개)**를 재생합니다.
  - 목적: “월드에 남는 히트 아트”는 막으면서도, 발동 체감(재미/가독성)을 유지합니다.

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc19 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (54/54)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc19_2026-02-22.zip`

