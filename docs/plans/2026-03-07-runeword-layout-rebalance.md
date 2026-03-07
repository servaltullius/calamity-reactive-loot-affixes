# Runeword Layout Rebalance

## Goal
- Runeword 탭에서 레시피 탐색을 가장 중요한 메인 작업으로 보이게 만든다.
- 변환 영역은 과도한 시각 점유를 줄이고, 요약/실행 패널 역할로 축소한다.
- 현재 단계형 흐름은 유지하되 실제 사용 순서에 맞게 레이아웃 우선순위를 다시 조정한다.

## Non-goals
- Runeword 로직, 명령 계약, 데이터 구조 변경
- Affix/Advanced 탭 의미 변경
- 새 기능 추가

## Affected files
- `Data/PrismaUI/views/CalamityAffixes/index.html`
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Constraints
- 인게임 오버레이 특성을 고려해 좁은 폭에서도 레이아웃이 무너지지 않아야 한다.
- 기존 단계 진행 상태와 CTA 상태 표현은 유지한다.
- 구조 회귀 테스트는 시각적 의도를 최소한의 마커로 계속 감시해야 한다.

## Milestones
1. Runeword 레이아웃을 `레시피 메인 + 변환 요약 사이드` 구조로 재배치
2. 베이스 선택을 compact selector로 축소
3. 변환 패널의 큐브/상태/버튼 밀도를 낮춰 compact summary로 조정
4. UX 정책 테스트를 새 구조 기준으로 보강

## Validation
1. `python3 tools/compose_affixes.py --check && python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json && python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null && python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null`
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / rollback
- 레시피 영역을 키우는 과정에서 모바일/좁은 폭 레이아웃이 깨질 수 있다.
- 변환 패널이 너무 축소되면 상태 정보 가독성이 다시 떨어질 수 있어, 요약 문구와 큐브 가독성을 함께 확인해야 한다.
