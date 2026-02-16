# **스카이림 엔진의 ARPG 장르적 변환: 바닐라 에셋의 기술적 재해석과 구현 방법론**

## **1\. 서론: 베데스다 크리에이션 엔진과 핵 앤 슬래시 장르의 융합 가능성 분석**

베데스다 게임 스튜디오(Bethesda Game Studios)의 대표작인 엘더스크롤 V: 스카이림(The Elder Scrolls V: Skyrim)은 본질적으로 1인칭 및 3인칭 시점의 몰입형 시뮬레이션(Immersive Sim)과 샌드박스 RPG를 지향하여 설계되었다. 이는 디아블로(Diablo) 시리즈나 패스 오브 엑자일(Path of Exile, PoE)이 추구하는 쿼터뷰(Quarter-view) 기반의 핵 앤 슬래시(Hack and Slash) 장르와는 근본적인 엔진 아키텍처와 게임플레이 루프에서 큰 차이를 보인다. 크리에이션 엔진(Creation Engine)은 물리 엔진인 하복(Havok)을 기반으로 한 객체 상호작용과 복잡한 AI 스케줄링에 최적화되어 있는 반면, ARPG 장르는 대규모 몬스터의 빠른 처치, 즉각적인 반응성, 그리고 직관적인 전리품 수집 시스템을 중시한다. 이러한 구조적 간극에도 불구하고, 스카이림의 모딩 커뮤니티는 지난 10년 이상 엔진의 한계를 시험하며 장르적 변환을 시도해왔다.

본 보고서는 스카이림의 바닐라 에셋(Vanilla Assets)—즉, 외부의 고용량 모델링이나 텍스처를 추가하지 않고 게임 내에 이미 존재하는 리소스—을 창의적으로 재조합하고 스크립트 엔진인 파피루스(Papyrus)와 외부 도구인 NifSkope를 활용하여 ARPG 특유의 게임플레이 경험을 구현하는 기술적 방법론을 심층적으로 분석한다. 특히 '챔피언 몬스터 시스템', '전리품 시각화(Loot Beam)', '발동형 스킬(Trigger Skills)', '토템 및 장판 메커니즘' 등 디아블로와 PoE의 핵심 요소를 스카이림 엔진 내에서 어떻게 역공학(Reverse Engineering)적으로 구현할 수 있는지에 대해 상세히 기술한다. 이는 단순한 모드 추천을 넘어, 엔진의 작동 원리를 이해하고 자원을 효율적으로 관리하는 전문적인 레벨 디자인 및 시스템 기획의 영역을 포괄한다.

## **2\. 시각적 피드백의 재구성: 엘리트 몬스터와 챔피언 시스템의 구현**

ARPG의 전투 경험을 결정짓는 핵심 요소 중 하나는 일반 몬스터(White Mob)와 명확히 구분되는 정예(Elite), 챔피언(Champion), 희귀(Rare) 몬스터의 존재이다. 디아블로 시리즈는 몬스터에게 '빠름', '화염 사슬', '포격' 등의 속성을 부여하고 이를 시각적으로 즉각 식별 가능한 색상(Color Grading)과 셰이더(Shader)로 표현한다. 스카이림 바닐라 데이터에는 이러한 강화된 적을 표현하기에 적합한 미사용 혹은 특정 퀘스트 전용 시각 효과들이 다수 존재하며, 이를 NPC에게 동적으로 적용함으로써 별도의 모델링 수정 없이도 강력한 시각적 위계를 구축할 수 있다.

### **2.1 드래곤 형상(Dragon Aspect) 셰이더의 구조적 분리 및 NPC 적용**

'드래곤본(Dragonborn)' DLC에 포함된 '드래곤 형상(Dragon Aspect)' 포효는 시각적으로 매우 강렬한 오라(Aura) 효과와 함께 고대 노르드어 문양이 몬스터의 몸을 감싸는 연출을 제공한다. 이는 디아블로 3의 정예 몬스터가 두르는 버프 효과나 PoE의 '네메시스(Nemesis)' 모드와 유사한 시각적 밀도를 가진다. 기술적으로 이 효과는 크게 두 가지 요소, 즉 갑옷(Armor) 형태의 메쉬와 스크립트로 제어되는 시각 효과(Visual Effect) 및 셰이더(EffectShader)로 구성된다.1

드래곤 형상은 기본적으로 플레이어 전용으로 설계되어 있어 NPC에게 직접 적용할 경우 의도치 않은 모델링 겹침 현상이 발생할 수 있다. 이를 해결하기 위해 모더는 크리에이션 키트(Creation Kit) 상에서 VisualEffect 프로퍼티와 EffectShader를 분리하여 호출하는 방식을 채택해야 한다. 바닐라 스크립트 구조를 분석해보면, 드래곤 형상 효과는 특정 슬롯에 장착되는 '방어구 아머 애드온(ArmorAddon)'과 그 위에 덧씌워지는 '멤브레인 셰이더(Membrane Shader)'로 나뉜다. 여기서 물리적인 방어구 형태를 제외하고, 셰이더 효과만을 추출하여 별도의 MagicEffect로 패키징할 수 있다.

