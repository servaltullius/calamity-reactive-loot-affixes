## Calamity Reactive Loot & Affixes v1.2.19-rc10 (Pre-release)

이번 RC는 전투 CTD 리스크를 낮추기 위한 **HealthDamage 훅 경로 안정화**와,
통화 드랍 체감 보정을 위한 **runeword/reforge 타입별 독립 롤 가드**를 함께 반영한 빌드입니다.

---

### 핵심 변경

1) **HandleHealthDamage 훅 경로 안정화**
- `skse/CalamityAffixes/src/Hooks.cpp`
- `skse/CalamityAffixes/include/CalamityAffixes/Hooks.h`
- `skse/CalamityAffixes/src/main.cpp`
- `HandleHealthDamage` vfunc 인덱스 정책을 명시화(`non-VR=0x104`, `VR=0x106`)하고 런타임 게이트 테스트를 추가했습니다.
- 훅 체인 충돌/포인터 오염 상황에서 원본 호출 포워딩과 히트데이터 정합성 검사를 강화해 크래시 가능성을 낮췄습니다.

2) **특수 액션 평가 경로 보호 강화**
- `skse/CalamityAffixes/src/EventBridge.Actions.Special.cpp`
- `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- `EvaluateConversion`/`EvaluateMindOverMatter`/`EvaluateCastOnCrit`에 `a_allowResync` 제어를 추가해 훅 경로와 이벤트 경로의 동기화 타이밍을 분리했습니다.
- 특수 액션 평가 함수에서 상태 잠금 일관성을 보강했습니다.

3) **활성화 통화 드랍의 타입별 독립 가드**
- `skse/CalamityAffixes/src/EventBridge.Triggers.Events.cpp`
- `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp`
- 기존의 “이미 통화/리스트가 있으면 전체 스킵”을 “runeword/reforge 타입별 독립 스킵”으로 변경했습니다.
- 한 타입이 이미 존재하더라도 다른 타입 롤은 수행되어 과도한 과소 드랍을 줄입니다.

4) **버전/패키징 정합성 업데이트**
- `skse/CalamityAffixes/CMakeLists.txt`
- 프로젝트 버전을 `1.2.19`로 정정해 패키지 버전 표기를 현재 릴리즈 라인과 맞췄습니다.

---

### 검증

- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (71/71)
- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null` PASS
- `tools/release_verify.sh --fast` PASS

---

### 배포 파일

- `CalamityAffixes_MO2_v1.2.19-rc10_2026-02-26.zip`
- SHA256: `a3d67c95c35587163f5933394c74d1d651b928efd3433c1493557eaa6ceb96dc`
