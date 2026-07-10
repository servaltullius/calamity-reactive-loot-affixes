## Calamity Reactive Loot & Affixes v1.2.22

> [!WARNING]
> 이 버전은 다양한 모드 구성에서 실사용 피드백을 받기 위한 공개 테스트(Pre-release)입니다. 설치 전 현재 저장 파일(`.ess`와 동명 `.skse`)을 백업하거나 별도 저장 슬롯을 사용해 주세요. 기존 `v1.2.21`은 안정 릴리스로 유지됩니다.

`v1.2.22`는 아이템을 판매하거나 상자에 보관한 뒤 기존 어픽스 상태가 다른 아이템에 이어질 수 있던 문제를 막는 인스턴스 소유권 안전성 핫픽스입니다.

### 유저가 체감할 변화

- 판매, 상자 보관, 회수 중 바뀌는 아이템 UID를 실제 소유자 기준으로 추적합니다.
- 서로 다른 두 인스턴스의 상태가 같은 목적지 키에 충돌해도 어픽스를 합치지 않습니다.
- 룬워드 패널에 `Reset State / 상태 초기화` 버튼이 추가됩니다.
  - 장비를 착용하고 베이스로 선택한 뒤 버튼을 두 번 누르면 실행됩니다.
  - 선택 장비의 Calamity 어픽스, 룬워드 진행도, 런타임 상태, 프리뷰와 추가 표시 이름을 제거합니다.
  - 재련 오브와 룬 조각은 환불되지 않습니다.

### 기존 세이브 호환성

- v1.2.21 이하가 플레이어 인벤토리에 만든 legacy 키는 현재 아이템과 UID가 정확히 확인되고 UID가 유일할 때만 자동 변환됩니다. 같은 owner+UID에 애매한 기존 상태가 있으면 현재 아이템으로 누수되지 않도록 먼저 제거합니다.
- 이미 다른 아이템에 잘못 붙은 상태는 원래 소유자를 안전하게 추론할 수 없습니다. 잘못 표시된 장비를 선택한 뒤 `Reset State / 상태 초기화`를 사용해 주세요.
- 초기화 뒤 무료 자동 재롤이 되지 않도록 evaluated 표시는 유지됩니다.

### 개발·배포 안정성

- 생성 JSON 검증을 수정시각 비교에서 콘텐츠 비교로 바꿨습니다.
- GitHub Actions fresh checkout에서 xwin/CommonLibSSE-NG 외부 캐시를 준비하고 실제 SKSE DLL, static checks, runtime-gate를 빌드합니다.
- 인스턴스 이동 정책에 판매, 보관, 회수, legacy 변환, 중복 이벤트, 충돌 무병합 회귀 테스트를 추가했습니다.

### 검증

- 정적 정책 검사 및 runtime-gate
- Python tools 테스트와 affix compose/lint
- .NET generator 테스트
- Linux clang-cl SKSE DLL 빌드
- MO2 ZIP 구조 및 릴리스 검증

### 배포 파일

- `CalamityAffixes_MO2_v1.2.22_2026-07-10.zip`
- SHA256: `68d9da2e5dcc26c8e0176c0e98ea18d8b8130cae35c7e5d6efdaefc1323bf935`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.21...v1.2.22