창의적인 응용 방법으로는 바닐라의 DragonAspectEffect 셰이더가 가진 고유의 붉은색과 주황색 펄스(Pulse)를 복제하여 RGB 값을 조정하는 것이다. NifSkope를 사용하지 않고도 크리에이션 키트 내의 EffectShader 설정 창에서 'Membrane Color'와 'Particle Color' 값을 수정함으로써, 냉기 속성 엘리트(푸른색), 독 속성 엘리트(녹색), 전격 속성 엘리트(보라색) 등의 변형을 텍스처 수정 없이 구현할 수 있다. 이는 텍스처 메모리를 추가로 점유하지 않으면서도 다양한 속성의 챔피언 몬스터를 시각적으로 구별해주는 효율적인 최적화 기법이다.3

더 나아가 스크립트 로직을 통해 NPC의 등급을 판별하는 시스템을 구축할 수 있다. OnInit 또는 OnLoad 이벤트에서 NPC의 레벨이나 키워드(Keyword)를 스캔하고, 해당 등급에 맞는 셰이더를 VisualEffect.Play(Reference) 함수로 호출함으로써, 별도의 장비 착용 없이 시각적 오라만을 씌우는 것이 가능하다. 이 방식은 장비 교체 스크립트보다 연산 부하가 적어 다수의 몬스터가 등장하는 ARPG 모드 환경에서 프레임 드랍을 최소화하는 데 기여한다.4

### **2.2 소븐가르드(Sovngarde) 발광 효과와 멤브레인 셰이더의 활용**

메인 퀘스트의 소븐가르드 지역에서 영웅 NPC들에게 적용되는 황금빛 후광 효과(FXSovengardeGlow)는 매우 저비용 고효율의 시각 자원이다. 이 효과는 FXSovengardeSCRIPT라는 전용 스크립트에 의해 제어되는데, 기술적으로는 모델의 외곽선을 따라 부드러운 빛을 생성하는 멤브레인 셰이더와 입자를 방출하는 파티클 셰이더의 결합이다.6

이 셰이더는 디아블로 3의 '희귀(Rare)' 몬스터나 '무적 하수인(Invulnerable Minions)' 속성을 가진 적을 표현하기에 최적이다. 예를 들어, 특정 몬스터가 '물리 공격 반사' 또는 '피해 면역' 상태일 때 이 황금빛 셰이더를 일시적으로 적용함으로써 플레이어에게 공격을 중지하거나 다른 전략을 사용해야 함을 직관적으로 알리는 시각적 신호(Visual Cue)로 활용할 수 있다. 바닐라 스크립트를 참조하여 OnEffectStart에서 셰이더를 Play()하고, OnEffectFinish에서 Stop() 하는 간단한 로직만으로도 전투의 가독성을 비약적으로 높일 수 있다. 특히 소븐가르드 셰이더는 캐릭터의 실루엣을 강조하므로, 어두운 던전이나 밤 시간대의 전투에서도 엘리트 몬스터의 위치를 명확히 식별하게 해준다.

### **2.3 위습마더(Wispmother) 및 뱀파이어 로드 이펙트의 응용**

스카이림의 크리처 중 위습마더와 뱀파이어 로드는 독특한 시각 효과를 보유하고 있다. 위습마더가 사용하는 '쉐이드(Shade)' 소환 효과나 뱀파이어 로드의 변신 및 이동 이펙트는 '환영(Illusion)' 속성이나 '빠른 이동(Teleport/Dash)' 속성을 가진 엘리트 몬스터를 표현하는 데 유용하다.

위습마더의 스크립트 구조를 살펴보면, 복제된 액터를 생성하고 투명도(Alpha)와 파티클을 조절하여 본체와 분신을 구분하기 어렵게 만드는 메커니즘을 사용한다. 이를 응용하여 PoE의 'Fractured(분열)' 맵 모드처럼 적이 사망 시 작은 분신들로 나뉘거나, 공격 시 잔상(Motion Blur)을 남기며 이동하는 'Fast(빠름)' 속성의 몬스터를 구현할 수 있다. 바닐라 데이터인 WispMotherSmoke 셰이더를 일반 산적이나 드라우그에게 적용하면, 마치 유령처럼 흐릿하고 타격 판정이 모호해 보이는 효과를 낼 수 있어 '회피율 증가' 속성을 시각화하는 데 적합하다.7

| 시각 효과 (Source) | 추천 ARPG 속성 (Attribute) | 기술적 구현 (Implementation) |
| :---- | :---- | :---- |
| **Dragon Aspect** (Red) | 공격력 강화 / 버서커 (Berserker) | EffectShader의 RGB 값을 (1.0, 0.2, 0.2)로 조정하여 적용 |
| **Dragon Aspect** (Blue) | 냉기 강화 / 빙결 (Frozen) | RGB 값을 (0.2, 0.2, 1.0)으로 조정, 파티클 속도 감소 |
| **Sovngarde Glow** (Gold) | 피해 반사 / 무적 (Invulnerable) | FXSovengardeSCRIPT를 참조하여 OnHit 시 발동하도록 수정 |
| **Wisp Smoke** (White) | 회피 / 유령 (Ghostly) | 투명도(Alpha) 0.7 설정 및 WispSmoke 파티클 부착 |
| **Spriggan Swarm** (Green) | 체력 재생 / 생명력 흡수 (Regen) | 스프리건의 치유 이펙트를 상시 적용 (Loop 설정) |

