# Calamity - Reactive Loot & Affixes v1.7.3-rc3

rc3는 **v1.7.3-rc2의 Prisma 패널 초기화 회귀를 수정한 교체 프리릴리스**입니다. rc2를 사용 중이라면 rc3로 교체해 주세요.

v1.7.3의 게임플레이 안정성 수정 3건(CTD 2건, 어픽스 발동이 영구히 멈추는 문제 1건)은 그대로 포함됩니다. 게임플레이 수치, 드랍 확률, 코세이브 형식, ESP 레코드와 공개 효과 계약은 rc2 대비 변경하지 않았습니다.

## rc2 대비 필수 수정

### Prisma 패널이 열리지만 동작하지 않는 문제

rc2에서 패널의 단일 HTML/CSS/JS를 여러 파일로 분리한 뒤, `state.js`가 아직 로드되지 않은 `tooltip.js`의 함수를 초기화 도중 호출했습니다. 단일 `<script>`에서는 뒤쪽 함수 선언이 미리 보이지만, 별도 classic `<script>` 파일 사이에는 그 호이스팅이 적용되지 않습니다.

그 결과 패널 DOM과 `CreateView` 로그는 성공해도 다음 초기화가 중단될 수 있었습니다.

- C++ ↔ JavaScript interop 등록
- 선택 아이템 툴팁과 룬워드 패널 데이터 갱신
- 버튼, 키보드, 드래그·리사이즈 이벤트 연결

`tooltip.js`가 필요한 함수를 먼저 등록하도록 실제 태그 순서를 고쳤습니다. 또한 테스트가 JS 파일을 다시 하나로 합치지 않고, HTML 순서대로 각 파일을 별도 classic script로 실행해 같은 회귀를 직접 잡도록 변경했습니다.

## 릴리스 안전성

- Papyrus 컴파일러가 없는 GitHub runner에서 검증된 `.pex`를 패키징하는 실제 E2E 두 건을 DLL 빌드 뒤 필수 실행합니다. DLL이 없거나 핀이 어긋나면 skip하지 않고 실패합니다.
- 같은 태그의 GitHub 릴리스가 이미 있으면 공개 자산을 덮어쓰지 않습니다. 대신 태그 commit, draft/prerelease 상태, DLL·ESP·MO2 ZIP·SHA256 sidecar의 이름과 실제 바이트 해시가 이번 빌드와 전부 같은지 확인합니다.
- 새 릴리스 생성은 원격 태그가 실제로 존재할 때만 진행합니다.

## 호환성

- 새 게임은 필요하지 않습니다.
- 코세이브 직렬화 형식, MCM 옵션, ESP 레코드, 어픽스 데이터와 드랍 확률은 변경하지 않았습니다.
- 패널 명령 ID, C++ ↔ JavaScript payload와 저장된 패널·툴팁 레이아웃 형식은 유지됩니다.

## 검증

- Chrome headless 실제 페이지 로드: JavaScript console error 0
- HTML 순서별 classic-script bootstrap 회귀 테스트: 통과
- 도구 워크플로 테스트: 124/124 통과
- Generator 테스트: 131/131 통과
- SKSE 정적 체크·런타임 게이트: 2/2 통과
- 별도 runtime-gate build: 1/1 통과
- 어픽스 스펙 린트: warning 0
- Papyrus compiler-less 패키징 E2E: 2/2 통과

## 인게임 확인 요청

1. F11 또는 MCM 버튼으로 패널을 열었을 때 현재 아이템·룬워드 데이터가 표시되는지
2. 아이템 선택을 바꾸면 툴팁과 패널 내용이 즉시 갱신되는지
3. 룬워드 베이스·레시피 선택, 변환·재련·상태 확인 버튼이 동작하는지
4. 패널 이동·크기 조절과 툴팁 위치·글자 크기 저장이 재시작 후 유지되는지
5. 발동형 무기를 장시간 사용해도 어픽스 발동이 멈추지 않는지
6. 함정 설치 후 지역을 벗어났다가 돌아오거나 여러 발동 어픽스를 함께 사용할 때 CTD가 없는지

정적·브라우저 검증은 완료했지만 Skyrim/PrismaUI 인게임 확인은 아직 완료하지 않았습니다. 문제가 발생하면 `Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log`와 크래시 로그를 함께 첨부해 주세요.
