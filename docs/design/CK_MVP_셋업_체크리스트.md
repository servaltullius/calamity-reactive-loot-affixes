아래 체크리스트는 “**CK에서 최소 MVP 플러그인(ESP/ESL)**”을 만들어서, 현재 레포의 `Data/` 스테이징 템플릿(Papyrus/MCM/KID/SPID + SKSE 이벤트 브리지)을 실제 게임에서 **바로 검증**할 수 있게 하는 절차입니다.

> 권장 전제: 플러그인 이름을 **`CalamityAffixes.esp`**로 맞춥니다. (MCM Helper `modName`과 일치 필요)  
> 레포 스켈레톤: `README.md` 참고

---

## 0) 사전 준비(한 번만)

- [ ] Skyrim SE/AE용 Creation Kit 설치/실행 가능
- [ ] 테스트용 모드매니저(MO2 등)에 이 레포의 `Data/`를 “모드 루트”로 설치해둠
- [ ] 게임 실행은 SKSE로 함 (SKSE 플러그인 기반 이벤트를 쓰므로)
- [ ] (MCM 사용 시) SkyUI + MCM Helper 설치

### Papyrus 컴파일 준비
- [ ] `Data/Scripts/Source/`에 아래 `.psc`가 존재함
  - `Data/Scripts/Source/CalamityAffixes_AffixManager.psc`
  - `Data/Scripts/Source/CalamityAffixes_PlayerAlias.psc`
  - `Data/Scripts/Source/CalamityAffixes_SkseBridge.psc`
  - `Data/Scripts/Source/CalamityAffixes_MCMConfig.psc` (MCM Helper)

---

## 1) CK에서 새 플러그인 생성

- [ ] CK → `File > Data...`
- [ ] `Skyrim.esm`(필수) 체크 후 로드
- [ ] `Active File`로 새 플러그인 생성 후 저장: **`CalamityAffixes.esp`**

---

## 2) MVP용 폼(Keyword / FormList / Test Item) 만들기

### 2.1 Keyword 만들기

최소 2개를 만듭니다.

- [ ] (필수) DoT 태그 키워드: `CAFF_TAG_DOT` (KYWD)
- [ ] (필수) 샘플 어픽스 키워드: `MA_AFFIX_ONHIT_TestBurst` (KYWD)

> 어픽스 키워드는 “아이템에 붙는 메타데이터”입니다. 실제 발동 로직은 중앙 매니저에서 처리합니다.

### 2.2 FormList 만들기(어픽스 키워드 리스트)

- [ ] FormList(FLST) 생성: `CalamityAffixes_AffixKeywordList`
- [ ] 리스트에 `MA_AFFIX_ONHIT_TestBurst` 추가

중요:
- [ ] **리스트 순서가 곧 affix id(인덱스)** 입니다. (나중에 어픽스가 늘면 순서 관리가 곧 호환성입니다.)

### 2.3 테스트 아이템 만들기(어픽스 키워드가 달린 장비)

가장 쉬운 방식: 반지/아뮬렛 등 ARMO를 복제해서 키워드를 추가합니다.

- [ ] 기존 반지(예: Gold Ring류)를 Duplicate
- [ ] 새 EditorID: `CalamityAffixes_TestRing`
- [ ] 이름(Name): `Calamity Test Ring` 등으로 알아보기 쉽게
- [ ] Keywords 목록에 `MA_AFFIX_ONHIT_TestBurst` 추가

---

## 3) 샘플 ProcSpell 준비(최소 1개)

`CalamityAffixes_AffixEffectBase`는 “프로크 시 플레이어가 대상에게 `ProcSpell`을 캐스팅”합니다.

MVP는 단순히 **바닐라 스펠을 재사용**해도 됩니다.

- [ ] (빠른 테스트) `ProcSpell`로 바닐라 `Fireball` 같은 눈에 띄는 스펠을 선택 (권장)
  - 이후 밸런스 단계에서 전용 스펠로 교체

---

## 4) AffixEffect(어픽스 정의) 퀘스트 1개 만들기

현재 템플릿은 “어픽스 정의를 Quest + Quest Script 인스턴스”로 둡니다. MVP는 1개만 만들어도 충분합니다.

- [ ] 새 Quest(QUST) 생성: `CalamityAffixes_Affix_TestBurst`
- [ ] `Start Game Enabled` 체크 (MVP에서는 안전하게 켜두는 걸 권장)
- [ ] Quest Scripts에 `CalamityAffixes_AffixEffectBase` 추가
- [ ] 스크립트 프로퍼티 설정
  - [ ] `AffixKeyword` = `MA_AFFIX_ONHIT_TestBurst`
  - [ ] `ProcChancePct` = `100.0` (첫 테스트는 100% 권장)
  - [ ] `ICDSeconds` = `1.5`
  - [ ] `TriggerOnOutgoingHit` = `true`
  - [ ] `TriggerOnIncomingHit` = `false`
  - [ ] `ProcSpell` = (3)에서 정한 스펠