## **3\. 전리품 시스템의 시각화: 루트 빔(Loot Beams)과 청각적 피드백**

핵 앤 슬래시 장르의 가장 큰 보상 심리는 화면을 가득 채우는 전리품과 그 중에서 가치 있는 아이템을 즉각적으로 알리는 '기둥(Beam)' 효과에서 온다. 스카이림은 기본적으로 아이템 드롭 시 물리 엔진에 의해 바닥에 흩뿌려지는 방식을 취하며, 작은 반지나 보석은 시각적으로 놓치기 쉽다. 이를 극복하기 위해 바닐라 스카이림의 퀘스트 마커 및 특수 효과를 재활용하여 직관적인 루팅 경험을 설계해야 한다.

### **3.1 메리디아의 빛(Meridia’s Light)을 활용한 전설 아이템 기둥**

퀘스트 '여명(The Break of Dawn)'에서 사용되는 메리디아의 비콘(Meridia’s Beacon) 빛줄기는 스카이림 내에서 가장 ARPG스러운 수직 광원 효과를 제공한다.9 이 빛줄기는 매우 멀리서도 식별 가능하며, 신성하고 강력한 느낌을 주기 때문에 전설(Legendary) 등급 아이템 드롭 효과로 가장 적합하다.

기술적 구현을 위해서는 먼저 Meshes\\Lob\\LobBeam01.nif 혹은 던전 입구 등에서 사용되는 LightBeam 관련 NIF 파일을 추출하여 별도의 Static 오브젝트나 Activator로 등록해야 한다. 이후 스크립트를 통해 다음과 같은 로직을 수행한다.

1. **아이템 생성 감지**: 엘리트 몬스터가 사망(OnDeath)하거나 상자에서 아이템이 생성되는 순간, 스크립트는 해당 아이템의 등급(키워드 또는 FormID 리스트 기반)을 검사한다.  
2. **빔 오브젝트 소환**: 아이템이 전설 등급일 경우, PlaceAtMe 함수를 사용하여 아이템의 좌표(X, Y, Z)에 빔 이펙트 오브젝트를 소환한다. 이때 Z축 좌표를 약간 조정하여 빔이 아이템 위로 정확히 솟구치도록 한다.  
3. **동적 연결 및 제거**: 소환된 빔 오브젝트는 아이템의 레퍼런스(Reference)와 연결되어야 한다. OnContainerChanged 이벤트나 OnItemRemoved 이벤트를 모니터링하여, 플레이어가 해당 아이템을 인벤토리에 넣는 순간 빔 오브젝트를 Delete() 또는 Disable() 처리해야 시각적 정합성을 유지할 수 있다.

NifSkope의 BSLightingShaderProperty에서 Emissive Color 속성을 수정하면, 동일한 빔 메쉬를 사용하더라도 유니크 아이템(금색), 세트 아이템(초록색), 희귀 아이템(노란색) 등 등급별로 다른 색상을 표현할 수 있다. 이는 외부 텍스처 없이 바닐라 메쉬의 속성값만 변경하는 것으로 가능하므로 모드의 용량을 최소화한다.12

### **3.2 영혼 흡수(Soul Trap) 및 넌루트(Nirnroot) 효과의 청각적 활용**

시각적 효과뿐만 아니라 청각적 피드백(Audio Feedback) 또한 중요하다. PoE의 'Exalted Orb' 드롭 사운드처럼, 가치 있는 아이템은 고유의 소리를 내야 한다.

* **영혼 흡수 이펙트 (SoulAbsorbVFX)**: 드래곤 영혼 흡수 시 발생하는 휘몰아치는 에너지 효과는 매우 화려하고 역동적이다. 이를 보스 몬스터 처치 시 대량의 경험치 획득이나 퀘스트 아이템 드롭 시각 효과로 짧게(afTime 조절) 재생할 수 있다.14  
* **넌루트 사운드 루프**: 붉은 넌루트나 일반 넌루트가 가진 Glow 효과와 특유의 윙윙거리는 사운드 루프(Sound Loop)는 드롭된 아이템이 주변에 있음을 청각적으로 알리는 데 매우 유용하다. 이 사운드 디스크립터(Sound Descriptor)를 전리품 빔 오브젝트에 연결하면, 플레이어는 화면 밖에서도 귀중한 아이템이 떨어졌음을 인지할 수 있다. 이는 시각 정보와 청각 정보를 동시에 제공하는 완벽한 루트 필터(Loot Filter) 경험을 제공한다.

## **4\. 전투 메커니즘의 확장: 투사체 병합과 발동형 스킬 시스템**

스카이림의 전투는 기본적으로 묵직하고 느린 템포를 가지고 있다. 이를 PoE 스타일의 빠르고 화려한 광역 전투(AoE)로 변환하기 위해서는 투사체 시스템의 개조와 조건부 발동 스킬의 구현이 필수적이다.

