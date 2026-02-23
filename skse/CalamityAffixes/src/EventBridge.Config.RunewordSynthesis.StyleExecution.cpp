#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Config.RunewordSynthesis.SpellSet.h"

#include <algorithm>

namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] std::chrono::milliseconds ToDurationMs(float a_seconds)
		{
			const float clamped = std::max(0.0f, a_seconds);
			return std::chrono::milliseconds(static_cast<std::int64_t>(clamped * 1000.0f));
		}
	}

	bool EventBridge::ConfigureSyntheticAdaptiveSpell(
		AffixRuntime& a_out,
		float a_runeScale,
		AdaptiveElementMode a_mode,
		RE::SpellItem* a_fire,
		RE::SpellItem* a_frost,
		RE::SpellItem* a_shock,
		float a_procBase,
		float a_procPerRune,
		float a_procMin,
		float a_procMax,
		float a_icdBase,
		float a_icdPerRune,
		float a_icdMin,
		float a_icdMax,
		float a_scaleBase,
		float a_scalePerRune,
		float a_perTargetIcdSeconds,
		Trigger a_trigger) const
	{
		if (!a_fire && !a_frost && !a_shock) {
			return false;
		}

		a_out.trigger = a_trigger;
		a_out.procChancePct = std::clamp(a_procBase + a_runeScale * a_procPerRune, a_procMin, a_procMax);
		a_out.icd = ToDurationMs(std::clamp(a_icdBase - a_runeScale * a_icdPerRune, a_icdMin, a_icdMax));
		if (a_perTargetIcdSeconds > 0.0f) {
			a_out.perTargetIcd = ToDurationMs(a_perTargetIcdSeconds);
		}
		a_out.action.type = ActionType::kCastSpellAdaptiveElement;
		a_out.action.adaptiveMode = a_mode;
		a_out.action.adaptiveFire = a_fire;
		a_out.action.adaptiveFrost = a_frost;
		a_out.action.adaptiveShock = a_shock;
		a_out.action.noHitEffectArt = false;

		if (a_scaleBase > 0.0f || a_scalePerRune > 0.0f) {
			a_out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
			a_out.action.magnitudeScaling.mult = std::clamp(a_scaleBase + a_runeScale * a_scalePerRune, 0.0f, 0.25f);
			a_out.action.magnitudeScaling.spellBaseAsMin = true;
		}

		return true;
	}

	bool EventBridge::ConfigureSyntheticSingleTargetSpell(
		AffixRuntime& a_out,
		float a_runeScale,
		RE::SpellItem* a_spell,
		float a_procBase,
		float a_procPerRune,
		float a_procMin,
		float a_procMax,
		float a_icdBase,
		float a_icdPerRune,
		float a_icdMin,
		float a_icdMax,
		float a_scaleBase,
		float a_scalePerRune,
		float a_perTargetIcdSeconds,
		Trigger a_trigger) const
	{
		if (!a_spell) {
			return false;
		}

		a_out.trigger = a_trigger;
		a_out.procChancePct = std::clamp(a_procBase + a_runeScale * a_procPerRune, a_procMin, a_procMax);
		a_out.icd = ToDurationMs(std::clamp(a_icdBase - a_runeScale * a_icdPerRune, a_icdMin, a_icdMax));
		if (a_perTargetIcdSeconds > 0.0f) {
			a_out.perTargetIcd = ToDurationMs(a_perTargetIcdSeconds);
		}
		a_out.action.type = ActionType::kCastSpell;
		a_out.action.spell = a_spell;
		a_out.action.applyToSelf = false;
		a_out.action.noHitEffectArt = true;

		if (a_scaleBase > 0.0f || a_scalePerRune > 0.0f) {
			a_out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
			a_out.action.magnitudeScaling.mult = std::clamp(a_scaleBase + a_runeScale * a_scalePerRune, 0.0f, 0.25f);
			a_out.action.magnitudeScaling.spellBaseAsMin = true;
		}

		return true;
	}

	bool EventBridge::ConfigureSyntheticSelfBuff(
		AffixRuntime& a_out,
		float a_runeScale,
		RE::SpellItem* a_spell,
		Trigger a_trigger,
		float a_procBase,
		float a_procPerRune,
		float a_procMin,
		float a_procMax,
		float a_icdBase,
		float a_icdPerRune,
		float a_icdMin,
		float a_icdMax) const
	{
		if (!a_spell) {
			return false;
		}

		a_out.trigger = a_trigger;
		a_out.procChancePct = std::clamp(a_procBase + a_runeScale * a_procPerRune, a_procMin, a_procMax);
		a_out.icd = ToDurationMs(std::clamp(a_icdBase - a_runeScale * a_icdPerRune, a_icdMin, a_icdMax));
		a_out.action.type = ActionType::kCastSpell;
		a_out.action.spell = a_spell;
		a_out.action.applyToSelf = true;
		a_out.action.noHitEffectArt = true;
		return true;
	}

	bool EventBridge::ConfigureSyntheticSummonSpell(
		AffixRuntime& a_out,
		float a_runeScale,
		RE::SpellItem* a_spell,
		Trigger a_trigger,
		float a_procBase,
		float a_procPerRune,
		float a_procMin,
		float a_procMax,
		float a_icdBase,
		float a_icdPerRune,
		float a_icdMin,
		float a_icdMax) const
	{
		if (!a_spell) {
			return false;
		}

		a_out.trigger = a_trigger;
		a_out.procChancePct = std::clamp(a_procBase + a_runeScale * a_procPerRune, a_procMin, a_procMax);
		a_out.icd = ToDurationMs(std::clamp(a_icdBase - a_runeScale * a_icdPerRune, a_icdMin, a_icdMax));
		a_out.action.type = ActionType::kCastSpell;
		a_out.action.spell = a_spell;
		a_out.action.applyToSelf = true;
		a_out.action.noHitEffectArt = true;
		return true;
	}

	bool EventBridge::ApplyLegacySyntheticRunewordFallback(
		AffixRuntime& a_out,
		const RunewordRecipe& a_recipe,
		std::uint32_t a_runeCount,
		const RunewordSynthesis::SpellSet& a_spellSet) const
	{
		if (a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kArmor) {
			RE::SpellItem* spell = nullptr;
			switch (a_runeCount % 4u) {
			case 0u:
				spell = a_spellSet.spellWard;
				break;
			case 1u:
				spell = a_spellSet.spellPhase;
				break;
			case 2u:
				spell = a_spellSet.spellMeditation;
				break;
			default:
				spell = a_spellSet.spellBarrier;
				break;
			}
			if (!spell) {
				spell = a_spellSet.spellWard ?
							a_spellSet.spellWard :
							(a_spellSet.spellPhase ?
								 a_spellSet.spellPhase :
								 (a_spellSet.spellMeditation ? a_spellSet.spellMeditation : a_spellSet.spellBarrier));
			}
			if (!spell) {
				return false;
			}
			a_out.trigger = Trigger::kIncomingHit;
			a_out.procChancePct = std::clamp(14.0f + static_cast<float>(a_runeCount) * 1.9f, 14.0f, 28.0f);
			const float icdSeconds = std::clamp(20.0f - static_cast<float>(a_runeCount) * 1.2f, 8.0f, 20.0f);
			a_out.icd = ToDurationMs(icdSeconds);
			a_out.action.type = ActionType::kCastSpell;
			a_out.action.spell = spell;
			a_out.action.applyToSelf = true;
			a_out.action.noHitEffectArt = true;
			return true;
		}

		if (a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kWeapon) {
			if (!a_spellSet.dynamicFire && !a_spellSet.dynamicFrost && !a_spellSet.dynamicShock) {
				return false;
			}
			a_out.trigger = Trigger::kHit;
			a_out.procChancePct = std::clamp(20.0f + static_cast<float>(a_runeCount) * 2.4f, 20.0f, 36.0f);
			const float icdSeconds = std::clamp(1.50f - static_cast<float>(a_runeCount) * 0.16f, 0.35f, 1.25f);
			a_out.icd = ToDurationMs(icdSeconds);
			a_out.action.type = ActionType::kCastSpellAdaptiveElement;
			a_out.action.adaptiveMode = AdaptiveElementMode::kWeakestResist;
			a_out.action.adaptiveFire = a_spellSet.dynamicFire;
			a_out.action.adaptiveFrost = a_spellSet.dynamicFrost;
			a_out.action.adaptiveShock = a_spellSet.dynamicShock;
			a_out.action.noHitEffectArt = false;
			a_out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
			a_out.action.magnitudeScaling.mult =
				std::clamp(0.08f + static_cast<float>(a_runeCount) * 0.012f, 0.08f, 0.18f);
			a_out.action.magnitudeScaling.spellBaseAsMin = true;
			return true;
		}

		if (!a_spellSet.shredFire && !a_spellSet.shredFrost && !a_spellSet.shredShock) {
			return false;
		}
		a_out.trigger = Trigger::kHit;
		a_out.procChancePct = std::clamp(18.0f + static_cast<float>(a_runeCount) * 2.0f, 18.0f, 34.0f);
		const float icdSeconds = std::clamp(3.2f - static_cast<float>(a_runeCount) * 0.45f, 0.9f, 3.2f);
		a_out.icd = ToDurationMs(icdSeconds);
		a_out.action.type = ActionType::kCastSpellAdaptiveElement;
		a_out.action.adaptiveMode = AdaptiveElementMode::kWeakestResist;
		a_out.action.adaptiveFire = a_spellSet.shredFire;
		a_out.action.adaptiveFrost = a_spellSet.shredFrost;
		a_out.action.adaptiveShock = a_spellSet.shredShock;
		a_out.action.noHitEffectArt = false;
		return true;
	}

	bool EventBridge::ConfigureSyntheticRunewordStyle(
		AffixRuntime& a_out,
		SyntheticRunewordStyle& a_inOutStyle,
		const RunewordRecipe& a_recipe,
		std::uint32_t a_runeCount,
		const RunewordSynthesis::SpellSet& a_spellSet) const
	{
		const float runeScale = static_cast<float>(a_runeCount);
		const bool armorRecommended =
			(a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kArmor);
		const Trigger summonTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;
		const Trigger offensiveTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;
		const Trigger auraTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;

		bool ready = false;
		switch (a_inOutStyle) {
		case SyntheticRunewordStyle::kAdaptiveStrike:
			ready = ConfigureSyntheticAdaptiveSpell(a_out, runeScale, 
				AdaptiveElementMode::kWeakestResist,
				a_spellSet.dynamicFire,
				a_spellSet.dynamicFrost,
				a_spellSet.dynamicShock,
				20.0f,
				2.2f,
				20.0f,
				36.0f,
				1.45f,
				0.16f,
				0.35f,
				1.35f,
				0.08f,
				0.012f,
				0.0f);
			break;
		case SyntheticRunewordStyle::kAdaptiveExposure:
			ready = ConfigureSyntheticAdaptiveSpell(a_out, runeScale, 
				AdaptiveElementMode::kStrongestResist,
				a_spellSet.shredFire,
				a_spellSet.shredFrost,
				a_spellSet.shredShock,
				16.0f,
				1.6f,
				16.0f,
				30.0f,
				3.2f,
				0.35f,
				0.95f,
				3.2f,
				0.0f,
				0.0f,
				8.0f);
			break;
		case SyntheticRunewordStyle::kSignatureGrief:
			ready = ConfigureSyntheticAdaptiveSpell(a_out, runeScale, 
				AdaptiveElementMode::kWeakestResist,
				a_spellSet.dynamicFire,
				a_spellSet.dynamicFrost,
				a_spellSet.dynamicShock,
				26.0f,
				2.4f,
				26.0f,
				40.0f,
				0.95f,
				0.10f,
				0.30f,
				0.95f,
				0.11f,
				0.015f,
				0.0f);
			break;
		case SyntheticRunewordStyle::kSignatureInfinity:
			ready = ConfigureSyntheticAdaptiveSpell(a_out, runeScale, 
				AdaptiveElementMode::kStrongestResist,
				a_spellSet.shredFire,
				a_spellSet.shredFrost,
				a_spellSet.shredShock,
				22.0f,
				1.8f,
				22.0f,
				34.0f,
				2.4f,
				0.24f,
				0.85f,
				2.4f,
				0.0f,
				0.0f,
				4.5f);
			break;
		case SyntheticRunewordStyle::kSignatureEnigma:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellPhase ? a_spellSet.spellPhase : (a_spellSet.spellCustomInvisibility ? a_spellSet.spellCustomInvisibility : a_spellSet.spellWard),
				Trigger::kIncomingHit,
				20.0f,
				1.8f,
				20.0f,
				34.0f,
				10.0f,
				0.65f,
				4.0f,
				10.0f);
			break;
		case SyntheticRunewordStyle::kSignatureCallToArms:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellHaste ? a_spellSet.spellHaste : (a_spellSet.spellWard ? a_spellSet.spellWard : a_spellSet.spellPhase),
				Trigger::kHit,
				24.0f,
				1.7f,
				24.0f,
				36.0f,
				9.0f,
				0.55f,
				3.0f,
				9.0f);
			break;
		case SyntheticRunewordStyle::kFireStrike:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.dynamicFire,
				18.0f,
				1.8f,
				18.0f,
				34.0f,
				1.2f,
				0.12f,
				0.35f,
				1.2f,
				0.06f,
				0.010f,
				0.0f);
			break;
		case SyntheticRunewordStyle::kFrostStrike:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.dynamicFrost,
				18.0f,
				1.8f,
				18.0f,
				34.0f,
				1.25f,
				0.13f,
				0.40f,
				1.25f,
				0.06f,
				0.010f,
				0.0f);
			break;
		case SyntheticRunewordStyle::kShockStrike:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellArcLightning ? a_spellSet.spellArcLightning : a_spellSet.dynamicShock,
				19.0f,
				1.9f,
				19.0f,
				35.0f,
				1.05f,
				0.11f,
				0.30f,
				1.05f,
				0.07f,
				0.011f,
				0.0f);
			break;
		case SyntheticRunewordStyle::kPoisonBloom:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellDotPoison,
				12.0f,
				1.6f,
				12.0f,
				26.0f,
				2.6f,
				0.26f,
				0.9f,
				2.6f,
				0.0f,
				0.0f,
				4.0f);
			break;
		case SyntheticRunewordStyle::kTarBloom:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellDotTar,
				12.0f,
				1.5f,
				12.0f,
				24.0f,
				2.8f,
				0.30f,
				0.9f,
				2.8f,
				0.0f,
				0.0f,
				4.0f);
			break;
		case SyntheticRunewordStyle::kSiphonBloom:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellDotSiphon,
				11.0f,
				1.5f,
				11.0f,
				24.0f,
				2.9f,
				0.30f,
				1.0f,
				2.9f,
				0.0f,
				0.0f,
				5.0f);
			break;
		case SyntheticRunewordStyle::kCurseFragile:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellChaosFragile ? a_spellSet.spellChaosFragile : a_spellSet.spellChaosSunder,
				15.0f,
				1.6f,
				15.0f,
				29.0f,
				4.4f,
				0.45f,
				1.6f,
				4.4f,
				0.0f,
				0.0f,
				8.0f,
				offensiveTrigger);
			break;
		case SyntheticRunewordStyle::kCurseSlowAttack:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellChaosSlowAttack ? a_spellSet.spellChaosSlowAttack : a_spellSet.spellChaosSunder,
				16.0f,
				1.6f,
				16.0f,
				30.0f,
				4.0f,
				0.40f,
				1.4f,
				4.0f,
				0.0f,
				0.0f,
				7.0f,
				offensiveTrigger);
			break;
		case SyntheticRunewordStyle::kCurseFear:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellVanillaFear ? a_spellSet.spellVanillaFear : a_spellSet.spellChaosSlowAttack,
				13.0f,
				1.2f,
				13.0f,
				23.0f,
				6.0f,
				0.6f,
				2.4f,
				6.0f,
				0.0f,
				0.0f,
				8.0f,
				offensiveTrigger);
			break;
		case SyntheticRunewordStyle::kCurseFrenzy:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellVanillaFrenzy ? a_spellSet.spellVanillaFrenzy : a_spellSet.spellChaosFragile,
				11.0f,
				1.1f,
				11.0f,
				21.0f,
				6.8f,
				0.7f,
				2.6f,
				6.8f,
				0.0f,
				0.0f,
				9.0f,
				offensiveTrigger);
			break;
		case SyntheticRunewordStyle::kSelfHaste:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, a_spellSet.spellHaste, Trigger::kHit, 18.0f, 1.5f, 18.0f, 32.0f, 7.5f, 0.75f, 2.4f, 7.5f);
			break;
		case SyntheticRunewordStyle::kSelfWard:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, a_spellSet.spellWard, Trigger::kIncomingHit, 15.0f, 1.7f, 15.0f, 28.0f, 19.0f, 1.1f, 8.0f, 19.0f);
			break;
		case SyntheticRunewordStyle::kSelfBarrier:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, a_spellSet.spellBarrier, Trigger::kIncomingHit, 15.0f, 1.7f, 15.0f, 28.0f, 18.0f, 1.0f, 7.5f, 18.0f);
			break;
		case SyntheticRunewordStyle::kSelfMeditation:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, a_spellSet.spellMeditation, Trigger::kIncomingHit, 16.0f, 1.5f, 16.0f, 27.0f, 17.0f, 0.9f, 7.0f, 17.0f);
			break;
		case SyntheticRunewordStyle::kSelfPhase:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, a_spellSet.spellPhase, Trigger::kIncomingHit, 14.0f, 1.4f, 14.0f, 24.0f, 16.0f, 0.8f, 7.0f, 16.0f);
			break;
		case SyntheticRunewordStyle::kSelfPhoenix:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellPhoenix ? a_spellSet.spellPhoenix : a_spellSet.spellPhase,
				Trigger::kIncomingHit,
				14.0f,
				1.4f,
				14.0f,
				25.0f,
				16.0f,
				0.8f,
				6.5f,
				16.0f);
			break;
		case SyntheticRunewordStyle::kSelfFlameCloak:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaFlameCloak ? a_spellSet.spellVanillaFlameCloak : (a_spellSet.spellPhoenix ? a_spellSet.spellPhoenix : a_spellSet.spellPhase),
				auraTrigger,
				12.0f,
				1.3f,
				12.0f,
				23.0f,
				14.0f,
				0.7f,
				4.5f,
				14.0f);
			break;
		case SyntheticRunewordStyle::kSelfFrostCloak:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaFrostCloak ? a_spellSet.spellVanillaFrostCloak : (a_spellSet.spellWard ? a_spellSet.spellWard : a_spellSet.spellPhase),
				auraTrigger,
				12.0f,
				1.3f,
				12.0f,
				23.0f,
				14.0f,
				0.7f,
				4.5f,
				14.0f);
			break;
		case SyntheticRunewordStyle::kSelfShockCloak:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaShockCloak ? a_spellSet.spellVanillaShockCloak : (a_spellSet.spellBarrier ? a_spellSet.spellBarrier : a_spellSet.spellPhase),
				auraTrigger,
				12.0f,
				1.3f,
				12.0f,
				23.0f,
				14.0f,
				0.7f,
				4.5f,
				14.0f);
			break;
		case SyntheticRunewordStyle::kSelfOakflesh:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaOakflesh ? a_spellSet.spellVanillaOakflesh : (a_spellSet.spellWard ? a_spellSet.spellWard : a_spellSet.spellPhase),
				auraTrigger,
				14.0f,
				1.4f,
				14.0f,
				25.0f,
				16.0f,
				0.9f,
				6.0f,
				16.0f);
			break;
		case SyntheticRunewordStyle::kSelfStoneflesh:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaStoneflesh ? a_spellSet.spellVanillaStoneflesh : (a_spellSet.spellBarrier ? a_spellSet.spellBarrier : a_spellSet.spellPhase),
				auraTrigger,
				14.0f,
				1.4f,
				14.0f,
				25.0f,
				16.0f,
				0.9f,
				6.0f,
				16.0f);
			break;
		case SyntheticRunewordStyle::kSelfIronflesh:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaIronflesh ? a_spellSet.spellVanillaIronflesh : (a_spellSet.spellBarrier ? a_spellSet.spellBarrier : a_spellSet.spellPhase),
				auraTrigger,
				13.0f,
				1.4f,
				13.0f,
				24.0f,
				17.0f,
				0.9f,
				6.0f,
				17.0f);
			break;
		case SyntheticRunewordStyle::kSelfEbonyflesh:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaEbonyflesh ? a_spellSet.spellVanillaEbonyflesh : (a_spellSet.spellWard ? a_spellSet.spellWard : a_spellSet.spellPhase),
				auraTrigger,
				12.0f,
				1.2f,
				12.0f,
				22.0f,
				18.0f,
				1.0f,
				7.0f,
				18.0f);
			break;
		case SyntheticRunewordStyle::kSelfMuffle:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellVanillaMuffle ? a_spellSet.spellVanillaMuffle : a_spellSet.spellPhase,
				Trigger::kIncomingHit,
				13.0f,
				1.4f,
				13.0f,
				24.0f,
				20.0f,
				1.1f,
				8.0f,
				20.0f);
			break;
		case SyntheticRunewordStyle::kSelfInvisibility:
			ready = ConfigureSyntheticSelfBuff(a_out, runeScale, 
				a_spellSet.spellCustomInvisibility ? a_spellSet.spellCustomInvisibility : a_spellSet.spellPhase,
				Trigger::kIncomingHit,
				8.0f,
				0.9f,
				8.0f,
				15.0f,
				28.0f,
				1.6f,
				14.0f,
				28.0f);
			break;
		case SyntheticRunewordStyle::kSoulTrap:
			ready = ConfigureSyntheticSingleTargetSpell(a_out, runeScale, 
				a_spellSet.spellVanillaSoulTrap ? a_spellSet.spellVanillaSoulTrap : a_spellSet.spellChaosFragile,
				16.0f,
				1.4f,
				16.0f,
				27.0f,
				7.0f,
				0.7f,
				2.5f,
				7.0f,
				0.0f,
				0.0f,
				6.0f,
				offensiveTrigger);
			break;
		case SyntheticRunewordStyle::kSummonFamiliar:
			ready = ConfigureSyntheticSummonSpell(
				a_out,
				runeScale,
				a_spellSet.spellSummonFamiliar,
				summonTrigger,
				8.0f,
				0.8f,
				8.0f,
				14.0f,
				28.0f,
				1.6f,
				18.0f,
				30.0f);
			break;
		case SyntheticRunewordStyle::kSummonFlameAtronach:
			ready = ConfigureSyntheticSummonSpell(
				a_out,
				runeScale,
				a_spellSet.spellSummonFlameAtronach,
				summonTrigger,
				7.0f,
				0.7f,
				7.0f,
				13.0f,
				30.0f,
				1.5f,
				20.0f,
				32.0f);
			break;
		case SyntheticRunewordStyle::kSummonFrostAtronach:
			ready = ConfigureSyntheticSummonSpell(
				a_out,
				runeScale,
				a_spellSet.spellSummonFrostAtronach,
				summonTrigger,
				6.5f,
				0.7f,
				6.5f,
				12.0f,
				32.0f,
				1.5f,
				21.0f,
				34.0f);
			break;
		case SyntheticRunewordStyle::kSummonStormAtronach:
			ready = ConfigureSyntheticSummonSpell(
				a_out,
				runeScale,
				a_spellSet.spellSummonStormAtronach,
				summonTrigger,
				6.0f,
				0.6f,
				6.0f,
				11.0f,
				34.0f,
				1.6f,
				22.0f,
				36.0f);
			break;
		case SyntheticRunewordStyle::kSummonDremoraLord:
			ready = ConfigureSyntheticSummonSpell(
				a_out,
				runeScale,
				a_spellSet.spellSummonDremoraLord,
				summonTrigger,
				4.0f,
				0.5f,
				4.0f,
				8.0f,
				38.0f,
				2.0f,
				26.0f,
				40.0f);
			break;
		case SyntheticRunewordStyle::kLegacyFallback:
		default:
			ready = false;
			break;
		}

		if (!ready && ApplyLegacySyntheticRunewordFallback(a_out, a_recipe, a_runeCount, a_spellSet)) {
			ready = true;
			a_inOutStyle = SyntheticRunewordStyle::kLegacyFallback;
		}

		return ready;
	}
}
