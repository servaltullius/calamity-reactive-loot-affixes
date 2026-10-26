#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace CalamityAffixes::RunewordSummary
{
	enum class BaseType : std::uint8_t
	{
		kUnknown = 0,
		kWeapon,
		kArmor,
	};

	struct RecipeKeyOverride
	{
		std::string_view id;
		std::string_view key;
	};

	template <std::size_t N>
	[[nodiscard]] constexpr std::string_view FindKeyOverride(
		std::string_view a_id,
		const RecipeKeyOverride (&a_overrides)[N]) noexcept
	{
		for (const auto& entry : a_overrides) {
			if (entry.id == a_id) {
				return entry.key;
			}
		}
		return {};
	}

	template <std::size_t N>
	[[nodiscard]] constexpr bool HasDuplicateOverrideIds(const RecipeKeyOverride (&a_overrides)[N]) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) {
			for (std::size_t j = i + 1; j < N; ++j) {
				if (a_overrides[i].id == a_overrides[j].id) {
					return true;
				}
			}
		}
		return false;
	}

	[[nodiscard]] constexpr std::uint32_t RecipeBucket(std::string_view a_id, std::uint32_t a_modulo) noexcept
	{
		if (a_modulo == 0u) {
			return 0u;
		}
		std::uint32_t hash = 2166136261u;
		for (const auto c : a_id) {
			hash ^= static_cast<std::uint8_t>(c);
			hash *= 16777619u;
		}
		return hash % a_modulo;
	}

	inline constexpr RecipeKeyOverride kEffectSummaryOverrides[] = {
		{ "rw_grief", "signature_grief" },
		{ "rw_infinity", "signature_infinity" },
		{ "rw_enigma", "signature_enigma" },
		{ "rw_call_to_arms", "signature_call_to_arms" },
		{ "rw_spirit", "signature_spirit" },
		{ "rw_insight", "signature_insight" },
		{ "rw_fortitude", "signature_fortitude" },
		{ "rw_heart_of_the_oak", "signature_heart_of_the_oak" },
		{ "rw_last_wish", "signature_last_wish" },
		{ "rw_exile", "lowhealth_ironskin" },
		{ "rw_breath_of_the_dying", "signature_breath_of_the_dying" },
		{ "rw_chains_of_honor", "signature_chains_of_honor" },
		{ "rw_dream", "signature_dream" },
		{ "rw_faith", "signature_faith" },
		{ "rw_phoenix", "kill_restore" },
		{ "rw_doom", "aura_slow" },
		{ "rw_sanctuary", "thorns_reflect" },
		{ "rw_obsession", "signature_obsession" },
		{ "rw_beast", "signature_beast" },
		{ "rw_hustle_w", "self_haste" },
		{ "rw_hustle_a", "signature_hustle_a" },
		{ "rw_treachery", "signature_treachery" },
		{ "rw_honor", "signature_honor" },
		{ "rw_eternity", "signature_eternity" },
		{ "rw_leaf", "status_leaf" },
		{ "rw_wrath", "fire_strike" },
		{ "rw_brand", "fire_dot" },
		{ "rw_voice_of_reason", "debuff_frost_shred" },
		{ "rw_pride", "signature_pride" },
		{ "rw_mist", "signature_mist" },
		{ "rw_holy_thunder", "shock_strike" },
		{ "rw_crescent_moon", "debuff_shock_shred" },
		{ "rw_destruction", "signature_destruction" },
		{ "rw_malice", "poison_dot" },
		{ "rw_venom", "debuff_poison_shred" },
		{ "rw_plague", "signature_plague" },
		{ "rw_nadir", "debuff_fear" },
		{ "rw_peace", "debuff_calm" },
		{ "rw_rift", "debuff_magic_shred" },
		{ "rw_delirium", "debuff_frenzy" },
		{ "rw_black", "debuff_paralysis" },
		{ "rw_ice", "aura_frost_shred" },
		{ "rw_flickering_flame", "aura_fire_shred" },
		{ "rw_silence", "debuff_magicka_drain" },
		{ "rw_obedience", "debuff_slow" },
		{ "rw_wind", "debuff_attack_slow" },
		{ "rw_famine", "signature_famine" },
		{ "rw_kingslayer", "bleed_dot" },
		{ "rw_death", "debuff_absorb_health" },
		{ "rw_mosaic", "signature_mosaic" },
		{ "rw_chaos", "signature_chaos" },
		{ "rw_steel", "debuff_heal_suppress" },
		{ "rw_fury", "status_fury" },
		{ "rw_hand_of_justice", "signature_hand_of_justice" },
		{ "rw_passion", "status_passion" },
		{ "rw_harmony", "debuff_stamina_suppress" },
		{ "rw_memory", "self_stamina_recall" },
		{ "rw_temper", "self_stamina_regen" },
		{ "rw_pattern", "self_crit_chance" },
		{ "rw_strength", "status_strength" },
		{ "rw_edge", "status_edge" },
		{ "rw_oath", "status_oath" },
		{ "rw_zephyr", "self_bow_speed" },
		{ "rw_melody", "absorb_stamina" },
		{ "rw_unbending_will", "status_unbending_will" },
		{ "rw_bulwark", "self_ward" },
		{ "rw_duress", "debuff_weaken" },
		{ "rw_stone", "debuff_armor_shred" },
		{ "rw_prudence", "self_shock_ward" },
		{ "rw_bone", "self_frost_ward" },
		{ "rw_splendor", "self_magic_shield" },
		{ "rw_hearth", "status_hearth" },
		{ "rw_ground", "self_shout_recovery" },
		{ "rw_principle", "self_fire_resist" },
		{ "rw_ancients_pledge", "self_poison_resist" },
		{ "rw_myth", "status_myth" },
		{ "rw_dragon", "signature_dragon" },
		{ "rw_bramble", "self_reflect_damage" },
		{ "rw_metamorphosis", "self_ethereal" },
		{ "rw_lore", "self_magicka_regen" },
		{ "rw_enlightenment", "status_enlightenment" },
		{ "rw_white", "self_mana_surge" },
		{ "rw_kings_grace", "self_heal_rate" },
		{ "rw_cure", "status_cure" },
		{ "rw_rain", "self_heal_rate_mult" },
		{ "rw_wisdom", "self_magicka_regen" },
		{ "rw_lionheart", "lowhealth_emergency" },
		{ "rw_rhyme", "signature_rhyme" },
		{ "rw_wealth", "self_carry_weight" },
		{ "rw_radiance", "self_detect_life" },
		{ "rw_stealth", "self_invisibility" },
		{ "rw_smoke", "self_smoke_escape" },
		{ "rw_gloom", "absorb_magicka" },
		{ "rw_lawbringer", "kill_turn_undead" },
		{ "rw_shadow_boxer", "signature_shadow_boxer" },
	};

	static_assert(!HasDuplicateOverrideIds(kEffectSummaryOverrides),
		"kEffectSummaryOverrides must not contain duplicate recipe ids.");

	[[nodiscard]] inline std::string_view ResolveEffectSummaryKey(
		std::string_view a_recipeId,
		BaseType a_recommendedBaseType)
	{
		if (const auto overrideKey = FindKeyOverride(a_recipeId, kEffectSummaryOverrides); !overrideKey.empty()) {
			return overrideKey;
		}

		switch (a_recommendedBaseType) {
		case BaseType::kWeapon:
			switch (RecipeBucket(a_recipeId, 6u)) {
			case 0u:
				return "adaptive_strike";
			case 1u:
				return "adaptive_exposure";
			case 2u:
				return "fire_strike";
			case 3u:
				return "frost_strike";
			case 4u:
				return "shock_strike";
			default:
				return "self_haste";
			}
		case BaseType::kArmor:
			switch (RecipeBucket(a_recipeId, 6u)) {
			case 0u:
				return "self_ward";
			case 1u:
				return "self_barrier";
			case 2u:
				return "self_meditation";
			case 3u:
				return "self_phase";
			case 4u:
				return "self_muffle";
			default:
				return "self_phoenix";
			}
		case BaseType::kUnknown:
		default:
			break;
		}

		return RecipeBucket(a_recipeId, 2u) == 0u ? "adaptive_exposure" : "adaptive_strike";
	}

	// Player-facing effect text per summary key, Korean and English. The recipe
	// list and the tooltip put the trigger, chance, cooldown and duration in
	// front of and behind it ("On hit 30% chance to <en>"), so an entry starts
	// with the effect itself, never with a trigger phrase, and writes English as
	// a verb phrase.
	struct ActionSummaryText
	{
		std::string_view key;
		std::string_view ko;
		std::string_view en;
	};

	inline constexpr ActionSummaryText kActionSummaryTexts[] = {
		{ "status_leaf", "나뭇잎의 불꽃: 화염 피해 15 + 화상 2중첩, 상시 파괴 마법 비용 -10%", "deal 15 Fire Damage and add 2 Burning stacks; passively -10% Destruction spell cost" },
		{ "status_strength", "압쇄: 대상 방어도 -100(노출), 상시 양손 무기 +10%", "lower the target's Armor by 100 (Exposed); passively +10% Two-Handed" },
		{ "status_edge", "파멸 낙인: 1.5초 뒤 그 적중 피해의 50% 폭발, 상시 궁술 +10%", "mark the target with Doom (1.5s later, 50% of that hit's damage bursts); passively +10% Archery" },
		{ "status_oath", "서약의 칼날: 출혈 2/초 + 출혈 축적 +15, 상시 한손 무기 +10%", "bleed the target for 2/s and add 15 Bleed build-up; passively +10% One-Handed" },
		{ "status_passion", "정열의 번개: 번개 피해 15 + 감전 축적 +100, 상시 환혹 마법 비용 -10%", "deal 15 Shock Damage and add 100 Shock build-up; passively -10% Illusion spell cost" },
		{ "status_unbending_will", "굳건한 의지: 상태 2가지 이상인 적에게 적중 피해의 25% 추가, 상시 막기 +15%", "deal +25% of the hit's damage to an enemy with 2+ statuses; passively +15% Block" },
		{ "status_cure", "정화의 태양: 공격자에게 화염 피해 20 + 화상 2중첩, 상시 회복 마법 비용 -10%", "deal 20 Fire Damage to the attacker and add 2 Burning stacks; passively -10% Restoration spell cost" },
		{ "status_hearth", "서리 반격: 공격자에게 냉기 피해 10 + 빙결 축적 +100, 상시 냉기 저항 +20", "deal 10 Frost Damage to the attacker and add 100 Freeze build-up; passively +20 Frost Resist" },
		{ "status_enlightenment", "깨달음의 눈: 공격자 마법 저항 -25(노출), 상시 변화 마법 비용 -10%", "lower the attacker's Magic Resist by 25 (Exposed); passively -10% Alteration spell cost" },
		{ "status_myth", "신화의 가시: 공격자에게 피해 10 + 출혈 축적 +100, 상시 중갑 착용 시 받는 피해 -10%", "deal 10 damage to the attacker and add 100 Bleed build-up; passively 10% less damage taken while wearing heavy armor" },
		{ "status_fury", "격노의 일격: 피해 15 + 출혈 축적 +100", "deal 15 damage and add 100 Bleed build-up" },
		{ "signature_grief", "몰아치는 연타가 피를 빨아 버팀", "heal for 8% of the hit's damage; passively +6% Attack Damage" },
		{ "signature_infinity", "가장 낮은 원소 저항을 더 깎아 약점 피해 증폭", "lower the target's lowest elemental resistance further (Exposed)" },
		{ "signature_enigma", "투명화·이동 속도 +45%(4초), 상시 이동 속도 +10%", "turn invisible with +45% Move Speed (4s); passively +10% Move Speed" },
		{ "signature_call_to_arms", "공격력 +20%(8초), 상시 체력 +50·마나 +30", "gain +20% Attack Damage (8s); passively +50 Health and +30 Magicka" },
		{ "signature_spirit", "주문 흡수 확률 +10%p; 상시 효과로 최대 마나 +30", "gain +10 percentage points Spell Absorption; passively gain +30 Max Magicka" },
		{ "signature_insight", "마나 회복 강화, 상시 마나 재생 +25%", "boost Magicka recovery; passively +25% Magicka Regen" },
		{ "signature_fortitude", "강철 의지 버프로 난전 생존력 상승", "raise your Damage Resist; passively +80 Armor" },
		{ "signature_heart_of_the_oak", "대상의 가장 높은 원소 저항을 낮춤(노출)", "lower the target's highest elemental resistance (Exposed)" },
		{ "signature_dream", "번개 공명을 축적해 전격 일격을 터뜨림", "strike with Shock and add 35 Shock build-up; passively +20 Shock Resist" },
		{ "signature_faith", "공격력 +30%(6초), 상시 공격력 +8%", "gain +30% Attack Damage (6s); passively +8% Attack Damage" },
		{ "signature_chains_of_honor", "이동 속도 +35%(5초), 상시 3원소 저항 +15", "gain +35% Move Speed (5s); passively +15 Fire/Frost/Shock Resist" },
		{ "signature_breath_of_the_dying", "처치한 시체가 폭발해 반경 600에 독 피해 24 + 시체 최대 체력 6%", "make the corpse explode for 24 Poison Damage + 6% of its max Health in radius 600" },
		{ "signature_shadow_boxer", "그림자 권투: 근접 적중마다 0.4초 뒤 그림자가 물리 피해의 40%로 재타격", "enter Shadow Boxing: your shadow repeats each melee hit 0.4s later for 40% of its physical damage" },
		{ "signature_obsession", "마법 피해 75~300(적중 피해 18%), 상시 마나 재생률 +25%", "deal 75-300 Magic Damage (18% of the hit's damage); passively +25% Magicka Regen Rate" },
		{ "signature_hustle_a", "방어도 +60(5초), 상시 이동 속도 +8%", "gain +60 Armor (5s); passively +8% Move Speed" },
		{ "signature_treachery", "이동 속도 +28%(5초), 상시 기력 재생 +25%", "gain +28% Move Speed (5s); passively +25% Stamina Regen" },
		{ "signature_honor", "체력 재생률 +80%(7초), 상시 체력 재생률 +20%", "gain +80% Health Regen Rate (7s); passively +20% Health Regen Rate" },
		{ "signature_mosaic", "화염·냉기·전격 피해를 각각 8~80(적중 피해 5%) 동시에 가함", "deal 8-80 Fire, Frost and Shock Damage each (5% of the hit's damage)" },
		{ "signature_chaos", "가장 낮은 원소 저항을 추적해 해당 원소 피해 45", "hit the target's weakest element for 45 damage" },
		{ "signature_rhyme", "방어도 +60(6초), 상시 냉기 저항 +40", "gain +60 Armor (6s); passively +40 Frost Resist" },
		{ "signature_destruction", "반경 약 5m 전격 폭풍: 물리 피해의 4%, 초당 10~40(5초)", "call a Shock storm (radius ~5m): 4% of physical hit damage as 10-40 Shock Damage/s (5s)" },
		{ "signature_hand_of_justice", "화염 심판으로 체력 40~250 흡수(적중 피해 18%)", "absorb 40-250 Health with holy fire (18% of the hit's damage)" },
		{ "signature_dragon", "방어도 +120·화염/냉기/전격 저항 +25(8초)", "gain +120 Armor and +25 Fire/Frost/Shock Resist (8s)" },
		{ "signature_mist", "마나 재생 -100%·마법 저항 -30(6초), 마나 75 즉시 소진", "cut Magicka Regen by 100% and Magic Resist by 30 (6s), and drain 75 Magicka at once" },
		{ "signature_famine", "기력 -30/초(5초), 공격력 -20%(6초)", "drain 30 Stamina/s (5s) and cut Attack Damage by 20% (6s)" },
		{ "signature_beast", "공격력 +40%·방어도 +150(10초)", "gain +40% Attack Damage and +150 Armor (10s)" },
		{ "signature_eternity", "방어도 +300·피해 반사 +25%(6초)", "gain +300 Armor and reflect 25% of damage (6s)" },
		{ "signature_last_wish", "체력 250 회복·방어도 +250·마법 저항 +50(12초)", "restore 250 Health and gain +250 Armor and +50 Magic Resist (12s)" },
		{ "signature_plague", "시체 연쇄 폭발: 독 피해 12 + 시체 최대 체력 3%, 반경 450, 최대 연쇄 깊이 2", "set off a corpse chain explosion: 12 Poison Damage + 3% of the corpse's max Health, radius 450, up to 2 links deep" },
		{ "signature_pride", "반경 약 3.5m 냉기 충격파: 물리 피해의 16%, 피해 30~200", "release a Frost shockwave (radius ~3.5m): 16% of physical hit damage, 30-200 damage" },
		{ "adaptive_strike", "적의 가장 약한 원소 저항 축을 자동 추적해 관통 타격", "strike the enemy's weakest elemental resistance" },
		{ "adaptive_exposure", "적의 가장 높은 저항 축을 벗겨내 후속 원소 피해 창출", "strip the enemy's highest resistance for follow-up elemental damage" },
		{ "fire_strike", "화염을 실은 일격으로 적에게 추가 화염 피해", "deal extra Fire Damage" },
		{ "frost_strike", "냉기를 실은 일격으로 적에게 추가 냉기 피해", "deal extra Frost Damage" },
		{ "shock_strike", "번개를 실은 일격으로 적에게 추가 전격 피해", "deal extra Shock Damage" },
		{ "fire_dot", "화염 낙인을 새겨 지속 화상 피해", "brand the target for 10 Fire Damage/s and add 2 Burning stacks" },
		{ "shock_dot", "전류를 흘려보내 지속 전격 피해", "deal Shock Damage over time" },
		{ "poison_dot", "독이 혈류를 타고 번져 지속 중독 피해", "poison the target for 8 damage/s" },
		{ "bleed_dot", "열린 상처에서 피가 흘러 지속 출혈 피해", "bleed the target for 3/s and add 35 Bleed build-up" },
		{ "siphon_bloom", "흡수 꽃망울이 자원을 빨아 약화", "drain the target's resources" },
		{ "debuff_paralysis", "암흑 충격으로 적의 몸을 순간 마비시킴", "paralyze the target" },
		{ "debuff_calm", "평화의 파동으로 적의 적대심을 일시 해제(레벨 60 이하)", "calm the enemy (up to level 60)" },
		{ "debuff_fear", "공포의 파동으로 적을 도주하게 만듦(레벨 60 이하)", "make the enemy flee in fear (up to level 60)" },
		{ "debuff_frenzy", "광란의 파동으로 적을 동료에게 돌아서게 만듦(레벨 60 이하)", "frenzy the enemy into attacking its allies (up to level 60)" },
		{ "debuff_slow", "묵직한 압박으로 적의 이동을 크게 둔화", "slow the target by 50%" },
		{ "debuff_absorb_health", "사신의 손길로 적의 생명력을 빼앗아 흡수", "absorb 15 Health from the target" },
		{ "debuff_magicka_drain", "침묵의 파동으로 적의 마력을 고갈시킴", "drain 50 of the target's Magicka" },
		{ "debuff_stamina_drain", "기근의 저주로 적의 기력을 소진시킴", "drain the enemy's Stamina" },
		{ "debuff_multi_element", "복합 원소 파편이 한꺼번에 작렬", "hit with several elements at once" },
		{ "aura_slow", "타격마다 냉기 감속장을 씌워 둔화 유지", "slow the target by 30% and add 20 Freeze build-up" },
		{ "aura_fire_shred", "타격마다 화염 저항을 깎아 후속 화상 극대화", "lower the target's Fire Resist by 25 (Exposed)" },
		{ "aura_frost_shred", "타격마다 냉기 저항을 깎아 후속 동결 극대화", "lower the target's Frost Resist by 30 (Exposed)" },
		{ "self_haste", "가속으로 빈틈을 메워 기동 우위 확보", "gain a burst of Move Speed" },
		{ "self_judgment", "심판의 기세를 실어 공격력을 대폭 끌어올림", "greatly raise your Attack Damage" },
		{ "self_stamina_recall", "전투 기억을 되살려 스태미나 회복 가속", "gain +100% Stamina Regen" },
		{ "self_crit_chance", "연계 리듬을 살려 치명타 확률 상승", "gain +15% Critical Chance" },
		{ "self_bow_speed", "질풍의 흐름으로 활 공격 속도 상승", "draw bows 30% faster" },
		{ "self_light_armor", "용의 비늘로 경갑 방호 기술 강화", "improve your Light Armor skill" },
		{ "self_ward", "수호막으로 방어도를 일시 대폭 상승", "gain a burst of Armor" },
		{ "self_barrier", "장벽으로 급사 구간을 얇게 끊어냄", "raise a barrier against burst damage" },
		{ "self_frost_ward", "냉기 보호막으로 동결 피해를 차단", "gain +45 Frost Resist" },
		{ "self_shock_ward", "전기 보호막으로 전격 피해를 크게 경감", "gain +40 Shock Resist" },
		{ "self_magic_shield", "마력 방패로 마법 저항 전반을 강화", "gain +30 Magic Resist" },
		{ "self_shock_resist", "대지 접지로 전격 저항을 올려 방어", "raise your Shock Resist" },
		{ "self_fire_resist", "원칙의 불꽃으로 화염 저항을 크게 강화", "gain +50 Fire Resist" },
		{ "self_poison_resist", "고대 서약으로 독과 질병에 대한 저항력 획득", "gain +60 Poison Resist" },
		{ "self_reflect_damage", "가시 갑옷으로 받은 피해 일부를 공격자에게 반사", "reflect 25% of damage to attackers" },
		{ "self_ethereal", "에테리얼 변신으로 잠시 무적 상태에 진입", "turn ethereal and invulnerable" },
		{ "self_heal_rate", "치유의 흐름으로 체력 회복 속도 상승", "gain +8 Heal Rate" },
		{ "self_heal_rate_mult", "생명의 비로 체력 재생을 대폭 가속", "gain +100% Heal Rate" },
		{ "self_meditation", "명상으로 전투 자원을 빠르게 회복", "recover combat resources quickly" },
		{ "self_magicka_regen", "지식의 빛으로 마나 재생을 가속", "speed up Magicka Regen" },
		{ "self_mana_surge", "마나 쇄도로 마력 총량을 일시 증폭", "gain +50 Magicka" },
		{ "self_phase", "위상 이동으로 즉시 이탈 각을 만듦", "phase out to break away" },
		{ "self_phoenix", "재점화로 반격 타이밍을 다시 잡음", "rekindle for a counterattack" },
		{ "self_muffle", "머플로 발소리를 지워 은신 진입 보조", "muffle your footsteps" },
		{ "self_invisibility", "투명화로 전투를 끊고 각 재설정", "turn invisible" },
		{ "self_smoke_escape", "공격자 이동 속도 -30%", "slow the attacker by 30%" },
		{ "self_carry_weight", "상시 소지 무게 +75·거래 가격 15% 유리", "Always active: Carry Weight +75 and prices 15% better" },
		{ "self_detect_life", "주변 생명체 감지(반경 약 60m)", "detect life around you (radius ~60m)" },
		{ "kill_poison_burst", "독성 폭발로 주변 적에게 피해", "set off a toxic burst that damages nearby enemies" },
		{ "kill_restore", "생명력과 마나를 즉시 회복", "restore Health and Magicka at once" },
		{ "kill_poison_cloud", "독 구름이 퍼져 잔여 적 압박", "spread a poison cloud over the remaining enemies" },
		{ "kill_turn_undead", "성스러운 빛으로 주변 언데드 퇴치(레벨 60 이하)", "turn nearby undead with holy light (up to level 60)" },
		{ "lowhealth_fade", "방어·저항 대폭 버프로 생존", "gain large Armor and resistance buffs" },
		{ "lowhealth_emergency", "긴급 대량 회복으로 사선 탈출", "restore 200 Health at once" },
		{ "lowhealth_ironskin", "강철 피부로 추가 피해 차단", "gain Iron Skin: +250 Armor" },
		{ "long_cd_bulwark", "대형 방어 결계 발동 (긴 재사용)", "raise a large protective ward (long cooldown)" },
		{ "long_cd_beast_rage", "야수의 격노로 폭발적 강화 (긴 재사용)", "enter a beast rage (long cooldown)" },
		{ "thorns_reflect", "받은 피해 일부를 공격자에게 되돌려 보복", "reflect 15% of damage to the attacker; passively +15 Magic Resist" },
		{ "soul_trap", "소울트랩으로 영혼을 묶어 포획", "soul trap the target" },
		{ "self_shout_recovery", "접지의 힘으로 용언 재충전 속도를 가속", "recover Shouts 50% faster" },
		{ "self_stamina_regen", "단련의 기운으로 기력 자연 재생을 가속", "gain +5 Stamina Regen" },
		{ "absorb_magicka", "암흑의 흡인으로 적의 마력을 빼앗아 전환", "absorb 30 Magicka from the attacker" },
		{ "absorb_stamina", "선율의 울림으로 적의 활력을 흡수해 전환", "absorb 20 Stamina from the target" },
		{ "debuff_weaken", "강압의 기운으로 공격자의 공격력을 약화", "cut the attacker's Attack Damage by 25%" },
		{ "debuff_attack_slow", "질풍의 압박이 적의 공격력을 꺾음", "cut the enemy's Attack Damage by 20%" },
		{ "debuff_armor_shred", "돌의 무게가 적의 방어구를 압쇄", "lower the attacker's Armor by 50 (Exposed)" },
		{ "debuff_poison_shred", "맹독의 침식으로 적의 독 저항을 와해", "lower the target's Poison Resist by 40 (Exposed)" },
		{ "debuff_shock_shred", "달빛 전류가 적의 전격 방어를 붕괴", "lower the target's Shock Resist by 35 (Exposed)" },
		{ "debuff_frost_shred", "이성의 냉기가 적의 냉기 방어를 관통", "lower the target's Frost Resist by 35 (Exposed)" },
		{ "debuff_magic_shred", "차원의 균열이 적의 마법 방어를 분쇄", "lower the target's Magic Resist by 30 (Exposed)" },
		{ "debuff_heal_suppress", "강철 출혈이 적의 상처 회복을 방해", "cut the target's healing by 80% and add 100 Bleed build-up" },
		{ "debuff_stamina_suppress", "조화의 교란이 적의 기력 재생을 억제", "cut the target's Stamina Regen by 80%" },
		{ "debuff_magicka_suppress", "안개가 적의 마력 흐름을 차단", "block the enemy's Magicka flow" },
	};

	[[nodiscard]] constexpr const ActionSummaryText* FindActionSummaryText(std::string_view a_key) noexcept
	{
		for (const auto& entry : kActionSummaryTexts) {
			if (entry.key == a_key) {
				return &entry;
			}
		}
		return nullptr;
	}

	[[nodiscard]] constexpr std::string_view ActionSummaryTextByKey(std::string_view a_key) noexcept
	{
		const auto* entry = FindActionSummaryText(a_key);
		return entry ? entry->ko : std::string_view{};
	}

	[[nodiscard]] constexpr std::string_view ActionSummaryTextEnByKey(std::string_view a_key) noexcept
	{
		const auto* entry = FindActionSummaryText(a_key);
		return entry ? entry->en : std::string_view{};
	}

	// A spell effect's area is stored in feet; the panel shows rounded metres
	// like the descriptions do (16 -> 5, 11 -> 3.5, 200 -> 60).
	[[nodiscard]] constexpr float AreaFeetToMetres(std::uint32_t a_feet) noexcept
	{
		const float metres = static_cast<float>(a_feet) * 0.3048f;
		const float step = metres < 10.0f ? 0.5f : (metres < 30.0f ? 1.0f : 5.0f);
		const auto steps = static_cast<std::uint32_t>(metres / step + 0.5f);
		const float rounded = static_cast<float>(steps) * step;
		return rounded > 0.0f ? rounded : step;
	}

	// True when a_text already states this duration ("(6초)", "+45%, 4s)"), so
	// the composer does not add " (지속 6초)" / " (Duration 6s)" a second time.
	[[nodiscard]] constexpr bool StatesDurationSeconds(
		std::string_view a_text,
		std::string_view a_seconds,
		std::string_view a_unit) noexcept
	{
		if (a_seconds.empty()) {
			return false;
		}
		const auto needleSize = a_seconds.size() + a_unit.size() + 1u;
		for (std::size_t pos = 0; pos + needleSize <= a_text.size(); ++pos) {
			if (a_text.substr(pos, a_seconds.size()) != a_seconds ||
				a_text.substr(pos + a_seconds.size(), a_unit.size()) != a_unit ||
				a_text[pos + a_seconds.size() + a_unit.size()] != ')') {
				continue;
			}
			const char before = pos > 0 ? a_text[pos - 1] : ' ';
			if ((before < '0' || before > '9') && before != '.') {
				return true;
			}
		}
		return false;
	}
}
