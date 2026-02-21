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
		{ "rw_spirit", "signature_spirit" },
		{ "rw_insight", "signature_insight" },
		{ "rw_call_to_arms", "signature_call_to_arms" },
		{ "rw_steel", "signature_steel" },
		{ "rw_fortitude", "signature_fortitude" },
		{ "rw_heart_of_the_oak", "signature_heart_of_the_oak" },
		{ "rw_infinity", "signature_infinity" },
		{ "rw_last_wish", "signature_last_wish" },
		{ "rw_exile", "signature_exile" },
		{ "rw_grief", "signature_grief" },
		{ "rw_breath_of_the_dying", "signature_breath_of_the_dying" },
		{ "rw_chains_of_honor", "signature_chains_of_honor" },
		{ "rw_enigma", "signature_enigma" },
		{ "rw_dream", "signature_dream" },
		{ "rw_faith", "signature_faith" },
		{ "rw_phoenix", "signature_phoenix" },
		{ "rw_doom", "signature_doom" },
		{ "rw_dragon", "self_flame_cloak" },
		{ "rw_hand_of_justice", "self_flame_cloak" },
		{ "rw_flickering_flame", "self_flame_cloak" },
		{ "rw_temper", "self_flame_cloak" },
		{ "rw_ice", "self_frost_cloak" },
		{ "rw_voice_of_reason", "self_frost_cloak" },
		{ "rw_hearth", "self_frost_cloak" },
		{ "rw_holy_thunder", "signature_holy_thunder" },
		{ "rw_zephyr", "signature_zephyr" },
		{ "rw_wind", "self_shock_cloak" },
		{ "rw_bulwark", "self_oakflesh" },
		{ "rw_cure", "self_oakflesh" },
		{ "rw_ancients_pledge", "signature_ancients_pledge" },
		{ "rw_lionheart", "self_oakflesh" },
		{ "rw_radiance", "self_oakflesh" },
		{ "rw_sanctuary", "self_stoneflesh" },
		{ "rw_duress", "self_stoneflesh" },
		{ "rw_peace", "self_stoneflesh" },
		{ "rw_myth", "self_stoneflesh" },
		{ "rw_bone", "signature_bone" },
		{ "rw_pride", "self_ironflesh" },
		{ "rw_stone", "self_ironflesh" },
		{ "rw_prudence", "self_ironflesh" },
		{ "rw_mist", "self_ironflesh" },
		{ "rw_metamorphosis", "self_ebonyflesh" },
		{ "rw_nadir", "signature_nadir" },
		{ "rw_delirium", "curse_frenzy" },
		{ "rw_chaos", "curse_frenzy" },
		{ "rw_malice", "signature_malice" },
		{ "rw_venom", "poison_bloom" },
		{ "rw_plague", "poison_bloom" },
		{ "rw_bramble", "poison_bloom" },
		{ "rw_black", "tar_bloom" },
		{ "rw_rift", "tar_bloom" },
		{ "rw_mosaic", "siphon_bloom" },
		{ "rw_obsession", "siphon_bloom" },
		{ "rw_white", "siphon_bloom" },
		{ "rw_lawbringer", "curse_fragile" },
		{ "rw_wrath", "curse_fragile" },
		{ "rw_kingslayer", "curse_fragile" },
		{ "rw_principle", "curse_fragile" },
		{ "rw_death", "curse_slow_attack" },
		{ "rw_famine", "curse_slow_attack" },
		{ "rw_leaf", "signature_leaf" },
		{ "rw_destruction", "fire_strike" },
		{ "rw_crescent_moon", "shock_strike" },
		{ "rw_beast", "self_haste" },
		{ "rw_hustle_w", "self_haste" },
		{ "rw_harmony", "self_haste" },
		{ "rw_fury", "self_haste" },
		{ "rw_unbending_will", "self_haste" },
		{ "rw_passion", "self_haste" },
		{ "rw_stealth", "signature_stealth" },
		{ "rw_smoke", "self_muffle" },
		{ "rw_treachery", "self_muffle" },
		{ "rw_gloom", "self_invisibility" },
		{ "rw_wealth", "soul_trap" },
		{ "rw_obedience", "soul_trap" },
		{ "rw_honor", "soul_trap" },
		{ "rw_eternity", "soul_trap" },
		{ "rw_memory", "self_meditation" },
		{ "rw_wisdom", "self_meditation" },
		{ "rw_lore", "self_meditation" },
		{ "rw_melody", "self_meditation" },
		{ "rw_enlightenment", "self_meditation" },
		{ "rw_pattern", "signature_pattern" },
		{ "rw_strength", "signature_strength" },
		{ "rw_kings_grace", "signature_kings_grace" },
		{ "rw_edge", "signature_edge" },
		{ "rw_oath", "adaptive_strike" },
		{ "rw_silence", "adaptive_exposure" },
		{ "rw_brand", "adaptive_exposure" },
		{ "rw_hustle_a", "self_phase" },
		{ "rw_splendor", "self_phase" },
		{ "rw_rhyme", "self_phase" },
		{ "rw_rain", "self_phoenix" },
		{ "rw_ground", "self_phoenix" },
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
		// Batch 1 (12 keys): handcrafted flavor lines to avoid one-size-fits-all template tone.
		if (a_key == "adaptive_strike") return "적의 가장 약한 원소 저항 축을 자동 추적해 관통 타격";
		if (a_key == "adaptive_exposure") return "적의 가장 높은 저항 축을 벗겨내 후속 원소 피해 창출";
		if (a_key == "signature_nadir") return "대상을 공포에 몰아넣어 전열을 무너뜨림";
		if (a_key == "signature_steel") return "단련된 강철 참격으로 교전 초반 주도권 확보";
		if (a_key == "signature_malice") return "상처를 찢어 독성 손상을 길게 누적";
		if (a_key == "signature_stealth") return "위기 순간 시야에서 이탈해 재배치 각 확보";
		if (a_key == "signature_leaf") return "타격 지점에 불씨를 심어 점화 폭발 유도";
		if (a_key == "signature_ancients_pledge") return "고대의 서약 결계로 급사 구간 완충";
		if (a_key == "signature_holy_thunder") return "성전 번개를 떨궈 즉시 충격 피해 전개";
		if (a_key == "signature_zephyr") return "질풍 흐름을 타고 기동 연계 타격";
		if (a_key == "signature_pattern") return "연계 리듬을 살려 콤보 압박 유지";
		if (a_key == "signature_kings_grace") return "성스러운 검격으로 결투형 마무리 일격";
		// Batch 2 (12 keys): signature mid-tier keys with distinct combat intent wording.
		if (a_key == "signature_strength") return "무게를 실은 강타로 방어 자세 붕괴";
		if (a_key == "signature_edge") return "날 선 사격으로 원거리 압박 강화";
		if (a_key == "signature_grief") return "빈틈 없이 몰아치는 초고속 참격 연타";
		if (a_key == "signature_infinity") return "저항이 높은 축을 먼저 찢어 약점 개방";
		if (a_key == "signature_enigma") return "위험 순간 위상 전환으로 즉시 이탈";
		if (a_key == "signature_call_to_arms") return "함성으로 아군 페이스를 끌어올려 공방 동시 강화";
		if (a_key == "signature_spirit") return "집중력 흐름을 올려 전투 리듬 안정화";
		if (a_key == "signature_insight") return "명상 파동으로 자원 고갈 구간을 완화";
		if (a_key == "signature_fortitude") return "강철 의지 버프로 난전 생존력 상승";
		if (a_key == "signature_heart_of_the_oak") return "견고한 저항 축부터 깎아 후속 딜 창출";
		if (a_key == "signature_last_wish") return "집요한 심판 효과로 장기전 압박 유지";
		if (a_key == "signature_exile") return "치명 구간에 긴급 장벽을 세워 급사 방지";
		if (a_key == "signature_breath_of_the_dying") return "죽음의 숨결로 사신의 연타를 폭주시킴";
		if (a_key == "signature_chains_of_honor") return "명예의 사슬로 성역의 보호를 단단히 고정";
		if (a_key == "signature_dream") return "번개 공명을 축적해 전격 일격을 터뜨림";
		if (a_key == "signature_faith") return "광신의 돌격으로 기동과 생존을 동시에 끌어올림";
		if (a_key == "signature_phoenix") return "불사조의 역류로 반격의 불꽃을 되살림";
		if (a_key == "signature_doom") return "차가운 심판으로 적의 흐름을 얼려 끊어냄";
		if (a_key == "signature_bone") return "뼈의 계약으로 사역마 전열을 보강";
		if (a_key == "fire_strike") return "화염 참격으로 적에게 추가 화염 타격";
		if (a_key == "frost_strike") return "냉기 참격으로 적에게 추가 냉기 타격";
		if (a_key == "shock_strike") return "번개 참격으로 적에게 추가 전격 타격";
		if (a_key == "poison_bloom") return "독성 꽃망울이 터져 지속 피해가 번짐";
		if (a_key == "tar_bloom") return "끈적한 타르가 번져 둔화를 유지";
		if (a_key == "siphon_bloom") return "흡수 꽃망울이 자원을 빨아 약화";
		if (a_key == "curse_fragile") return "취약의 저주로 적의 몸을 물러지게 만듦";
		if (a_key == "curse_slow_attack") return "둔화 저주로 적의 공격 템포를 꺾음";
		if (a_key == "curse_fear") return "공포 저주로 적의 발을 흩뜨려 공간 확보";
		if (a_key == "curse_frenzy") return "광란 저주로 적을 난동에 빠뜨림";
		if (a_key == "self_haste") return "가속으로 빈틈을 메워 기동 우위 확보";
		if (a_key == "self_ward") return "수호막으로 한 번 버틸 생존 창 생성";
		if (a_key == "self_barrier") return "장벽으로 급사 구간을 얇게 끊어냄";
		if (a_key == "self_meditation") return "명상으로 전투 자원을 빠르게 회복";
		if (a_key == "self_phase") return "위상 이동으로 즉시 이탈 각을 만듦";
		if (a_key == "self_phoenix") return "재점화로 반격 타이밍을 다시 잡음";
		if (a_key == "self_flame_cloak") return "화염 망토로 근접에서 지속 압박";
		if (a_key == "self_frost_cloak") return "냉기 망토로 근접에서 지속 억제";
		if (a_key == "self_shock_cloak") return "번개 망토로 근접에서 전격 압박";
		if (a_key == "self_oakflesh") return "오크플레시로 피부를 단단히 굳힘";
		if (a_key == "self_stoneflesh") return "스톤플레시로 돌처럼 단단해짐";
		if (a_key == "self_ironflesh") return "아이언플레시로 강철 피부를 두름";
		if (a_key == "self_ebonyflesh") return "에보니플레시로 흑단 갑주를 두름";
		if (a_key == "self_muffle") return "머플로 발소리를 지워 은신 진입 보조";
		if (a_key == "self_invisibility") return "투명화로 전투를 끊고 각 재설정";
		if (a_key == "soul_trap") return "소울트랩으로 영혼을 묶어 포획";
		return {};
	}
}