### **4.1 NifSkope를 활용한 다중 투사체(Multi-Projectile) 병합 기술**

크리에이션 키트(CK)의 데이터 구조상, 하나의 MagicEffect는 단 하나의 Projectile 레코드만 가질 수 있다. 즉, 바닐라 상태에서는 '다중 투사체 보조 젬(GMP)'처럼 한 번의 시전으로 여러 발의 화염구를 부채꼴로 발사하는 기능을 구현하기 어렵다. 스크립트로 Cast를 여러 번 호출하는 방법은 연산 부하가 크고 발사 타이밍이 어긋날 수 있다. 이에 대한 해결책은 NifSkope를 이용한 메쉬 레벨의 병합(Merge)이다.15

* **메쉬 병합 프로세스**:  
  1. FireboltProjectile.nif, IceSpikeProjectile.nif, LightningBoltProjectile.nif 등 원하는 투사체 파일들을 NifSkope에서 연다.  
  2. 하나의 메인 파일(부모 노드 NiNode) 아래에 다른 투사체들의 NiTriShape 블록을 복사하여 붙여넣는다.  
  3. 각 투사체 블록의 Translation (X, Y, Z 좌표) 값을 조정하여 중앙에서 좌우로 약간씩 이격시킨다.  
  4. 이렇게 병합된 NIF 파일을 새로운 Projectile 레코드로 등록한다.  
* **충돌 처리와 폭발(Explosion) 로직**: 시각적으로는 여러 개의 투사체가 날아가는 것처럼 보이지만, 게임 엔진은 이를 하나의 충돌체(Collision Box)로 인식할 가능성이 높다. 따라서 투사체가 적중했을 때의 효과를 극대화하기 위해 Explosion 타입을 '범위형(Radius)'으로 설정해야 한다. 폭발 반경 내에 화염, 냉기, 전격 데미지를 동시에 입히도록 Enchantment를 구성하면, 단일 투사체 판정이지만 실제 효과는 광역 마법과 동일하게 작동한다. 이는 '트라이 엘리먼트 볼트'와 같은 복합 속성 스킬을 바닐라 자원만으로 구현하는 핵심 기술이다.15

### **4.2 스크립트를 통한 '치명타 시 시전(Cast on Crit)' 메커니즘**

PoE의 꽃이라 불리는 'CoC(Cast on Crit)' 메커니즘은 스카이림의 파피루스 스크립트로 정교하게 구현할 수 있다.17

* **이벤트 기반 트리거**: 스카이림의 OnHit 이벤트나 퍽(Perk)의 Entry Point 중 'Apply Combat Hit Spell' 기능을 활용한다. 플레이어의 공격이 치명타(Critical Hit)로 판정되면, 스크립트가 연결된 주문을 즉시 발동시킨다.  
* **가상 시전(Virtual Casting)과 XMarker**: 플레이어가 직접 주문을 시전하는 애니메이션(Cast 모션)을 취하면 근접 공격의 흐름이 끊기게 된다. 이를 방지하기 위해 Instant 타입의 마법이나, 투사체만 생성하는 '더미 스펠(Dummy Spell)'을 활용해야 한다. 투사체의 발사 방향을 제어하기 위해, 스크립트는 플레이어의 시선 각도(GetAngleZ)를 계산하여 전방 일정 거리(예: 500 유닛) 앞에 투명한 XMarker를 배치한다(MoveTo). 그리고 Spell.RemoteCast(Player, Player, XMarker) 함수를 사용하여 플레이어 위치에서 마커를 향해 마법이 발사되도록 한다. 이를 통해 플레이어는 검을 휘두르는 동시에 마법이 발사되는 매끄러운 전투 경험을 할 수 있다.20  
* **내부 쿨다운(Internal Cooldown) 관리**: 초당 공격 횟수(APS)가 높은 무기를 사용할 경우, 스크립트가 과도하게 실행되어 스택 덤프(Stack Dump)나 렉을 유발할 수 있다. 이를 방지하기 위해 PoE처럼 0.15초\~0.5초의 내부 쿨다운 변수를 설정하고, RegisterForSingleUpdate나 Utility.Wait 대신 시간 차이(Utility.GetCurrentRealTime())를 비교하는 로직을 사용하여 최적화를 수행해야 한다.18

## **5\. 전장의 지배: 토템(Totem) 및 장판(Hazard) 시스템의 구현**

디아블로의 히드라(Hydra)나 PoE의 쇠뇌(Ballista) 토템, 그리고 다양한 장판(Ground Effect) 스킬은 스카이림의 Hazard 레코드와 'Invisible Actor' 기법을 통해 구현된다. 이 두 방식은 각각의 장단점이 뚜렷하므로 용도에 맞게 선택해야 한다.21

### **5.1 Hazard 레코드의 활용과 한계**

스카이림의 Hazard 오브젝트(예: 화염 룬, 독 웅덩이, 화염의 벽)는 특정 범위 내의 대상에게 주기적으로 마법 효과를 적용하는 데 최적화된 엔진 기본 기능이다.

