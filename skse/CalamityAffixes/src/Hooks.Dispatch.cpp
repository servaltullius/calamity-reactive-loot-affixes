#include "Hooks.Dispatch.h"

#include <array>
#include <cmath>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <SKSE/SKSE.h>

#include "CalamityAffixes/HitDataUtil.h"

namespace CalamityAffixes::Hooks::detail
{
	namespace
	{
		[[nodiscard]] RE::BGSArtObject* ResolveFirstHitOrEnchantArt(const RE::SpellItem* a_spell) noexcept
		{
			if (!a_spell) {
				return nullptr;
			}

			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				auto* hitEffectArt = effect->baseEffect->data.hitEffectArt;
				if (hitEffectArt) {
					return hitEffectArt;
				}

				auto* enchantEffectArt = effect->baseEffect->data.enchantEffectArt;
				if (enchantEffectArt) {
					return enchantEffectArt;
				}
			}

			return nullptr;
		}

		[[nodiscard]] RE::BGSArtObject* ResolveSafeCastOnCritProcFeedbackArt() noexcept
		{
			static RE::BGSArtObject* cached = []() noexcept -> RE::BGSArtObject* {
				constexpr std::array<RE::FormID, 2> kFallbackSpellFormIDs{
					0x00012FD0,
					0x0002B96C
				};
				for (const auto formID : kFallbackSpellFormIDs) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (!spell) {
						continue;
					}
					if (auto* art = ResolveFirstHitOrEnchantArt(spell); art) {
						return art;
					}
				}
				return nullptr;
			}();