---

## 5) Runtime Quest(중앙 매니저 + SKSE 브리지) 만들기

- [ ] 새 Quest(QUST) 생성: `CalamityAffixes_Runtime`
- [ ] `Start Game Enabled` 체크

### 5.1 Quest Scripts 부착

- [ ] Quest Scripts에 `CalamityAffixes_AffixManager` 추가
- [ ] Quest Scripts에 `CalamityAffixes_SkseBridge` 추가

> CK 주의: 퀘스트 생성 후 **반드시 OK로 폼 생성 완료**한 다음 스크립트를 붙이세요(크래시 예방).  
> (MCM Helper 문서에서도 동일한 주의가 언급됨)

### 5.2 `CalamityAffixes_AffixManager` 프로퍼티 채우기

- [ ] `PlayerRef` = PlayerRef(기본 플레이어 Actor)
- [ ] `AffixKeywordList` = `CalamityAffixes_AffixKeywordList`
- [ ] `AffixEffects` 배열 길이를 1로 만들고, `[0]`에 `CalamityAffixes_Affix_TestBurst` 지정
- [ ] `TestItem` = `CalamityAffixes_TestRing`

### 5.3 `CalamityAffixes_SkseBridge` 프로퍼티 채우기

- [ ] `Manager` = `CalamityAffixes_Runtime` (해당 퀘스트의 매니저 스크립트 인스턴스)

### 5.4 Player Alias(장비 이벤트/피격 이벤트용) 만들기

- [ ] Quest Aliases 탭 → 새 Reference Alias 생성: `PlayerAlias_Runtime`
- [ ] Fill Type: Player(또는 Specific Reference로 PlayerRef)
- [ ] Alias Scripts에 `CalamityAffixes_PlayerAlias` 추가
- [ ] Alias Script 프로퍼티: `Manager` = `CalamityAffixes_Runtime`

---

## 6) MCM Quest(MCM Helper) 만들기 (권장)

MCM Helper는 “config.json만으로 끝”이 아니라, **MCM_ConfigBase를 상속한 config 스크립트가 붙은 퀘스트**가 필요합니다. (문서: “creating a config script” 참고)

- [ ] 새 Quest(QUST) 생성: `CalamityAffixes_MCM`
- [ ] `Start Game Enabled` 체크
- [ ] Quest Scripts에 `CalamityAffixes_MCMConfig` 추가
- [ ] `CalamityAffixes_MCMConfig` 프로퍼티: `Manager` = `CalamityAffixes_Runtime`

### 6.1 MCM Helper “로드 이벤트”용 PlayerAlias 추가

- [ ] Quest Aliases 탭 → 새 Reference Alias 생성: `PlayerAlias_MCM`
- [ ] Fill Type: Specific Reference (Cell: Any, Ref: PlayerRef)
- [ ] Alias Scripts에 **`SKI_PlayerLoadGameAlias`** 추가 (SkyUI 제공)

---

## 7) (선택) DoT 테스트 세팅

SKSE 이벤트 싱크는 `TESMagicEffectApplyEvent`를 이용해 DoT를 “틱”이 아닌 “적용/갱신”으로만 취급합니다.  
필터는 `CAFF_TAG_DOT` 키워드가 붙은 MGEF만 통과합니다.

- [ ] DoT용 Magic Effect(MGEF) 하나를 만들거나 복제해서 Keywords에 `CAFF_TAG_DOT` 추가
- [ ] 해당 MGEF를 적용하는 테스트 Spell(SPEL) 생성
- [ ] 게임에서 해당 주문을 적에게 적용했을 때, `CalamityAffixes_DotApply` 이벤트가 발생(브리지 통해 매니저로 전달)하는지 확인

---

## 8) 빠른 게임 내 검증 루트

### 8.1 먼저 “프로크 파이프라인”부터 확인 (100% 확률 추천)

- [ ] `CalamityAffixes.dll`을 빌드해 `Data/SKSE/Plugins/CalamityAffixes.dll`에 설치
- [ ] SKSE로 게임 실행 후 콘솔에 `CalamityAffixes: EventBridge registered.`가 찍히는지 확인
- [ ] `Calamity Test Ring` 착용
- [ ] 적을 1회 타격 → 대상에게 `ProcSpell`이 발동하는지 확인

### 8.2 문제 생길 때 체크 포인트(우선순위)

- [ ] 플러그인 이름이 `CalamityAffixes.esp`인지 (MCM Helper `modName`과 불일치 시 MCM이 안 뜸)
- [ ] `CalamityAffixes_Runtime`가 Start Game Enabled인지
- [ ] `AffixKeywordList`(FormList)와 `AffixEffects` 배열 길이/순서가 맞는지
- [ ] 테스트 아이템(ARMO)에 어픽스 키워드가 들어갔는지
- [ ] 첫 테스트는 `ProcChancePct=100`으로 “발동 자체”를 먼저 확인했는지