* **장점**: 엔진 레벨에서 처리되므로 스크립트 부하가 매우 적고 안정적이다. 다수의 장판을 깔아도 프레임 저하가 적다.  
* **단점**: Hazard는 기술적으로 액터(Actor)가 아니다. 따라서 OnEffectStart 이벤트에서 akCaster로 인식되지 않아, 시전자의 공격력에 비례한 데미지 계산이나 복잡한 조건부 로직을 적용하기 어렵다.  
* **창의적 활용**: 바닐라의 Wall of Flames 셰이더 색상을 변경하여 '저주받은 대지(Cursed Ground)'를 만들 수 있다. 데미지보다는 적의 이동 속도를 늦추거나 마법 저항력을 깎는 디버프(Debuff) 장판으로 활용하는 것이 효과적이다. 이는 PoE의 'Desecrated Ground'와 유사한 전술적 요소를 제공한다.

### **5.2 투명 액터(Invisible Actor)를 활용한 고급 토템 AI**

단순한 장판이 아니라, 주변 적을 자동으로 감지하고 조준 사격하는 '포탑(Turret)'이나 '토템'을 구현하려면 Hazard 대신 움직이지 않는 소환수(Stationary Summon) 방식을 사용해야 한다.21

* **구현 단계**:  
  1. **외형 설정**: 드워븐 스파이더, 스프리건, 혹은 Static 오브젝트(토템 기둥 메쉬)의 외형을 가진 NPC를 생성한다. 만약 물리적인 충돌만 필요하고 모습은 이펙트로 대체하고 싶다면 투명 텍스처를 입힌다.  
  2. **AI 패키지 구성**: CombatOverridePackage를 사용하여 이동 속도를 0으로 설정하거나 Can Move \= False 플래그를 켠다. 그리고 사거리 내의 적을 감지하면 즉시 마법을 시전하거나 화살을 발사하도록 설정한다.  
  3. **소멸 및 제한 로직**: 소환 시간 만료 시 Delete() 되도록 스크립트를 짜거나, 바닐라의 소환수 시스템(Commanded Actor)을 이용해 최대 소환 개수를 제한한다. 이는 PoE의 토템 최대 개수 제한과 동일한 메커니즘이다.  
  4. **시각적 일치**: '헤르메우스 모라'의 촉수 모델이나 '솔츠하임'의 스톤 비석 모델을 토템의 외형으로 사용하면, 스카이림의 로어(Lore)를 해치지 않으면서도 이질감 없는 ARPG 분위기를 연출할 수 있다.23

## **6\. 시점과 조작의 변혁: 등각 투영(Isometric)과 이동 방식의 타협**

스카이림을 디아블로처럼 보이게 하는 가장 결정적인 요소는 카메라 시점이다. 하지만 스카이림은 1/3인칭 백뷰(Back-view)를 상정하고 만들어졌기 때문에, 완벽한 탑뷰(Top-view) 구현에는 여러 기술적 난관이 따른다.

### **6.1 등각 투영 카메라(Isometric Camera) 설정과 한계**

바닐라 카메라 설정만으로는 완벽한 쿼터뷰 구현이 불가능하며, SKSE 기반의 모드 조합이 필수적이다.24

* **필수 모드 및 설정**: Smooth Camera 또는 3PCO와 같은 카메라 모드를 통해 카메라의 추적 속도와 위치를 제어해야 한다. 특히 True Directional Movement (TDM) 모드는 캐릭터가 카메라가 보는 방향이 아닌, 입력한 키 방향(WASD)으로 즉시 회전하고 이동하게 해주는 기능을 제공하여 360도 전방위 공격이 가능한 ARPG 조작감을 완성한다.  
* **시점 고정(Camera Lock)**: 특정 각도(Pitch, 약 45\~60도)와 거리(Zoom, 약 800\~1200 유닛)를 강제하는 스크립트나 설정을 통해, 플레이어가 마우스로 카메라를 돌려도 등각 시점이 유지되도록 해야 한다. Some Collision Camera와 같은 모드는 카메라가 지형지물에 부딪혀 멋대로 줌인되는 현상을 방지한다.  
* **던전 천장 문제 (Occlusion)**: 실내(Indoor) 던전에서는 낮은 천장 메쉬 때문에 카메라가 캐릭터 등 뒤로 급격히 줌인되는 문제가 발생한다. 이를 해결하기 위한 근본적인 방법은 실내 천장을 높게 수정한 메쉬를 사용하는 것이지만, 이는 바닐라 에셋 활용 범위를 벗어난다. 따라서 현실적인 타협안은 실내 진입 시 자동으로 숄더뷰(Shoulder View)로 전환하거나, Fade 노드를 사용하여 천장 메쉬를 투명하게 만드는 방식이다.

### **6.2 클릭 투 무브(Click-to-Move)의 기술적 난관과 대안**

디아블로의 상징인 '마우스 클릭 이동'은 스카이림 엔진에서 구현하기 가장 어려운 기능 중 하나이다.24

