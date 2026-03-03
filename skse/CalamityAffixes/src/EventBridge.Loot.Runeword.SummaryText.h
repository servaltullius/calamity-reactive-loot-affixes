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
		{ "rw_last_wish", "lowhealth_fade" },
		{ "rw_exile", "lowhealth_ironskin" },
		{ "rw_breath_of_the_dying", "kill_poison_burst" },
		{ "rw_chains_of_honor", "signature_chains_of_honor" },
		{ "rw_dream", "signature_dream" },
		{ "rw_faith", "signature_faith" },
		{ "rw_phoenix", "kill_restore" },
		{ "rw_doom", "aura_slow" },
		{ "rw_sanctuary", "thorns_reflect" },
		{ "rw_obsession", "siphon_bloom" },
		{ "rw_beast", "long_cd_beast_rage" },
		{ "rw_hustle_w", "self_haste" },
		{ "rw_hustle_a", "self_phase" },
		{ "rw_treachery", "self_muffle" },
		{ "rw_honor", "soul_trap" },
		{ "rw_eternity", "long_cd_bulwark" },
		{ "rw_leaf", "fire_strike" },
		{ "rw_wrath", "fire_strike" },
		{ "rw_brand", "fire_dot" },
		{ "rw_voice_of_reason", "frost_strike" },
		{ "rw_pride", "frost_strike" },
		{ "rw_mist", "debuff_frostbite" },
		{ "rw_holy_thunder", "shock_strike" },
		{ "rw_crescent_moon", "shock_strike" },
		{ "rw_destruction", "shock_dot" },
		{ "rw_malice", "poison_dot" },
		{ "rw_venom", "poison_dot" },
		{ "rw_plague", "kill_poison_cloud" },
		{ "rw_nadir", "debuff_fear" },
		{ "rw_peace", "debuff_calm" },
		{ "rw_rift", "debuff_calm" },
		{ "rw_delirium", "debuff_frenzy" },
		{ "rw_black", "debuff_paralysis" },
		{ "rw_ice", "aura_frost_shred" },
		{ "rw_flickering_flame", "aura_fire_shred" },
		{ "rw_silence", "debuff_magicka_drain" },
		{ "rw_obedience", "debuff_slow" },
		{ "rw_wind", "debuff_slow" },
		{ "rw_famine", "debuff_stamina_drain" },
		{ "rw_kingslayer", "bleed_dot" },
		{ "rw_death", "debuff_absorb_health" },
		{ "rw_mosaic", "debuff_multi_element" },
		{ "rw_chaos", "adaptive_strike" },
		{ "rw_steel", "self_weapon_speed" },
		{ "rw_fury", "self_weapon_fury" },
		{ "rw_hand_of_justice", "self_judgment" },
		{ "rw_passion", "self_haste" },
		{ "rw_harmony", "self_haste" },
		{ "rw_memory", "self_stamina_recall" },
		{ "rw_temper", "self_tempered_strike" },
		{ "rw_pattern", "self_crit_chance" },
		{ "rw_strength", "self_two_handed" },
		{ "rw_edge", "self_marksman" },
		{ "rw_oath", "self_one_handed" },
		{ "rw_zephyr", "self_bow_speed" },
		{ "rw_melody", "self_bow_speed" },
		{ "rw_unbending_will", "self_block" },
		{ "rw_bulwark", "self_ward" },
		{ "rw_duress", "self_ward" },
		{ "rw_stone", "self_stone_skin" },
		{ "rw_prudence", "self_shock_ward" },
		{ "rw_bone", "self_frost_ward" },
		{ "rw_splendor", "self_magic_shield" },
		{ "rw_hearth", "self_frost_resist" },
		{ "rw_ground", "self_shock_resist" },
		{ "rw_principle", "self_fire_resist" },
		{ "rw_ancients_pledge", "self_poison_resist" },
		{ "rw_myth", "self_heavy_armor" },
		{ "rw_dragon", "self_light_armor" },
		{ "rw_bramble", "self_reflect_damage" },
		{ "rw_metamorphosis", "self_ethereal" },
		{ "rw_lore", "self_magicka_regen" },
		{ "rw_enlightenment", "self_alteration" },
		{ "rw_white", "self_mana_surge" },
		{ "rw_kings_grace", "self_heal_rate" },
		{ "rw_cure", "self_heal_rate" },
		{ "rw_rain", "self_heal_rate_mult" },
		{ "rw_wisdom", "self_healing_wisdom" },
		{ "rw_lionheart", "lowhealth_emergency" },
		{ "rw_rhyme", "self_phase" },
		{ "rw_wealth", "self_carry_weight" },
		{ "rw_radiance", "self_detect_life" },
		{ "rw_stealth", "self_invisibility" },
		{ "rw_smoke", "self_smoke_escape" },
		{ "rw_gloom", "self_invisibility" },
		{ "rw_lawbringer", "kill_turn_undead" },
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

	[[nodiscard]] inline std::string_view ActionSummaryTextByKey(std::string_view a_key) noexcept
	{
		if (a_key == "signature_grief") return "빈틈 없이 몰아치는 초고속 참격 연타";
		if (a_key == "signature_infinity") return "저항이 높은 축을 먼저 찢어 약점 개방";
		if (a_key == "signature_enigma") return "위험 순간 위상 전환으로 즉시 이탈";
		if (a_key == "signature_call_to_arms") return "함성으로 아군 페이스를 끌어올려 공방 동시 강화";
		if (a_key == "signature_spirit") return "집중력 흐름을 올려 전투 리듬 안정화";
		if (a_key == "signature_insight") return "명상 파동으로 자원 고갈 구간을 완화";
		if (a_key == "signature_fortitude") return "강철 의지 버프로 난전 생존력 상승";
		if (a_key == "signature_heart_of_the_oak") return "견고한 저항 축부터 깎아 후속 딜 창출";
		if (a_key == "signature_dream") return "번개 공명을 축적해 전격 일격을 터뜨림";
		if (a_key == "signature_faith") return "광신의 돌격으로 기동과 생존을 동시에 끌어올림";
		if (a_key == "signature_chains_of_honor") return "명예의 사슬로 성역의 보호를 단단히 고정";
		if (a_key == "adaptive_strike") return "적의 가장 약한 원소 저항 축을 자동 추적해 관통 타격";
		if (a_key == "adaptive_exposure") return "적의 가장 높은 저항 축을 벗겨내 후속 원소 피해 창출";
		if (a_key == "fire_strike") return "화염을 실은 일격으로 적에게 추가 화염 피해";
		if (a_key == "frost_strike") return "냉기를 실은 일격으로 적에게 추가 냉기 피해";
		if (a_key == "shock_strike") return "번개를 실은 일격으로 적에게 추가 전격 피해";
		if (a_key == "fire_dot") return "화염 낙인을 새겨 지속 화상 피해";
		if (a_key == "shock_dot") return "전류를 흘려보내 지속 전격 피해";
		if (a_key == "poison_dot") return "독이 혈류를 타고 번져 지속 중독 피해";
		if (a_key == "bleed_dot") return "열린 상처에서 피가 흘러 지속 출혈 피해";
		if (a_key == "siphon_bloom") return "흡수 꽃망울이 자원을 빨아 약화";
		if (a_key == "debuff_paralysis") return "암흑 충격으로 적의 몸을 순간 마비시킴";
		if (a_key == "debuff_calm") return "평화의 파동으로 적의 적대심을 일시 해제";
		if (a_key == "debuff_fear") return "공포의 파동으로 적을 도주하게 만듦";
		if (a_key == "debuff_frenzy") return "광란의 파동으로 적을 동료에게 돌아서게 만듦";
		if (a_key == "debuff_frostbite") return "서리 파편이 적의 움직임을 무겁게 둔화";
		if (a_key == "debuff_slow") return "묵직한 압박으로 적의 이동을 크게 둔화";
		if (a_key == "debuff_absorb_health") return "사신의 손길로 적의 생명력을 빼앗아 흡수";
		if (a_key == "debuff_magicka_drain") return "침묵의 파동으로 적의 마력을 고갈시킴";
		if (a_key == "debuff_stamina_drain") return "기근의 저주로 적의 체력을 소진시킴";
		if (a_key == "debuff_multi_element") return "복합 원소 파편이 한꺼번에 작렬";
		if (a_key == "aura_slow") return "타격마다 냉기 감속장을 씌워 둔화 유지";
		if (a_key == "aura_fire_shred") return "타격마다 화염 저항을 깎아 후속 화상 극대화";
		if (a_key == "aura_frost_shred") return "타격마다 냉기 저항을 깎아 후속 동결 극대화";
		if (a_key == "self_haste") return "가속으로 빈틈을 메워 기동 우위 확보";
		if (a_key == "self_weapon_speed") return "연타 가속으로 무기 공격 속도를 끌어올림";
		if (a_key == "self_weapon_fury") return "격노의 흐름으로 무기 공격 속도 상승";
		if (a_key == "self_judgment") return "심판의 기세를 실어 공격력을 대폭 끌어올림";
		if (a_key == "self_tempered_strike") return "피격의 열기를 공격력으로 전환해 반격 강화";
		if (a_key == "self_stamina_recall") return "전투 기억을 되살려 스태미나 회복 가속";
		if (a_key == "self_crit_chance") return "연계 리듬을 살려 치명타 확률 상승";
		if (a_key == "self_two_handed") return "양손 전투 감각을 극대화해 타격력 강화";
		if (a_key == "self_one_handed") return "서약의 힘으로 한손 전투 기술 강화";
		if (a_key == "self_marksman") return "예리한 집중으로 궁술 정확도 상승";
		if (a_key == "self_bow_speed") return "질풍의 흐름으로 활 공격 속도 상승";
		if (a_key == "self_block") return "굳건한 의지로 방패 기술 대폭 강화";
		if (a_key == "self_heavy_armor") return "신화적 가호로 중갑 방호 기술 강화";
		if (a_key == "self_light_armor") return "용의 비늘로 경갑 방호 기술 강화";
		if (a_key == "self_alteration") return "깨달음의 파동으로 변화마법 역량 강화";
		if (a_key == "self_ward") return "수호막으로 방어도를 일시 대폭 상승";
		if (a_key == "self_barrier") return "장벽으로 급사 구간을 얇게 끊어냄";
		if (a_key == "self_stone_skin") return "석화로 피부를 돌처럼 굳혀 강력 방어";
		if (a_key == "self_frost_ward") return "냉기 보호막으로 동결 피해를 차단";
		if (a_key == "self_shock_ward") return "전기 보호막으로 전격 피해를 크게 경감";
		if (a_key == "self_magic_shield") return "마력 방패로 마법 저항 전반을 강화";
		if (a_key == "self_frost_resist") return "화덕의 온기로 냉기 저항을 올려 방어";
		if (a_key == "self_shock_resist") return "대지 접지로 전격 저항을 올려 방어";
		if (a_key == "self_fire_resist") return "원칙의 불꽃으로 화염 저항을 크게 강화";
		if (a_key == "self_poison_resist") return "고대 서약으로 독과 질병에 대한 저항력 획득";
		if (a_key == "self_reflect_damage") return "가시 갑옷으로 받은 피해 일부를 공격자에게 반사";
		if (a_key == "self_ethereal") return "에테리얼 변신으로 잠시 무적 상태에 진입";
		if (a_key == "self_heal_rate") return "치유의 흐름으로 체력 회복 속도 상승";
		if (a_key == "self_heal_rate_mult") return "생명의 비로 체력 재생을 대폭 가속";
		if (a_key == "self_healing_wisdom") return "지혜의 파동으로 상처를 서서히 치유";
		if (a_key == "self_meditation") return "명상으로 전투 자원을 빠르게 회복";
		if (a_key == "self_magicka_regen") return "지식의 빛으로 마나 재생을 가속";
		if (a_key == "self_mana_surge") return "마나 쇄도로 마력 총량을 일시 증폭";
		if (a_key == "self_phase") return "위상 이동으로 즉시 이탈 각을 만듦";
		if (a_key == "self_phoenix") return "재점화로 반격 타이밍을 다시 잡음";
		if (a_key == "self_muffle") return "머플로 발소리를 지워 은신 진입 보조";
		if (a_key == "self_invisibility") return "투명화로 전투를 끊고 각 재설정";
		if (a_key == "self_smoke_escape") return "연막 속에서 이동 속도를 올려 긴급 이탈";
		if (a_key == "self_carry_weight") return "부의 축적으로 소지무게를 크게 늘림";
		if (a_key == "self_detect_life") return "광채로 주변 생명체를 감지해 정보 우위 확보";
		if (a_key == "kill_poison_burst") return "처치 시 독성 폭발로 주변 적에게 피해";
		if (a_key == "kill_restore") return "처치 시 생명력과 마나를 즉시 회복";
		if (a_key == "kill_poison_cloud") return "처치 시 독 구름이 퍼져 잔여 적 압박";
		if (a_key == "kill_turn_undead") return "처치 시 성스러운 빛이 주변 언데드를 퇴치";
		if (a_key == "lowhealth_fade") return "체력 위급 시 방어·저항 대폭 버프로 생존";
		if (a_key == "lowhealth_emergency") return "체력 위급 시 긴급 대량 회복으로 사선 탈출";
		if (a_key == "lowhealth_ironskin") return "체력 위급 시 강철 피부로 추가 피해 차단";
		if (a_key == "long_cd_bulwark") return "피격 시 대형 방어 결계 발동 (긴 재사용)";
		if (a_key == "long_cd_beast_rage") return "타격 시 야수의 격노로 폭발적 강화 (긴 재사용)";
		if (a_key == "thorns_reflect") return "받은 피해 일부를 공격자에게 되돌려 보복";
		if (a_key == "soul_trap") return "소울트랩으로 영혼을 묶어 포획";
		return {};
	}
}