			return cached;
		}

		[[nodiscard]] RE::BGSArtObject* ResolveCastOnCritProcFeedbackArt(const RE::SpellItem* a_spell) noexcept
		{
			(void)a_spell;
			return ResolveSafeCastOnCritProcFeedbackArt();
		}

		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> s_nextAllowedByTarget;
		std::mutex s_nextAllowedByTargetMutex;

		[[nodiscard]] bool ShouldPlayCastOnCritProcFeedbackVfx(
			RE::Actor* a_target,
			std::chrono::steady_clock::time_point a_now) noexcept
		{
			if (!a_target || a_target->IsDead()) {
				return false;
			}

			constexpr auto kPerTargetIcd = std::chrono::milliseconds(120);
			constexpr std::size_t kMaxEntries = 512u;
			const std::scoped_lock lock(s_nextAllowedByTargetMutex);

			const auto targetFormID = a_target->GetFormID();
			if (const auto it = s_nextAllowedByTarget.find(targetFormID); it != s_nextAllowedByTarget.end() && a_now < it->second) {
				return false;
			}

			s_nextAllowedByTarget[targetFormID] = a_now + kPerTargetIcd;

			if (s_nextAllowedByTarget.size() > kMaxEntries) {
				for (auto it = s_nextAllowedByTarget.begin(); it != s_nextAllowedByTarget.end();) {
					if (a_now >= it->second) {
						it = s_nextAllowedByTarget.erase(it);
					} else {
						++it;
					}
				}
			}

			return true;
		}

		void PlayCastOnCritProcFeedbackVfxSafe(
			RE::Actor* a_target,
			const RE::SpellItem* a_spell,
			std::chrono::steady_clock::time_point a_now) noexcept
		{
			if (!ShouldPlayCastOnCritProcFeedbackVfx(a_target, a_now)) {
				return;
			}

			auto* art = ResolveCastOnCritProcFeedbackArt(a_spell);
			if (!art) {
				return;
			}

			constexpr float kFeedbackDurationSec = 0.06f;
			a_target->InstantiateHitArt(
				art,
				kFeedbackDurationSec,
				nullptr,
				false,
				false,
				nullptr,
				false);
		}

		[[nodiscard]] RE::FormID ResolveCastOnCritProcFeedbackSoundFormID(
			const RE::SpellItem* a_spell,
			RE::MagicSystem::SoundID a_soundID) noexcept
		{
			if (!a_spell) {
				return 0;
			}

			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				for (const auto& soundPair : effect->baseEffect->effectSounds) {
					if (soundPair.id != a_soundID || !soundPair.sound) {
						continue;
					}

					return soundPair.sound->GetFormID();
				}
			}

			return 0;
		}

		[[nodiscard]] RE::FormID ResolveCastOnCritProcFeedbackSoundFormID(const RE::SpellItem* a_spell) noexcept
		{
			const auto hitSoundFormID = ResolveCastOnCritProcFeedbackSoundFormID(a_spell, RE::MagicSystem::SoundID::kHit);
			if (hitSoundFormID != 0) {
				return hitSoundFormID;
			}
			return ResolveCastOnCritProcFeedbackSoundFormID(a_spell, RE::MagicSystem::SoundID::kRelease);
		}

		void PlayCastOnCritProcFeedbackSfx(const RE::SpellItem* a_spell) noexcept
		{
			auto* audioManager = RE::BSAudioManager::GetSingleton();
			if (!audioManager) {
				return;
			}

			const auto soundFormID = ResolveCastOnCritProcFeedbackSoundFormID(a_spell);
			if (soundFormID == 0) {
				return;
			}

			audioManager->Play(soundFormID);
		}

		struct ProcDispatchRecord
		{
			std::chrono::steady_clock::time_point dispatchTime{};
		};

		constexpr float kStaleDamageMinExpected = 5.0f;
		constexpr float kStaleDamageRatioThreshold = 0.25f;
		constexpr auto kStaleDamageElapsedMax = std::chrono::seconds(5);
		constexpr auto kProcDispatchCleanupAge = std::chrono::seconds(10);
		constexpr std::size_t kProcDispatchMaxEntries = 512u;

		std::unordered_map<std::uint64_t, ProcDispatchRecord> s_procDispatch;
		std::mutex s_procDispatchMutex;

		// Guards against proc-on-proc chain reactions across deferred SKSE tasks.
		// Set to true while ExecutePostHealthDamageActions runs; any HandleHealthDamage
		// triggered synchronously by CastSpellImmediate on the same thread will see
		// this flag and skip the proc evaluation path.
		thread_local bool g_inProcDispatch = false;

		void ExecutePostHealthDamageActions(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			float a_adjustedDamage,
			float a_originalDamage,
			const std::array<DeferredConversionCast, DamageAdjustmentResult::kMaxConversions>& a_conversions,
			std::size_t a_conversionCount,
			std::chrono::steady_clock::time_point a_now,
			const RE::HitData* a_capturedHitData) noexcept
		{
			if (!a_target) {
				return;
			}

			auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
			if (!bridge) {
				return;
			}

			struct ScopedProcGuard {
				ScopedProcGuard() { g_inProcDispatch = true; }
				~ScopedProcGuard() { g_inProcDispatch = false; }
			} procGuard;

			(void)a_adjustedDamage;

			// Use the pre-captured hitData snapshot from the hook path.
			// GetLastHitData(target) is unreliable for projectile hits because
			// the engine may clear/overwrite lastHitData before this deferred task runs.
			const auto* hitData = a_capturedHitData;

			const auto coc = bridge->EvaluateCastOnCrit(
				a_attacker,
				a_target,
				hitData,
				false);

			bridge->OnHealthDamage(a_target, a_attacker, hitData, a_originalDamage);

			for (std::size_t i = 0; i < a_conversionCount; ++i) {
				const auto& conv = a_conversions[i];
				if (conv.spellFormID != 0u && conv.magnitudeOverride > 0.0f && a_attacker) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(conv.spellFormID);
					if (spell) {
						if (auto* caster = a_attacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
							caster->CastSpellImmediate(
								spell,
								conv.noHitEffectArt,
								a_target,
								conv.effectiveness,
								false,
								conv.magnitudeOverride,
								a_attacker);
						}
					}
				}
			}

			if (coc.spell && a_attacker) {
				bool casted = false;
				if (auto* magicCaster = a_attacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
					magicCaster->CastSpellImmediate(
						coc.spell,
						coc.noHitEffectArt,
						a_target,
						coc.effectiveness,
						false,
						coc.magnitudeOverride,
						a_attacker);
					casted = true;
				}
				if (casted && coc.noHitEffectArt) {
					PlayCastOnCritProcFeedbackSfx(coc.spell);
					PlayCastOnCritProcFeedbackVfxSafe(a_target, coc.spell, a_now);
				}
			}
		}
	}

	bool IsInProcDispatchGuard() noexcept
	{
		return g_inProcDispatch;
	}

	const RE::HitData* ResolveStableHitDataForSpecialActions(
		const RE::HitData* a_hitData,
		const RE::Actor* a_target,
		RE::Actor* a_attacker) noexcept
	{
		if (!a_hitData) {
			return nullptr;
		}

		if (!HitDataUtil::HitDataMatchesActors(a_hitData, a_target, a_attacker)) {
			return nullptr;
		}

		if (!HitDataUtil::HasHitLikeSource(a_hitData, a_attacker)) {
			return nullptr;
		}

		if (a_hitData->attackDataSpell) {
			const auto editorId = SafeCStringView(a_hitData->attackDataSpell->GetFormEditorID());
			if (!editorId.empty() && editorId.starts_with("CAFF_")) {
				return nullptr;
			}
		}

		return a_hitData;
	}

	bool ShouldAllowProcDispatch(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_preHitData,
		float a_damage,
		std::chrono::steady_clock::time_point a_now) noexcept
	{
		constexpr auto kTimeCooldown = std::chrono::milliseconds(250);

		const auto pairKey =
			(static_cast<std::uint64_t>(a_target->GetFormID()) << 32) |
			static_cast<std::uint64_t>(a_attacker ? a_attacker->GetFormID() : 0u);

		const std::scoped_lock lock(s_procDispatchMutex);
		auto& record = s_procDispatch[pairKey];

		const bool hasRecord = record.dispatchTime.time_since_epoch().count() != 0;
		const auto elapsed = hasRecord ? (a_now - record.dispatchTime) : std::chrono::steady_clock::duration::max();

		if (hasRecord && elapsed < kTimeCooldown) {
			return false;
		}

		if (a_preHitData && hasRecord && elapsed < kStaleDamageElapsedMax) {
			const float expectedDealt = std::max(
				0.0f,
				a_preHitData->totalDamage - a_preHitData->resistedPhysicalDamage - a_preHitData->resistedTypedDamage);
			const float absDmg = std::abs(a_damage);
			if (expectedDealt >= kStaleDamageMinExpected && absDmg > 0.0f && absDmg < expectedDealt * kStaleDamageRatioThreshold) {
				SKSE::log::trace("CalamityAffixes: stale damage guard blocked proc (expected={:.1f}, actual={:.1f}).", expectedDealt, absDmg);
				return false;
			}
		}

		record.dispatchTime = a_now;

		if (s_procDispatch.size() > kProcDispatchMaxEntries) {
			for (auto it = s_procDispatch.begin(); it != s_procDispatch.end();) {
				if ((a_now - it->second.dispatchTime) >= kProcDispatchCleanupAge) {
					it = s_procDispatch.erase(it);
				} else {
					++it;
				}
			}
		}

		return true;
	}

	DamageAdjustmentResult AdjustDamageAndEvaluateSpecials(
		CalamityAffixes::EventBridge* a_bridge,
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_preHitData,
		float a_damage) noexcept
	{
		DamageAdjustmentResult result{};
		result.adjustedDamage = std::abs(a_damage);

		const float critMult = a_bridge->GetCritDamageMultiplier(a_attacker, a_preHitData);
		if (std::isfinite(critMult) && critMult > 1.0f) {
			result.adjustedDamage *= critMult;
		}
		if (!std::isfinite(result.adjustedDamage)) {
			result.adjustedDamage = std::abs(a_damage);
		}
		result.originalDamage = result.adjustedDamage;

		const auto conversionResults = a_bridge->EvaluateConversion(
			a_attacker, a_target, a_preHitData, result.adjustedDamage, false);
		const auto mindOverMatter = a_bridge->EvaluateMindOverMatter(
			a_target, a_attacker, a_preHitData, result.adjustedDamage, false);
		(void)mindOverMatter;

		for (std::size_t i = 0; i < conversionResults.count; ++i) {
			const auto& cr = conversionResults.entries[i];
			if (cr.spell && cr.convertedDamage > 0.0f && a_attacker && a_target) {
				auto& dc = result.conversions[result.conversionCount++];
				dc.spellFormID = cr.spell->GetFormID();
				dc.effectiveness = cr.effectiveness;
				dc.noHitEffectArt = cr.noHitEffectArt;
				dc.magnitudeOverride = cr.convertedDamage;
			}
		}

		return result;
	}

	void SchedulePostHealthDamageActions(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const DamageAdjustmentResult& a_adjustment,
		std::chrono::steady_clock::time_point a_now,
		const RE::HitData* a_preHitData) noexcept
	{
		auto capturedHitData = std::make_shared<std::optional<RE::HitData>>();
		if (a_preHitData) {
			capturedHitData->emplace(*a_preHitData);
		}

		if (auto* tasks = SKSE::GetTaskInterface()) {
			const RE::FormID targetFormID = a_target->GetFormID();
			const RE::FormID attackerFormID = a_attacker ? a_attacker->GetFormID() : 0u;
			const auto deferredConversions = a_adjustment.conversions;
			const auto conversionCount = a_adjustment.conversionCount;
			const float adjustedDamage = a_adjustment.adjustedDamage;
			const float originalDamage = a_adjustment.originalDamage;
			tasks->AddTask([targetFormID, attackerFormID, adjustedDamage, originalDamage, deferredConversions, conversionCount, a_now, capturedHitData]() {
				auto* target = RE::TESForm::LookupByID<RE::Actor>(targetFormID);
				auto* attacker = attackerFormID != 0u ?
					                 RE::TESForm::LookupByID<RE::Actor>(attackerFormID) :
					                 nullptr;
				const RE::HitData* hitDataPtr = capturedHitData->has_value() ? &capturedHitData->value() : nullptr;
				ExecutePostHealthDamageActions(
					target,
					attacker,
					adjustedDamage,
					originalDamage,
					deferredConversions,
					conversionCount,
					a_now,
					hitDataPtr);
			});
			return;
		}

		ExecutePostHealthDamageActions(
			a_target,
			a_attacker,
			a_adjustment.adjustedDamage,
			a_adjustment.originalDamage,
			a_adjustment.conversions,
			a_adjustment.conversionCount,
			a_now,
			a_preHitData);
	}

	void ClearDispatchRuntimeState() noexcept
	{
		{
			const std::scoped_lock lock(s_procDispatchMutex);
			s_procDispatch.clear();
		}
		{
			const std::scoped_lock lock(s_nextAllowedByTargetMutex);
			s_nextAllowedByTarget.clear();
		}
	}
}