* **네비메쉬(Navmesh)와 레이턴시**: 클릭 이동을 구현하려면 마우스가 클릭한 지점의 월드 좌표(Raycast Hit)를 따서, 해당 좌표로 이동하는 AI 패키지(Pathfinding)를 실시간으로 플레이어에게 부여해야 한다. 유니티 엔진 기반의 모딩 툴(uMMORPG)에서는 이것이 가능하지만, 스카이림의 파피루스 스크립트는 반응속도(Latency)가 느려 클릭 후 캐릭터가 반응하기까지 미세한 지연이 발생한다. 이는 빠르고 정밀한 조작이 필요한 핵 앤 슬래시 장르에서 치명적인 단점이 된다.27  
* **WASD와 타겟 락온의 결합**: 따라서 무리한 클릭 투 무브 구현보다는, 'WASD' 키보드 이동을 유지하되 **타겟 락온(Target Lock)** 시스템을 강화하는 것이 더 나은 대안이다. 마우스 커서가 가리키는 적을 자동으로 바라보고(FaceTarget), 스킬을 시전하면 해당 방향으로 투사체가 유도되는 방식을 채택함으로써, 조작의 편의성과 엔진의 안정성을 동시에 확보할 수 있다.28

## **7\. 최적화 및 엔진 한계 고려 (Technical Constraints)**

ARPG 스타일은 필연적으로 다수의 몬스터와 화려한 이펙트를 한 화면에 담게 된다. 스카이림 엔진은 이러한 물량 처리에 취약할 수 있으므로 최적화 전략이 필수적이다.

* **드로우 콜(Draw Call)과 프레임 방어**: 등각 시점은 평소보다 더 넓은 범위를 조망하므로 렌더링 부하가 커진다. 원경의 디테일(LOD)을 줄이거나, 몬스터 스폰 시 PlaceAtMe를 과도하게 사용하여 메모리 단편화를 일으키지 않도록 주의해야 한다. 대신 미리 맵에 배치된 Disabled 액터를 스크립트로 Enable 하는 방식(Object Pooling 유사 기법)을 사용해야 스크립트 랙(Script Lag)과 프레임 드랍을 방지할 수 있다.29  
* **스크립트 덤프 방지**: OnHit이나 OnUpdate 이벤트가 초당 수십 번 발생하는 고속 연사 스킬(예: CoC 빌드)의 경우, 스크립트 큐가 밀리는 현상이 발생할 수 있다. RegisterForSingleUpdate를 사용하여 이전 업데이트가 끝나기 전에는 다음 업데이트가 예약되지 않도록 안전장치를 마련해야 한다. 또한 복잡한 연산은 OnHit 이벤트 내부가 아닌, 별도의 State나 Quest Script로 분산시켜야 한다.20

## **8\. 결론**

스카이림의 바닐라 에셋을 활용하여 디아블로 및 PoE 스타일의 게임플레이를 구현하는 것은 단순한 모드 설치를 넘어선, 엔진의 한계를 시험하고 재해석하는 창의적인 기술적 도전이다. **드래곤 형상**과 **메리디아의 빛**을 분해하여 시각적 보상 체계를 구축하고, **NifSkope 메쉬 병합**과 **파피루스 이벤트 스크립팅**을 통해 전투의 밀도와 속도감을 높이며, **Hazard**와 **Invisible Actor**를 적재적소에 배치하여 전술적 깊이를 더하는 것이 핵심 방법론이다. 비록 '클릭 투 무브'와 같은 일부 조작 방식은 엔진의 근본적인 한계로 인해 타협이 필요하지만, 본 보고서에서 제시된 기술적 우회로들을 통해 스카이림의 방대한 세계관 위에서 정교하고 몰입감 넘치는 핵 앤 슬래시 경험을 재창조하는 것은 충분히 가능한 목표이다. 이러한 시도는 스카이림을 단순히 '오래된 게임'이 아닌, 무한한 가능성을 지닌 '플랫폼'으로 바라볼 때 비로소 실현될 수 있다.

## **9\. Calamity 적용 우선순위(선별 버전)**

본 문서의 아이디어를 전부 가져오기보다, **현재 Calamity 구조(SKSE DLL + 데이터 주도 + CK 최소화)**에 맞는 항목만 우선 채택한다.

### **9.1 바로 가져갈 항목 (P1)**

1. **챔피언/엘리트 시각 피드백**
   - 바닐라 EffectShader 기반 색상 오라(속성/등급별)만 우선 적용
   - 방어구 교체형 연출보다 `VisualEffect.Play/Stop` 중심으로 저비용 구현
2. **루트 빔 + 픽업 연동 제거**
   - 고등급 드랍에만 빔/사운드 부여
   - 아이템 획득/소멸 시 빔 정리(잔상 오브젝트 누수 금지)
3. **트리거 스킬 체감 강화**
   - CoC/시체폭발/소환형 효과는 유지하되 ICD/대상별 쿨다운 가드 필수
   - 군중 전투에서 프레임 저하를 막는 안전장치(최대 동시 처리량) 유지
4. **어픽스 밸런스 운영 원칙**
   - 신규 드랍은 prefix 중심 운영(현재 suffix 드랍 soft-disable 상태)
   - suffix는 호환성 유지를 위해 데이터/런타임 처리만 유지

