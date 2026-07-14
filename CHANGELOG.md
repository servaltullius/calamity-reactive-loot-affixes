# Changelog

이 프로젝트의 주요 변경사항은 이 파일에 기록합니다.

형식은 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)를 따르고,
버전 표기는 [Semantic Versioning](https://semver.org/spec/v2.0.0.html) 기반으로 관리합니다.

## [Unreleased]

### Added

- 룬 조각/재련 오브 피티 카운터와 시체 FormID·날짜·카테고리 처리 mask를 저장하는 추가 코세이브 레코드 `CCRT v1`을 도입했습니다.
- 플레이어/player-owned summon·proxy 적대 처치, 제외 대상, 카테고리별 전환 중복 방지와 corpse-only 소스 계약을 고정하는 런타임 회귀 검증을 추가했습니다.
- Prisma 레시피의 한국어/영어 상세 payload, viewport clamp, wheel·scrollbar·키보드 스크롤 및 ARIA 상태를 검증하는 UI 계약을 추가했습니다.

### Changed

- 룬 조각과 재련 오브의 실제 판정 권한을 SKSE `TESDeathEvent` 기반 **eligible hostile corpse-only** 경로로 일원화했습니다. 성공한 통화는 월드에 생성하지 않고 사망한 적의 인벤토리에 조용히 직접 추가합니다.
- 일반 상자/컨테이너 활성화, 아이템 픽업, 월드 `PlaceAtMe`, 플레이어 인벤토리 직행과 새 SPID 통화 분배를 비활성화했습니다. `loot.currencyDropMode=hybrid`는 구 설정 호환 토큰으로만 유지합니다.
- 피해자가 팔로워/동료, 소환·지휘 액터, 아동, player-owned 또는 비적대 대상인 경우와 환경 오브젝트·player-owned가 아닌 독립 NPC/팔로워의 처치를 통화 판정에서 제외했습니다.
- `Data/CalamityAffixes_DISTR.ini`를 빈 호환 산출물로 전환하고 SPID를 권장 의존성에서 제거했습니다.
- 룬워드 레시피의 효과·권장 베이스·룬 순서·상세를 선택 언어에 맞춰 검색 결과, 선택 상태와 hover에서 일관되게 표시하도록 Prisma payload를 확장했습니다.
- Prisma 패널을 viewport 안으로 제한하고 레시피 탐색기의 네이티브 scrollbar·키보드·ARIA 상호작용을 보강했습니다.
- Dragon의 화염/냉기/전격 저항을 Chains of Honor와 공유하던 MGEF에서 분리해 전용 append-only MGEF 3개를 사용하도록 변경했습니다.
- 생성기가 정확한 **94개 룬워드 + 33개 룬 가중치** 계약, 중복 ID와 생성 레코드 충돌에서 즉시 실패하도록 검증을 강화했습니다.
- MO2 패키징이 방금 빌드한 DLL과 실제 Papyrus 컴파일 결과만 받도록 하고, ZIP의 정확한 PEX 3종·`runtime_contract.json`·버전·해시가 맞지 않으면 릴리스 검증이 실패하도록 변경했습니다.

### Fixed

- 전투와 무관한 일반 상자/컨테이너에서 통화가 판정되거나, 월드에 갑자기 튀어나와 보일 수 있던 부자연스러운 드랍 경로를 제거했습니다.
- 같은 시체에 death event가 다시 전달되거나 저장을 다시 불러왔을 때 룬 조각/재련 오브 판정이 중복될 수 있던 경로를 카테고리 ledger로 차단했습니다.
- 레시피 상세가 선택 언어나 표시 위치에 따라 축약·누락되고, 패널이 화면 밖으로 벗어나거나 스크롤 입력 상태가 보조기술에 전달되지 않던 UI 계약을 보완했습니다.

### Compatibility

- 기본 룬 조각 `8%`, 재련 오브 `5%`, 룬 가중치 `El-Amn=4 / Sol-Um=3 / Mal-Lo=2 / Sur-Zod=1`과 99회 룬 무드롭 피티는 유지합니다. 피티는 이제 `CCRT`에 저장됩니다.
- 업데이트 전에 SPID로 통화 또는 해당 레벨드 리스트가 들어간 전환 시체는 카테고리별로 감지해 같은 카테고리만 건너뜁니다.
- 기존 어픽스·룬워드 ID와 생성 레코드 순서를 유지하고 Dragon 전용 MGEF 3개만 append-only 영역에 추가합니다.
- 기존 장비, 보유 룬 조각·재련 오브, 완성 룬워드와 기존 코세이브 직렬화 레코드는 그대로 로드됩니다. `CCRT`는 추가 레코드이므로 새 게임이 필요하지 않습니다.

## [1.3.3] - 2026-07-14

v1.3.3은 v1.3.2의 패널 성능 개선을 유지하면서, 레시피 탐색기의 커스텀 wheel RAF가 네이티브 scrollbar 조작과 경쟁하던 회귀를 바로잡는 공개 테스트 hotfix입니다. 실제 게임 테스트에서 수정 후 스크롤이 부드러워진 것을 확인했습니다.

### Added

- 실제 index.html의 스크롤 controller를 추출해 가짜 CEF DOM, 정수형 scrollTop과 RAF queue에서 실행하는 Node 동작 회귀 테스트를 추가했습니다.
- 마우스 휠 위·아래, legacy deltaY=0, 다중 노치 clamp, line/page mode, 트랙패드, 즉시·지연 thumb 이동과 pointer/mouse 취소를 고정하는 계약을 추가했습니다.

### Changed

- 명확한 120단위 일반 마우스 휠 노치만 한 노치당 64px 목표로 정규화하고, 트랙패드의 작은 연속 pixel delta는 그대로 유지합니다.
- scroll 감쇠 시간상수를 72ms에서 48ms로 줄이고, 0.75px 도달 거리와 0.5px 최소 유효 이동량으로 정수형 CEF에서도 RAF가 확실히 종료되도록 변경했습니다.
- 커스텀 frame-paced wheel 처리를 명시적으로 지정된 레시피 목록에만 한정해 다른 패널 스크롤 영역의 네이티브 동작을 보존합니다.

### Fixed

- 레시피 탐색기의 진행 중 wheel RAF가 scrollbar thumb·track·키보드 또는 외부 scrollTop 이동을 이전 목표 위치로 되감던 문제를 수정했습니다.
- 자체 scroll 이벤트 뒤 native scroll 통지가 한 RAF 늦게 도착하는 경우에도 다음 RAF가 실제 위치를 덮어쓰기 전에 취소·동기화하도록 수정했습니다.
- 일부 CEF 입력 경로에서 일반 마우스 휠이 작은 pixel delta로 전달되어 스크롤이 지나치게 느리던 문제를 수정했습니다.
- 정수 단위로 양자화되는 scrollTop에서 마지막 1px 부근의 RAF가 종료되지 않을 수 있던 문제를 수정했습니다.

### Compatibility

- Prisma UI 프레임워크와 v1.3.2의 Tick 병합, 정적 레시피 분리, keyed DOM, View 복구 및 MCM 안전 폴백을 유지합니다.
- 어픽스·룬워드 효과 정의, 룬 가중치와 분포, 기본 파편 드롭률 8%, 99회 무드롭 피티는 변경하지 않았습니다.
- 기존 장비, 파편, 완성 룬워드와 저장·직렬화 형식은 변경하지 않았으며 새 게임은 필요하지 않습니다.

## [1.3.2] - 2026-07-13

`v1.3.2`는 Prisma UI 프레임워크와 v1.3.1의 View 복구 경로를 유지하면서, 패널을 연 뒤 반복되던 메인 스레드 작업과 거친 입력 처리를 줄여 조작감과 프레임 일관성을 개선하는 공개 테스트 패치입니다.

### Added

- worker Tick 병합, 정적 레시피 카탈로그 분리, 중복 interop 억제와 시간 기반 스크롤 동작을 고정하는 Prisma 패널 성능 회귀 검증을 추가했습니다.

### Changed

- worker가 예약하거나 실행 중인 Prisma Tick을 최대 하나로 제한해, 이전 Tick이 밀린 동안 같은 작업이 SKSE task queue에 중복으로 쌓이지 않도록 변경했습니다.
- 94개 룬워드의 정적 레시피 카탈로그를 주기 갱신에서 분리하고, 선택 항목과 보유 파편·진행 상태 같은 동적 데이터만 필요한 시점에 갱신하도록 변경했습니다.
- 이미 적용된 패널 open 상태와 내용이 같은 interop payload는 다시 보내지 않도록 상태 동기화 경로를 정리했습니다.
- 레시피 목록은 keyed DOM 재사용과 검색용 텍스트 캐시를 사용해, 선택이나 검색이 바뀔 때 전체 항목을 불필요하게 다시 생성하는 비용을 줄였습니다.
- 휠 입력은 고정 픽셀 즉시 점프 대신 delta와 경과 시간을 제한한 시간 기반 스크롤로 처리합니다.
- 패널 상태 강조는 무한 `box-shadow` 애니메이션과 `offsetWidth` 강제 reflow 대신 짧은 compositor 친화적 전환과 `requestAnimationFrame` 단위 갱신을 사용합니다.

### Fixed

- 패널이 열린 동안 300ms 주기 작업이 밀리면 동일 Tick이 계속 예약되어 입력 반응과 프레임이 더 둔해질 수 있던 문제를 수정했습니다.
- 내용이 바뀌지 않았는데도 정적 레시피 설명과 JSON payload를 반복 생성할 수 있던 문제를 수정했습니다.
- 레시피 스크롤이 작은 휠 입력에도 큰 간격으로 즉시 이동해 딱딱하게 느껴지던 문제를 완화했습니다.
- 상태 전환 애니메이션이 동기 레이아웃 계산과 넓은 그림자 repaint를 유발할 수 있던 경로를 제거했습니다.

### Compatibility

- Prisma UI는 전체 툴팁과 룬워드·재련 패널을 위한 기본 프레임워크로 계속 사용하며, v1.3.1의 API/View/DOM-ready 복구와 MCM 안전 폴백을 유지합니다.
- 어픽스 및 룬워드 효과 정의, 룬 가중치와 분포, 기본 파편 드롭률 `8%`, 99회 무드롭 피티는 변경하지 않았습니다.
- 기존 장비, 보유 파편, 재련 오브와 완성된 룬워드는 그대로 유지됩니다.
- 세이브 및 어픽스·룬워드 직렬화 형식은 변경하지 않았으며 새 게임은 필요하지 않습니다.

## [1.3.1] - 2026-07-13

`v1.3.1`은 Prisma UI 프레임워크를 유지하면서 View 생성·DOM 준비·재시도 수명주기를 안정화하고, Prisma를 사용할 수 없을 때도 상태 확인과 안전한 통화 복구를 제공하는 공개 테스트 패치입니다.

### Added

- MCM에 Prisma UI 상태 확인, 룬워드 상태 표시와 누락 통화 복구 기능을 추가했습니다.
- Prisma View identity, 오래된 DOM-ready 콜백, timeout 복구와 MCM 이벤트 연결을 고정하는 회귀 테스트를 추가했습니다.

### Changed

- Prisma API 및 View 생성 실패 시 5초 간격으로 재시도하고, DOM-ready가 15초 동안 도착하지 않으면 해당 View를 폐기한 뒤 복구하도록 변경했습니다.
- Prisma 상태 확인은 정상 View를 강제로 파괴하지 않는 비파괴 검사로 동작합니다.
- 새 View가 생성되면 툴팁, 언어, 레이아웃, 룬워드 목록, 레시피와 패널 캐시를 완전히 갱신합니다.
- 패널 열기 요청은 View 로딩 중에도 보존하며, 실제 DOM-ready 이후에만 패널이 열린 것으로 처리합니다.
- 사용을 종료한 Vibekit 도구와 남아 있던 릴리스 검증 호출을 제거했습니다.

### Fixed

- `CreateView()`가 invalid handle을 반환했는데도 View API를 호출할 수 있던 문제를 수정했습니다.
- invalid nonzero View가 남아 이후 복구를 영구적으로 막을 수 있던 문제를 수정했습니다.
- 이전 View의 늦은 DOM-ready 콜백이 새 View의 준비 상태를 덮어쓸 수 있던 문제를 수정했습니다.
- Prisma 콜백과 활성 View 상태 사이의 동시 접근 및 동기 콜백 손실 가능성을 수정했습니다.
- Prisma가 없거나 View 생성에 실패했는데도 패널이 열렸다는 성공 HUD가 표시되던 문제를 수정했습니다.

### Compatibility

- Prisma UI는 전체 툴팁과 룬워드·재련 패널을 위한 기본 프레임워크로 계속 사용합니다.
- MCM 폴백은 상태 확인과 안전 복구만 제공하며 전체 Prisma UI를 복제하지 않습니다.
- 세이브 및 어픽스·룬워드 직렬화 형식, 효과 정의, 룬 분포와 드롭률은 변경하지 않았습니다.
- 누락 통화 복구는 세이브당 한 번만 동작하며 장비, 어픽스 또는 완성 룬워드를 초기화하지 않습니다.

## [1.3.0] - 2026-07-13

`v1.3.0`은 어픽스와 룬워드의 역할 중복을 줄이고, 각 효과가 실제 빌드 선택으로 구분되도록 발동 구조·접미 중첩·장비 유형·런타임 정확성을 함께 재정비한 공개 테스트 프리릴리즈입니다.

### Added

- 한손, 양손, 활·석궁을 구분하는 무기 하위 유형 계약을 추가해 Swordsman, Champion, Marksman, Eagle Eye 계열이 의미 있는 무기군에 생성되도록 했습니다.
- Strength, Edge, Zephyr, Oath, Myth, Unbending Will에 권장 룬워드 베이스 경고를 추가했습니다. 경고는 비차단 방식이며 플레이어가 원하면 그대로 변환할 수 있습니다.
- Plague, Breath of the Dying 및 일반 시체 폭발이 함께 존재할 때 고유 효과가 영구적으로 가려지지 않도록 시체 폭발 선택 우선순위를 추가했습니다.
- 동일 접미 계열 최고 티어 선택, 무기 하위 유형, 시체 폭발 우선순위와 새 룬워드 의미 계약을 고정하는 회귀 테스트를 추가했습니다.

### Changed

- Destruction, Hand of Justice, Dragon, Mist, Famine, Beast, Eternity, Enigma, Last Wish, Plague, Pride, Mosaic, Obsession 등 주요 룬워드를 단순 수치 변형에서 고유한 복합 발동 효과로 개편했습니다.
- Chaos, Breath of the Dying, Rhyme, Faith, Call to Arms, Honor, Hustle-A를 포함한 기존 룬워드의 시그니처 동작과 설명을 강화했습니다.
- Shadow Stride, Nourishing Flame, Mana Knot, Shock Weakness, Death Mark, Ice Form의 발동 조건과 전투 역할을 구분했습니다.
- 같은 접미 계열을 여러 부위에 장착하면 합산하지 않고 가장 높은 티어 하나만 적용하도록 변경했습니다.
- 접미 22개 계열의 실제 T1/T2/T3 선택 가중치를 `6/3/1`로 통일했습니다.
- Assassin 접미 툴팁에 치명타 확률뿐 아니라 실제 적용되는 치명타 피해 증가도 표시합니다.
- 프리픽스 티어 계열과 원소 피해 주입 계열은 한 아이템에서 같은 패밀리가 중복 선택되지 않도록 변경했습니다.

### Fixed

- hostile Magic Effect의 잘못된 수치 부호, 회복 플래그, Actor Value 연결과 시체 체력 비율 단위를 수정했습니다.
- 모드 효과를 껐다 켰을 때 발동 효과와 접미 패시브가 정확히 중단·복원되도록 런타임 상태 동기화를 보강했습니다.
- 같은 타격이 여러 이벤트 경로에서 중복 처리될 수 있던 문제를 줄이고 함정·룬워드 재변환 경로를 보완했습니다.
- 구버전 저장에 남은 접미 Ability를 실제 Spell 보유 상태와 대조해 불필요한 패시브를 제거하고 누락된 패시브를 복원합니다.
- Plague 시체 폭발이 Death Pyre 계열의 높은 기본 피해 때문에 선택 단계에서 항상 사라지던 문제를 수정했습니다.
- Generator의 전방 Magic Effect 참조와 일부 룬워드의 잘못된 Magic Effect 연결을 제거했습니다.

### Balance

- 공개 프리픽스 73개, 접미 22계열 66개, 룬워드 94개의 역할을 재검토했으며 정규화된 공개 효과 중 완전히 동일한 의미의 중복은 남기지 않았습니다.
- 공격속도, 이동·은신, 방어, 자원 재생, 원소 공격처럼 선택지가 많은 역할군은 새 일반 효과를 더 추가하지 않고 공개 테스트 피드백을 우선합니다.
- `v1.2.25`의 룬 파편 `4/3/2/1` 가중치, 기본 파편 드롭 판정 `8%`, 99회 무드롭 피티는 유지됩니다.

### Compatibility

- 세이브 및 어픽스·룬워드 직렬화 형식은 변경하지 않았습니다.
- 기존 어픽스 ID, 룬워드 ID, 생성 레코드 순서와 FormID 배치를 유지합니다.
- 기존 장비와 완성된 룬워드는 그대로 유지되며, 같은 ID의 개편된 효과 정의를 사용합니다.
- 무기 하위 유형 제한은 업데이트 후 새로 생성하거나 재련하는 장비에 적용되며 기존 장비를 강제로 재추첨하지 않습니다.
- 구버전 접미 패시브는 다음 런타임 동기화에서 현재 장비 상태에 맞게 자동 정리됩니다.

## [1.2.25] - 2026-07-12

`v1.2.25`는 공개 피드백에서 확인된 룬 파편 편중을 완화하고, 룬워드 성능 차이에 비해 과도했던 고급 룬 병목을 현실적인 범위로 줄이는 룬 경제 밸런스 프리릴리즈입니다.

### Added

- 룬 33종의 정확한 4단계 가중치, 4:1 상·하한 비율, SPID 94티켓 산출물과 런타임 계약을 고정하는 회귀 테스트를 추가했습니다.

### Changed

- 룬 가중치를 저급 `El-Amn=4`, 중급 `Sol-Um=3`, 고급 `Mal-Lo=2`, 최상급 `Sur-Zod=1`의 네 단계로 재조정했습니다.
- SKSE 상자/컨테이너 선택과 SPID 시체 레벨드 리스트가 같은 4:1 분포를 사용하도록 SSOT, 생성기 폴백과 DLL 폴백을 일치시켰습니다.
- 첫 10종 룬의 파편 내 점유율을 약 78.25%에서 42.55%로 낮추고, 최상급 룬 1종의 비중을 약 0.02~0.06%에서 약 1.064%로 조정했습니다.
- 누락 룬 폴백을 새 최상급 가중치 `1`로 낮춰 계약 누락이 오히려 최고 빈도로 역전되지 않도록 했습니다.

### Fixed

- Windows dotnet 래퍼를 사용하는 WSL 패키징에서 `/mnt/c` staging 경로가 잘못된 WSL UNC로 전달되어 Generator가 실패하던 문제를 Windows 경로 변환으로 수정했습니다.

### Balance

- 독립 가중 추첨·교환 없음 기준 94개 레시피의 완성 기대 파편 격차는 약 563배에서 8.64배로 줄어듭니다.
- 고급 룬워드는 크게 접근하기 쉬워지는 대신, 가장 쉬운 저급 레시피는 기존보다 약 2.55배 느려질 수 있습니다.
- 기본 파편 드롭 판정 `8%`와 99회 무드롭 피티는 변경하지 않았습니다.

### Compatibility

- 세이브·어픽스·룬워드 직렬화 형식은 변경하지 않았으며, 이미 보유한 룬 파편과 완성 룬워드는 그대로 유지됩니다.
- 새 분포는 업데이트 후 새로 선택되는 파편에 적용됩니다. 이미 SPID로 분배된 액터/시체 인벤토리는 즉시 다시 추첨되지 않습니다.

## [1.2.24] - 2026-07-11

`v1.2.24`는 무기 거치대가 사용하는 플레이어 인벤토리 → 월드 오브젝트 경로에서 어픽스 상태가 플레이어 UID에 남아, 이후 다른 창고에서 꺼낸 무기에 잘못 이어질 수 있던 문제를 막는 상태 소유권 핫픽스입니다.

### Added

- 드롭된 월드 참조 전용 상태 키, 동일 참조 재획득 검증, 핸들 재사용 차단, 중복 드롭 기록 갱신 및 기존 세이브의 orphan player-key 정리를 고정하는 회귀 테스트를 추가했습니다.

### Changed

- 플레이어가 드롭한 장비의 상태를 재사용 가능한 플레이어 UID 공간에서 제거하고, 실제 월드 참조 FormID에 임시 귀속합니다.
- 같은 월드 참조를 다시 주울 때에만 해당 상태를 새 플레이어 UID로 복원합니다.
- 월드 참조 삭제 대기열이 참조별 상태 키를 함께 보존하며, 세이브 로드·되돌리기 시 이전 세이브의 대기 항목을 초기화합니다.
- 모드를 다시 활성화할 때와 세이브를 로드할 때 실제 인벤토리에 없는 플레이어 소유 상태를 fail-closed 방식으로 정리합니다.

### Fixed

- 어픽스 무기 A를 무기 거치대에 건 뒤 다른 창고에서 꺼낸 무기 B가 A의 이전 UID를 재사용하면 A의 어픽스와 런타임 상태가 B에 보이던 문제를 수정했습니다.
- 저장·로드 뒤 드롭 추적 캐시가 초기화된 경우에도 동일 월드 참조를 통해 원래 장비 상태를 복원할 수 있도록 수정했습니다.
- 삭제된 월드 참조의 어픽스, 런타임 상태, 룬워드 상태, 프리뷰·평가 마커가 일부 남을 수 있던 정리 경로를 통합했습니다.
- 오래된 참조 핸들이 다른 월드 오브젝트에 재사용됐을 때 상태를 잘못 이전하거나 삭제할 수 있던 경로를 차단했습니다.

### Compatibility

- 세이브 및 어픽스 직렬화 레코드 버전은 변경하지 않았습니다.
- `v1.2.23`에서 무기를 거치대에 둔 채 아직 다른 아이템이 오염되지 않은 세이브는 로드 시 남은 플레이어 UID 상태를 안전하게 폐기합니다. 원본 거치대 무기의 상태는 증명할 방법이 없어 보존되지 않을 수 있습니다.
- 이미 다른 아이템 B에 A의 상태가 나타난 세이브는 자동으로 A/B를 구분할 수 없습니다. 해당 아이템에 `Reset State / 상태 초기화`를 사용하거나 문제가 발생하기 전 세이브에서 다시 시작해야 합니다.
- `v1.2.23`의 해상도별 툴팁 배율과 기존 설정 호환성은 그대로 유지됩니다.

## [1.2.23] - 2026-07-11

`v1.2.23`은 4K에서 확인된 기존 어픽스 툴팁 외관을 유지하면서 QHD, FHD, 울트라와이드 해상도에서 화면 비율에 맞게 크기와 기본 위치를 조정하는 UI 호환성 업데이트입니다.

### Added

- 4K, QHD, FHD, UWQHD의 툴팁 배율·기본 위치와 모바일 분기, 수동 글자 크기 저장 경로를 고정하는 회귀 테스트를 추가했습니다.

### Changed

- 툴팁 자동 배율 기준을 `3840x2160`의 기존 `2.5x` 외관으로 명시하고, 뷰포트의 가로·세로 중 작은 비율을 사용하도록 변경했습니다.
- 저장된 사용자 배치가 없을 때 툴팁 기본 `right/top` 위치도 같은 4K 기준으로 비례 조정합니다.

### Fixed

- QHD와 FHD에서 어픽스 툴팁이 4K보다 화면을 더 크게 차지하던 문제를 수정했습니다.
- `3440x1440` 같은 울트라와이드에서 가로 폭만 따라 툴팁이 과도하게 커질 수 있는 문제를 높이 기준 비율로 제한했습니다.

### Compatibility

- 세이브 및 어픽스 런타임 저장 형식은 변경하지 않았습니다.
- 기존 `prisma_tooltip_layout.json`의 수동 `right/top` 위치와 글자 크기는 그대로 유지됩니다. 현재 해상도의 새 기본 위치를 적용하려면 해당 해상도에서 툴팁 UI를 초기화해야 합니다.
- 폭 `900px` 이하 분기와 기존 글자 크기 조절 범위 `70%~180%`는 유지됩니다.

## [1.2.22] - 2026-07-10

`v1.2.22`는 아이템 판매·보관 중 ExtraUniqueID 소유권이 바뀔 때 인스턴스 상태가 끊기거나 다른 아이템으로 이어질 수 있던 문제를 막는 안전성 핫픽스입니다.

### Added

- 룬워드 패널에 선택 장비의 Calamity 어픽스, 룬워드 진행도, 런타임 상태, 프리뷰와 표시 이름을 제거하는 `Reset State / 상태 초기화` 동작을 추가했습니다.
- 실수 방지를 위해 상태 초기화는 6초 안에 두 번 눌러야 하며, 소모한 재련 오브와 룬 조각은 환불되지 않습니다.
- 판매·상자 보관·회수 및 목적지 키 충돌을 owner+UID 기준으로 검증하는 회귀 테스트를 추가했습니다.

### Changed

- `ExtraUniqueID` 인스턴스 키 의미를 SKSE 원본 계약인 `(owner FormID, UID)`로 명확히 통일했습니다.
- 구버전이 `(item FormID, UID)`로 만든 플레이어 인벤토리 키는 UID가 유일할 때만 로드 후 변환합니다. 같은 owner+UID에 애매한 기존 상태가 있으면 먼저 fail-closed로 제거해 현재 아이템에 잘못 붙지 않게 합니다.
- 생성물 최신성 검사는 파일 수정시각 대신 실제 JSON 콘텐츠를 비교하도록 변경했습니다.
- GitHub Actions가 fresh checkout에서도 xwin과 CommonLibSSE-NG를 외부 캐시에 준비한 뒤 실제 DLL/runtime-gate를 빌드하도록 보강했습니다.

### Fixed

- `TESUniqueIDChangeEvent`의 아이템 FormID를 플레이어 소유자 ID로 오인해 판매·상자 이동 이벤트를 놓치던 문제를 수정했습니다.
- UID 변경 목적지에 기존 상태가 있을 때 두 아이템의 어픽스 토큰을 합치던 동작을 제거했습니다. 소유자를 증명할 수 없는 충돌 상태는 양쪽 모두 fail-closed로 제거해 다른 아이템으로 이어지지 않게 합니다.
- 플레이어가 직접 생성한 `ExtraUniqueID`에 아이템 FormID를 기록하던 문제를 수정해 실제 소유자 FormID를 기록합니다.

### Compatibility

- 이미 다른 아이템에 잘못 붙어 버린 legacy 상태는 원래 소유자를 안전하게 추론할 수 없습니다. 해당 장비를 선택하고 `Reset State / 상태 초기화`를 사용해야 합니다.
- 상태 초기화 뒤 자동 어픽스 재롤을 막기 위해 evaluated 표시는 유지됩니다.

## [1.2.21] - 2026-03-22

`v1.2.21`은 `v1.2.20` 이후 확인된 룬워드 워크벤치 스크롤 UX 문제를 정리한 유지보수 릴리즈입니다.

### Changed

- Prisma 룬워드 레시피 목록이 전역 inertia 스크롤을 타지 않고, 목록 전용 amplified wheel scroll 경로를 사용하도록 조정했습니다.
- CEF가 작은 휠 델타를 줄 때도 의미 있게 내려가도록 최소 스크롤 step을 추가해, 휠을 과도하게 많이 굴려야 하던 문제를 완화했습니다.

### Fixed

- 룬워드 레시피 목록이 무겁게 끊기거나, 반대로 너무 조금씩만 내려가서 긴 목록 탐색이 불편하던 문제를 수정했습니다.

## [1.2.20] - 2026-03-22

상세한 RC 흐름은 [v1.2.20 RC 통합 노트](docs/releases/2026-03-22-v1.2.20-rc02-rc23-consolidated-notes.md)를 참고하세요.

### Added

- 룬워드 효과군을 스카이림 마법 학파 기반으로 재설계하고, 겹치던 효과를 고유 효과로 바꿔 빌드 선택지가 더 분명해졌습니다.
- Utility prefix 3종을 전투용 효과로 복귀시키고, suffix 패밀리를 재편해 장비 파밍 결과가 더 뚜렷하게 갈리도록 정리했습니다.
- Bloom 계열 트랩에 `planted / burst` 피드백과 생성 실패 진단을 추가해, 심김과 실제 발동을 인게임에서 바로 확인할 수 있게 했습니다.

### Changed

- 룬워드 워크벤치 UI를 재설계해 레시피 프리뷰 기본 오픈, 스크롤 복구, 레이아웃 밀도 조정, 패널 드래그/배치 개선을 한 사이클에 묶었습니다.
- 아이템 설명과 공개 문서를 대규모로 손봐, 한국어 설명이 더 구체적으로 보이고 내부용 정의는 플레이어 문서에서 숨기도록 정리했습니다.
- 트리거, 룬워드, 직렬화, Prisma 패널 관련 내부 구조를 분해해 이후 기능 수정과 검증이 쉬운 방향으로 정리했습니다.
- 특수 액션 계약을 강화해 잘못된 `runtime.trigger`나 암묵적인 `0 = always-on` 보정을 제거했고, 디버그 HUD와 verbose 로그도 분리했습니다.

### Fixed

- 플레이어 피격 시 `TESHitEvent` fallback 재진입으로 이어지던 전투 프리징을 수정하고, `HandleHealthDamage` hook 기반의 안정적인 incoming hit routing으로 되돌렸습니다.
- SE/AE 공용 CommonLibSSE 타깃이 깨져 Address Library 초기화가 실패하던 릴리즈 빌드 문제를 수정했습니다.
- `ConvertDamage`의 원소별 동시 적용, `AttackDamageMult`/`WeaponSpeedMult` magnitude 단위, 어픽스 개수 reroll, 방어구 suffix 드롭 풀 누락 같은 데이터/런타임 버그를 정리했습니다.
- 프로젝트 경로 이동 뒤 build tree가 깨지던 문제, generator/runtime contract sync 불안정, MO2 ZIP 패키징과 Papyrus compile 검증 경로를 보강해 배포 안정성을 높였습니다.

## [1.2.20-rc23] - 2026-03-09

### Added

- Bloom 계열 트랩에 `planted / burst` 진단 HUD와 생성 실패 사유 알림을 추가해, 트랩이 심어졌는지와 실제 발동했는지를 인게임에서 바로 확인할 수 있게 했습니다.

### Changed

- 특수 액션(`CastOnCrit`, `ConvertDamage`, `MindOverMatter`, `Archmage`, `CorpseExplosion`, `SummonCorpseExplosion`)은 이제 유효한 `runtime.trigger`와 명시적 `runtime.procChancePercent > 0`이 없으면 보정 없이 실패하도록 계약을 강화했습니다.
- 기존 특수 액션의 `procChancePercent: 0.0` 데이터는 모두 의미 보존 형태의 `100.0`으로 마이그레이션했습니다.
- 공개 문서 생성 경로에서 INTERNAL 항목을 기본 숨김 처리해, 플레이어용 카탈로그와 prefix 문서가 내부 companion spell 정의를 직접 드러내지 않도록 정리했습니다.
- `debug notifications` 설정을 `debugHudNotifications`와 `debugVerboseLogging`으로 분리하고, legacy `debugNotifications`는 호환용 입력으로만 읽도록 바꿨습니다.

### Fixed

- debug HUD/log 분리 과정에서 룬워드 안내, 복구 알림, 설정 변경 피드백 같은 일반 HUD 메시지까지 숨겨지던 회귀를 수정했습니다.
- release/build 검증 스크립트가 `dotnet` 실행 경로 문제로 테스트 커버리지를 잃지 않도록 `CAFF_DOTNET` 주입 경로와 fake-`dotnet` 기반 테스트를 추가했습니다.

## [1.2.20-rc22] - 2026-03-08

### Fixed

- 플레이어 피격 시 `TESHitEvent` fallback incoming trigger가 proc 후속 이벤트를 다시 먹으며 프리징으로 이어질 수 있던 경로를 재진입 guard로 차단했습니다.
- `PlayerCharacter HandleHealthDamage` hook를 기본 경로로 사용하도록 바꿔, 플레이어 incoming hit 처리가 불안정한 fallback 경로에 과도하게 의존하던 문제를 줄였습니다.
- 예전 `allowPlayerHealthDamageHook` 런타임 override는 무시하고 항상 플레이어 피격 hook를 활성화하도록 정리했습니다.

## [1.2.20-rc21] - 2026-03-08

### Fixed

- 릴리즈 빌드가 `CommonLibSSE-NG`의 `install.linux-clangcl-rel-se`를 참조하던 문제를 `install.linux-clangcl-rel` flatrim(SE/AE 공용) 기준으로 수정했습니다.
- `ensure_skse_build.py`가 `CMAKE_PREFIX_PATH`까지 검사해, 잘못된 SE 캐시를 유지한 채 패키징하는 상황을 막도록 보강했습니다.

## [1.2.20-rc20] - 2026-03-08

### Changed

- Prisma control panel 드래그 중 위치 갱신을 `requestAnimationFrame + transform` 기반으로 정리해 레이아웃 흔들림과 잦은 style write를 줄였습니다.

### Fixed

- 대형 Prisma 패널을 끌 때 드래그가 무겁거나 미세하게 튀는 체감 문제를 완화했습니다.
- 패널 드래그/리사이즈/복원 경로가 서로 다른 `left/top/right/bottom` 갱신 방식을 섞던 부분을 공통 anchored-position helper로 맞췄습니다.

## [1.2.20-rc19] - 2026-03-08

### Added

- 프로젝트 경로 이동 뒤에도 `ensure_skse_build.py`와 no-space run-root helper가 stale build tree를 자동 복구하도록 정리했습니다.
- `HealthDamage` guard 조합을 fake 이벤트 시퀀스로 직접 검증하는 runtime-gate 테스트를 추가했습니다.
- `Transmute` fragment consume/rollback helper와 `Reforge` regular-slot reroll helper를 분리해 동작 기반 검증 지점을 늘렸습니다.

### Changed

- SKSE 플러그인 버전 메타데이터가 CMake 생성 버전을 단일 진실원으로 사용하도록 정리했습니다.
- `EventBridge`의 serialization load, trigger runtime, runeword selection/transmute/reforge 흐름을 helper 중심으로 더 잘게 분해했습니다.
- `PrismaTooltip` 명령/상태 처리를 controller와 view-state helper 기준으로 재배치했습니다.

### Fixed

- 프로젝트 경로 변경 뒤 `ctest`/release verification이 예전 절대경로에 묶여 깨지던 문제를 줄였습니다.
- `HealthDamage` stale-window, per-target repeat, low-health snapshot 검증이 문자열 검색에 과도하게 의존하던 상태를 보강했습니다.
- 룬워드 재련 시 보존해야 하는 runeword token과 regular affix reroll 비교 경로를 분리해 회귀 위험을 낮췄습니다.

[Unreleased]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.3.3...HEAD
[1.3.3]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.3.2...v1.3.3
[1.3.2]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.3.1...v1.3.2
[1.3.1]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.3.0...v1.3.1
[1.3.0]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.25...v1.3.0
[1.2.25]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.24...v1.2.25
[1.2.24]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.23...v1.2.24
[1.2.23]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.22...v1.2.23
[1.2.22]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.21...v1.2.22
[1.2.21]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20...v1.2.21
[1.2.20]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.19.2...v1.2.20
[1.2.20-rc23]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc22...v1.2.20-rc23
[1.2.20-rc22]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc21...v1.2.20-rc22
[1.2.20-rc21]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc20...v1.2.20-rc21
[1.2.20-rc20]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc19...v1.2.20-rc20
[1.2.20-rc19]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc18...v1.2.20-rc19
