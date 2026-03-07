# Runeword Layout Rebalance

## Goal
- Runeword 탭을 `Base / Recipe / Transmute` 적층 화면이 아니라 `Context / Explorer / Review / Action` 작업 화면으로 재구성한다.
- 레시피 탐색기를 화면의 절대적인 메인 영역으로 승격하고, 베이스와 실행은 보조 레일로 축소한다.
- 실제 인게임 패널 높이가 낮아도 레시피 목록과 실행 컨트롤이 동시에 보이도록 정보 밀도를 다시 맞춘다.

## Non-goals
- Runeword 로직, 명령 계약, 데이터 구조 변경
- Affix/Advanced 탭 의미 변경
- 룬워드 효과 산식이나 큐브 규칙 변경

## Affected files
- `Data/PrismaUI/views/CalamityAffixes/index.html`
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Constraints
- 인게임 오버레이 특성을 고려해 좁은 폭에서도 레이아웃이 무너지지 않아야 한다.
- 기존 진행 상태와 CTA 상태 표현은 유지하되, 과한 세로 적층은 제거한다.
- 구조 회귀 테스트는 시각적 의도를 최소한의 마커로 계속 감시해야 한다.

## Milestones
1. 상단을 `compact context bar + progress strip`으로 축소
2. 본문을 `base rail / recipe explorer / review-action inspector` 3열 workbench로 재배치
3. 현재 베이스/선택 레시피를 context chip으로 분리하고, 본문 카드의 반복 요약을 줄인다
4. 우측 inspector는 상태/버튼/큐브/상세 미리보기를 compact stack으로 재정렬
5. UX 정책 테스트를 새 workbench 마커 기준으로 갱신

## Validation
1. `python3 tools/compose_affixes.py --check && python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json && python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null && python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null`
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / rollback
- 3열 workbench가 좁은 폭에서 답답해질 수 있어 반응형 붕괴 순서를 carefully 잡아야 한다.
- 좌/우 레일을 너무 줄이면 베이스 선택과 실행 맥락이 오히려 약해질 수 있다.
- context chip과 inspector 요약이 중복되면 정보 구조가 다시 무거워질 수 있다.