### **9.2 다음 단계 항목 (P2)**

1. **Hazard 기반 장판 효과**
   - 단순 디버프/지대 제어 중심으로 먼저 도입
2. **토템/포탑 메커니즘**
   - Invisible Actor 방식은 상한 개수/수명/정리 로직을 먼저 설계한 뒤 도입

### **9.3 보류/비채택 항목 (현재 기준)**

1. **등각 고정 카메라/클릭 투 무브**
   - 엔진 구조상 안정성/조작 일관성 리스크가 커서 현재 스코프에서 제외
2. **NIF 다중 투사체 병합을 핵심 전투 메커니즘으로 채택**
   - 시각과 실제 판정 불일치 리스크가 있어, 당장은 VFX 연출용으로만 제한

### **9.4 적용 판단 기준(게이트)**

1. 전투 체감 개선이 명확해야 함(드랍 인지성, 빌드 개성, 피드백 가독성)
2. 스크립트 안정성 유지(스택 덤프/누수/무한 루프 없음)
3. 대규모 전투에서 프레임 방어(기존 대비 급격한 하락 없음)
4. 데이터 주도 워크플로우를 유지해야 함(`affixes/affixes.json` 중심)

#### **참고 자료**

1. Dragon aspect, invisibility, lighting cloak. Looks pretty cool. : r/skyrim \- Reddit, 2월 16, 2026에 액세스, [https://www.reddit.com/r/skyrim/comments/1bxlsv7/dragon\_aspect\_invisibility\_lighting\_cloak\_looks/](https://www.reddit.com/r/skyrim/comments/1bxlsv7/dragon_aspect_invisibility_lighting_cloak_looks/)  
2. Skyrim Creations \- Vortikai's Dragon Aspect Toggle \- Bethesda, 2월 16, 2026에 액세스, [https://creations.bethesda.net/en/skyrim/details/6f55efdc-2da5-4426-a211-3de12178f3f5/Vortikai\_s\_Dragon\_Aspect\_Toggle](https://creations.bethesda.net/en/skyrim/details/6f55efdc-2da5-4426-a211-3de12178f3f5/Vortikai_s_Dragon_Aspect_Toggle)  
3. Champion's Cloak Texture Pack (PS4) \- Skyrim Creations, 2월 16, 2026에 액세스, [https://creations.bethesda.net/en/skyrim/details/afe1e948-7647-4981-b9c1-60ffa859993d/Champion\_s\_Cloak\_Texture\_Pack\_\_PS4\_](https://creations.bethesda.net/en/skyrim/details/afe1e948-7647-4981-b9c1-60ffa859993d/Champion_s_Cloak_Texture_Pack__PS4_)  
4. How do you get dragon aspect with console commands? : r/skyrim \- Reddit, 2월 16, 2026에 액세스, [https://www.reddit.com/r/skyrim/comments/628423/how\_do\_you\_get\_dragon\_aspect\_with\_console\_commands/](https://www.reddit.com/r/skyrim/comments/628423/how_do_you_get_dragon_aspect_with_console_commands/)  
5. \[LE\] Apply effect to an NPC \- Creation Kit and Modders \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/9357188-le-apply-effect-to-an-npc/](https://forums.nexusmods.com/topic/9357188-le-apply-effect-to-an-npc/)  
6. Understanding visual effects in script \- Creation Kit and Modders \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/13517206-understanding-visual-effects-in-script/](https://forums.nexusmods.com/topic/13517206-understanding-visual-effects-in-script/)  
7. Skyrim Mod: More Draconic Dragon Aspect \- YouTube, 2월 16, 2026에 액세스, [https://www.youtube.com/watch?v=2WPvp6nAXkg](https://www.youtube.com/watch?v=2WPvp6nAXkg)  
8. Skyrim 1350 Mods: Flying Animations for Dragon Aspect, New Creatures and More, 2월 16, 2026에 액세스, [https://www.youtube.com/watch?v=VNG1Rmye5FE](https://www.youtube.com/watch?v=VNG1Rmye5FE)  
9. The Break of Dawn \- The Elder Scrolls Wiki \- Fandom, 2월 16, 2026에 액세스, [https://elderscrolls.fandom.com/wiki/The\_Break\_of\_Dawn](https://elderscrolls.fandom.com/wiki/The_Break_of_Dawn)  
10. Skyrim Mod Feature: Light Sword \- Burning Eye of Meridia \- YouTube, 2월 16, 2026에 액세스, [https://www.youtube.com/watch?v=oXGLGdWEQdQ](https://www.youtube.com/watch?v=oXGLGdWEQdQ)  
11. Standing in Meridia's Light (minor quest spoilers) : r/skyrim \- Reddit, 2월 16, 2026에 액세스, [https://www.reddit.com/r/skyrim/comments/18lnzd/standing\_in\_meridias\_light\_minor\_quest\_spoilers/](https://www.reddit.com/r/skyrim/comments/18lnzd/standing_in_meridias_light_minor_quest_spoilers/)  
12. Editting projectile nifs \- Creation Kit and Modders \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/5977153-editting-projectile-nifs/](https://forums.nexusmods.com/topic/5977153-editting-projectile-nifs/)  
13. NifSkope Comprehensive Guide \- the Oblivion ConstructionSet Wiki, 2월 16, 2026에 액세스, [https://cs.uesp.net/wiki/NifSkope\_Comprehensive\_Guide](https://cs.uesp.net/wiki/NifSkope_Comprehensive_Guide)  
14. \[LE\] Apply the dragon absorb effect on death of a regular NPC \- Creation Kit and Modders, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/8799833-le-apply-the-dragon-absorb-effect-on-death-of-a-regular-npc/](https://forums.nexusmods.com/topic/8799833-le-apply-the-dragon-absorb-effect-on-death-of-a-regular-npc/)  
15. Getting multiple magic effects to play simultaneously. \- Creation Kit ..., 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/12282638-getting-multiple-magic-effects-to-play-simultaneously/](https://forums.nexusmods.com/topic/12282638-getting-multiple-magic-effects-to-play-simultaneously/)  
16. Multi Projectile Spell \- Skyrim LE \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/1744411-multi-projectile-spell/](https://forums.nexusmods.com/topic/1744411-multi-projectile-spell/)  
17. What is the best build in path of exile 2? \- Quora, 2월 16, 2026에 액세스, [https://www.quora.com/What-is-the-best-build-in-path-of-exile-2](https://www.quora.com/What-is-the-best-build-in-path-of-exile-2)  
18. Path of Exile 1 Gameplay Help and Discussion \- Cospri's Malice \- Embrace it \- Forum, 2월 16, 2026에 액세스, [https://www.pathofexile.com/forum/view-thread/1717008](https://www.pathofexile.com/forum/view-thread/1717008)  
19. pWn3d1337/Skyrim\_SpellHotbar2: SpellHotbar2, now only SKSE besides MCM, WIP \- GitHub, 2월 16, 2026에 액세스, [https://github.com/pWn3d1337/Skyrim\_SpellHotbar2](https://github.com/pWn3d1337/Skyrim_SpellHotbar2)  
20. Multiple Projectiles in a Magic Effect \- Papyrus 101 \- TES Alliance, 2월 16, 2026에 액세스, [https://tesalliance.org/forums/index.php?/topic/5515-multiple-projectiles-in-a-magic-effect/](https://tesalliance.org/forums/index.php?/topic/5515-multiple-projectiles-in-a-magic-effect/)  
21. Hazard Objects? \- Skyrim LE \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/624171-hazard-objects/](https://forums.nexusmods.com/topic/624171-hazard-objects/)  
22. Hazard Scripting help \- Creation Kit and Modders \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/764483-hazard-scripting-help/](https://forums.nexusmods.com/topic/764483-hazard-scripting-help/)  
23. What was Radiant AI, anyway? \- paavohtl's blog, 2월 16, 2026에 액세스, [https://blog.paavo.me/radiant-ai/](https://blog.paavo.me/radiant-ai/)  
24. The Isometric Scrolls Skyrim \- TISS \- A Tweak Guide by Sacrossauro ..., 2월 16, 2026에 액세스, [https://www.nexusmods.com/skyrimspecialedition/mods/89358](https://www.nexusmods.com/skyrimspecialedition/mods/89358)  
25. Skyrim Isometric Experience Reborn : r/skyrimmods \- Reddit, 2월 16, 2026에 액세스, [https://www.reddit.com/r/skyrimmods/comments/1hu84iy/skyrim\_isometric\_experience\_reborn/](https://www.reddit.com/r/skyrimmods/comments/1hu84iy/skyrim_isometric_experience_reborn/)  
26. Is walking with just the mouse possible in Skyrim? \- Arqade \- Stack Exchange, 2월 16, 2026에 액세스, [https://gaming.stackexchange.com/questions/98946/is-walking-with-just-the-mouse-possible-in-skyrim](https://gaming.stackexchange.com/questions/98946/is-walking-with-just-the-mouse-possible-in-skyrim)  
27. uMMORPG Remastered Documentation | PDF | User Interface | Acceleration \- Scribd, 2월 16, 2026에 액세스, [https://www.scribd.com/document/840698505/uMMORPG-Remastered-Documentation](https://www.scribd.com/document/840698505/uMMORPG-Remastered-Documentation)  
28. Skyrim Mods | True Directional Movement \- Modernized Third Person Gameplay \- YouTube, 2월 16, 2026에 액세스, [https://www.youtube.com/watch?v=cV5E2qEp88w](https://www.youtube.com/watch?v=cV5E2qEp88w)  
29. Unofficial Skyrim Patch: Version History \- AFK Mods, 2월 16, 2026에 액세스, [https://www.afkmods.com/Unofficial%20Skyrim%20Patch%20Version%20History.html](https://www.afkmods.com/Unofficial%20Skyrim%20Patch%20Version%20History.html)  
30. Understanding Leveled Lists \- Creation Kit and Modders \- Nexus Mods Forums, 2월 16, 2026에 액세스, [https://forums.nexusmods.com/topic/13478654-understanding-leveled-lists/](https://forums.nexusmods.com/topic/13478654-understanding-leveled-lists/)
