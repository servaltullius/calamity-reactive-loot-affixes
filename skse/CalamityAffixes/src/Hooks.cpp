#include "CalamityAffixes/Hooks.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/TriggerGuards.h"

namespace CalamityAffixes::Hooks
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

		[[nodiscard]] bool ShouldPlayCastOnCritProcFeedbackVfx(
			RE::Actor* a_target,
			std::chrono::steady_clock::time_point a_now) noexcept
		{
			if (!a_target || a_target->IsDead()) {
				return false;
			}

			static std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> nextAllowedByTarget;
			static std::mutex nextAllowedByTargetMutex;
			constexpr auto kPerTargetIcd = std::chrono::milliseconds(120);
			constexpr std::size_t kMaxEntries = 512u;
			const std::scoped_lock lock(nextAllowedByTargetMutex);

			const auto targetFormID = a_target->GetFormID();
			if (const auto it = nextAllowedByTarget.find(targetFormID); it != nextAllowedByTarget.end() && a_now < it->second) {
				return false;
			}

			nextAllowedByTarget[targetFormID] = a_now + kPerTargetIcd;

			if (nextAllowedByTarget.size() > kMaxEntries) {
				for (auto it = nextAllowedByTarget.begin(); it != nextAllowedByTarget.end();) {
					if (a_now >= it->second) {
						it = nextAllowedByTarget.erase(it);
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

		[[nodiscard]] bool HitDataMatchesCurrentActors(
			const RE::HitData* a_hitData,
			const RE::Actor* a_target,
			const RE::Actor* a_attacker) noexcept
		{
			if (!a_hitData || !a_target) {
				return false;
			}

			const auto hitTarget = a_hitData->target.get().get();
			if (hitTarget && hitTarget != a_target) {
				return false;
			}

			const auto hitAggressor = a_hitData->aggressor.get().get();
			if (a_attacker) {
				if (hitAggressor && hitAggressor != a_attacker) {
					return false;
				}
			} else if (hitAggressor) {
				return false;
			}

			return true;
		}

		[[nodiscard]] bool HasHitLikeSource(const RE::HitData* a_hitData, RE::Actor* a_attacker) noexcept
		{
			if (!a_hitData) {
				return false;
			}

			if (a_hitData->weapon != nullptr || a_hitData->attackDataSpell != nullptr) {
				return true;
			}

			// Bow/crossbow arrows: hitData->weapon may be null — resolve from attacker.
			if (HitDataUtil::ResolveHitWeapon(a_hitData, a_attacker)) {
				return true;
			}

			return a_hitData->flags.any(RE::HitData::Flag::kMeleeAttack) ||
			       a_hitData->flags.any(RE::HitData::Flag::kExplosion);
		}

		[[nodiscard]] const RE::HitData* ResolveStableHitDataForSpecialActions(
			const RE::HitData* a_hitData,
			const RE::Actor* a_target,
			RE::Actor* a_attacker) noexcept
		{
			if (!a_hitData) {
				return nullptr;
			}

			if (!HitDataMatchesCurrentActors(a_hitData, a_target, a_attacker)) {
				return nullptr;
			}

			if (!HasHitLikeSource(a_hitData, a_attacker)) {
				return nullptr;
			}

			// Reject internal proc spell damage to prevent chain-reaction loops
			// (proc spell → damage → CoC/Conversion → more proc spells → ...).
			if (a_hitData->attackDataSpell) {
				const auto editorId = SafeCStringView(a_hitData->attackDataSpell->GetFormEditorID());
				if (!editorId.empty() && editorId.starts_with("CAFF_")) {
					return nullptr;
				}
			}

			return a_hitData;
		}

		struct DeferredConversionCast
		{
			RE::FormID spellFormID{ 0u };
			float effectiveness{ 1.0f };
			bool noHitEffectArt{ false };
			float magnitudeOverride{ 0.0f };
		};

		// Guards against proc-on-proc chain reactions across deferred SKSE tasks.
		// Set to true while ExecutePostHealthDamageActions runs; any HandleHealthDamage
		// triggered synchronously by CastSpellImmediate on the same thread will see
		// this flag and skip the proc evaluation path.
		thread_local bool g_inProcDispatch = false;

		void ExecutePostHealthDamageActions(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			float a_adjustedDamage,
			DeferredConversionCast a_conversion,
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

			// Use the pre-captured hitData snapshot from ThunkImpl.
			// GetLastHitData(target) is unreliable for projectile hits because
			// the engine may clear/overwrite lastHitData before this deferred task runs.
			const auto* hitData = a_capturedHitData;

			// Evaluate CoC while hitData pointer is still valid (before any spell casts).
			const auto coc = bridge->EvaluateCastOnCrit(
				a_attacker,
				a_target,
				hitData,
				false);

			// Route hit procs BEFORE casting Conversion/CoC spells.
			// CastSpellImmediate can trigger HandleHealthDamage synchronously,
			// which overwrites the actor's lastHitData and invalidates our hitData pointer.
			bridge->OnHealthDamage(a_target, a_attacker, hitData, a_adjustedDamage);

			// Cast Conversion spell (hitData pointer may become stale after this).
			if (a_conversion.spellFormID != 0u && a_conversion.magnitudeOverride > 0.0f && a_attacker) {
				auto* conversionSpell = RE::TESForm::LookupByID<RE::SpellItem>(a_conversion.spellFormID);
				if (conversionSpell) {
					if (auto* magicCaster = a_attacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
						magicCaster->CastSpellImmediate(
							conversionSpell,
							a_conversion.noHitEffectArt,
							a_target,
							a_conversion.effectiveness,
							false,
							a_conversion.magnitudeOverride,
							a_attacker);
					}
				}
			}

			// Cast CoC spell from pre-evaluated result.
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

		class ActorHandleHealthDamageHook
		{
		public:
			static void Install()
			{
				if (_installed) {
					return;
				}

				const std::size_t idx = HandleHealthDamageVfuncIndexForRuntime(REL::Module::IsVR());
				_vfuncIndex = idx;

				_hookedActor = false;
				_hookedCharacter = false;
				_hookedPlayerCharacter = false;

				REL::Relocation<std::uintptr_t> actorVtbl{ RE::VTABLE_Actor[0] };
				_hookedActor = TryInstallVfuncHook(actorVtbl, idx, ThunkActor, _originalActor, "Actor");

				REL::Relocation<std::uintptr_t> characterVtbl{ RE::VTABLE_Character[0] };
				_hookedCharacter = TryInstallVfuncHook(characterVtbl, idx, ThunkCharacter, _originalCharacter, "Character");

				bool allowPlayerHook = false;
				if (auto* bridge = CalamityAffixes::EventBridge::GetSingleton(); bridge) {
					allowPlayerHook = bridge->AllowsPlayerHealthDamageHook();
				}

				if (allowPlayerHook) {
					REL::Relocation<std::uintptr_t> playerVtbl{ RE::VTABLE_PlayerCharacter[0] };
					_hookedPlayerCharacter = TryInstallVfuncHook(
						playerVtbl,
						idx,
						ThunkPlayerCharacter,
						_originalPlayerCharacter,
						"PlayerCharacter");
				} else {
					SKSE::log::info(
						"CalamityAffixes: allowPlayerHealthDamageHook=false; skipping PlayerCharacter HandleHealthDamage hook.");
				}

				_installed = _hookedActor || _hookedCharacter || _hookedPlayerCharacter;
				if (!_installed) {
					SKSE::log::warn(
						"CalamityAffixes: HandleHealthDamage hooks were skipped (idx=0x{:X}); using TESHitEvent fallback only.",
						idx);
					return;
				}

				SKSE::log::info(
					"CalamityAffixes: installed HandleHealthDamage hooks (idx=0x{:X}, actor={}, character={}, player={}).",
					idx,
					_hookedActor,
					_hookedCharacter,
					_hookedPlayerCharacter);
			}

			[[nodiscard]] static bool IsHooked(const RE::Actor* a_actor) noexcept
			{
				if (!_installed || !a_actor || _vfuncIndex == 0) {
					return false;
				}

				auto** vtbl = *reinterpret_cast<void***>(const_cast<RE::Actor*>(a_actor));
				if (!vtbl) {
					return false;
				}

				const auto* current = vtbl[_vfuncIndex];
				if (_hookedActor && current == reinterpret_cast<const void*>(ThunkActor)) {
					return true;
				}
				if (_hookedCharacter && current == reinterpret_cast<const void*>(ThunkCharacter)) {
					return true;
				}
				if (_hookedPlayerCharacter && current == reinterpret_cast<const void*>(ThunkPlayerCharacter)) {
					return true;
				}
				return false;
			}

		private:
			using ThunkFn = void(RE::Actor*, RE::Actor*, float);

			[[nodiscard]] static bool IsLikelySkyrimTextAddress(std::uintptr_t a_address) noexcept
			{
				if (a_address == 0u) {
					return false;
				}

				const auto& module = REL::Module::get();
				auto inSegment = [a_address](const REL::Segment& a_segment) {
					const auto segmentBase = a_segment.address();
					const auto segmentSize = a_segment.size();
					if (segmentBase == 0u || segmentSize == 0u) {
						return false;
					}
					const auto segmentEnd = segmentBase + segmentSize;
					return a_address >= segmentBase && a_address < segmentEnd;
				};

				return inSegment(module.segment(REL::Segment::textx)) ||
				       inSegment(module.segment(REL::Segment::textw));
			}

			[[nodiscard]] static bool TryInstallVfuncHook(
				REL::Relocation<std::uintptr_t> a_vtbl,
				std::size_t a_index,
				ThunkFn* a_thunk,
				ThunkFn*& a_original,
				const char* a_label)
			{
				auto* entries = reinterpret_cast<std::uintptr_t*>(a_vtbl.address());
				if (!entries) {
					SKSE::log::warn(
						"CalamityAffixes: {} vtable unavailable; skipping HandleHealthDamage hook.",
						a_label ? a_label : "<unknown>");
					return false;
				}

				const auto current = entries[a_index];
				if (!IsLikelySkyrimTextAddress(current)) {
					SKSE::log::warn(
						"CalamityAffixes: {} HandleHealthDamage slot already patched (addr=0x{:X}); skipping to avoid hook-chain conflicts.",
						a_label ? a_label : "<unknown>",
						current);
					return false;
				}

				a_original = reinterpret_cast<ThunkFn*>(current);
				a_vtbl.write_vfunc(a_index, a_thunk);
				return true;
			}

			struct ScopedFlag
			{
				bool& flag;
				explicit ScopedFlag(bool& a_flag) :
					flag(a_flag)
				{
					flag = true;
				}
				~ScopedFlag()
				{
					flag = false;
				}
			};

			static void CallOriginal(
				ThunkFn* a_original,
				RE::Actor* a_this,
				RE::Actor* a_attacker,
				float a_damage,
				const char* a_hookLabel)
			{
				if (!a_original) {
					SKSE::log::warn(
						"CalamityAffixes: {} original HandleHealthDamage pointer is null; skipping call.",
						a_hookLabel ? a_hookLabel : "<unknown>");
					return;
				}

				a_original(a_this, a_attacker, a_damage);
			}

			static void ThunkImpl(
				ThunkFn* a_original,
				RE::Actor* a_this,
				RE::Actor* a_attacker,
				float a_damage,
				const char* a_hookLabel)
			{
				static thread_local bool inHook = false;
				auto* safeTarget = SanitizeObjectPointer(a_this);
				auto* safeAttacker = SanitizeObjectPointer(a_attacker);

				if (!safeTarget) {
					SKSE::log::warn(
						"CalamityAffixes: {} HandleHealthDamage received invalid target pointer (target=0x{:X}, attacker=0x{:X}); dropping callback.",
						a_hookLabel ? a_hookLabel : "<unknown>",
						reinterpret_cast<std::uintptr_t>(a_this),
						reinterpret_cast<std::uintptr_t>(a_attacker));
					return;
				}
				if (a_attacker && !safeAttacker) {
					SKSE::log::warn(
						"CalamityAffixes: {} HandleHealthDamage sanitized invalid attacker pointer (attacker=0x{:X}); forwarding as null.",
						a_hookLabel ? a_hookLabel : "<unknown>",
						reinterpret_cast<std::uintptr_t>(a_attacker));
				}

				if (inHook || g_inProcDispatch) {
					CallOriginal(a_original, safeTarget, safeAttacker, a_damage, a_hookLabel);
					return;
				}

				ScopedFlag guard(inHook);

				if (!ShouldProcessHealthDamageHookPointers(safeTarget, safeAttacker)) {
					CallOriginal(a_original, safeTarget, safeAttacker, a_damage, a_hookLabel);
					return;
				}

				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge) {
					CallOriginal(a_original, safeTarget, safeAttacker, a_damage, a_hookLabel);
					return;
				}

				const auto context = BuildCombatTriggerContext(safeTarget, safeAttacker);
				const auto now = std::chrono::steady_clock::now();

				if (!ShouldProcessHealthDamageProcPath(
						context.hasTarget,
						context.hasAttacker,
						context.targetIsPlayer,
						context.attackerIsPlayerOwned,
						context.hasPlayerOwner,
						context.hostileEitherDirection,
						bridge->AllowsNonHostilePlayerOwnedOutgoingProcs())) {
					CallOriginal(a_original, safeTarget, safeAttacker, a_damage, a_hookLabel);
					return;
				}

				const auto* preHitData = ResolveStableHitDataForSpecialActions(
					HitDataUtil::GetLastHitData(safeTarget),
					safeTarget,
					safeAttacker);
				float adjustedDamage = a_damage;
				const float critMult = bridge->GetCritDamageMultiplier(
					safeAttacker,
					preHitData);
				if (std::isfinite(critMult) && critMult > 1.0f) {
					adjustedDamage *= critMult;
				}
				if (!std::isfinite(adjustedDamage) || adjustedDamage < 0.0f) {
					adjustedDamage = a_damage;
				}
				const auto conversion = bridge->EvaluateConversion(
					safeAttacker,
					safeTarget,
					preHitData,
					adjustedDamage,
					false);
				const auto mindOverMatter = bridge->EvaluateMindOverMatter(
					safeTarget,
					safeAttacker,
					preHitData,
					adjustedDamage,
					false);
				(void)mindOverMatter;
				DeferredConversionCast deferredConversion{};
				if (conversion.spell && conversion.convertedDamage > 0.0f && safeAttacker && safeTarget) {
					deferredConversion.spellFormID = conversion.spell->GetFormID();
					deferredConversion.effectiveness = conversion.effectiveness;
					deferredConversion.noHitEffectArt = conversion.noHitEffectArt;
					deferredConversion.magnitudeOverride = conversion.convertedDamage;
				}

				// Snapshot the HitData before CallOriginal — for projectile hits, the engine
				// may clear/overwrite lastHitData before the deferred SKSE task runs.
				auto capturedHitData = std::make_shared<std::optional<RE::HitData>>();
				if (preHitData) {
					capturedHitData->emplace(*preHitData);
				}

				CallOriginal(a_original, safeTarget, safeAttacker, adjustedDamage, a_hookLabel);

				if (auto* tasks = SKSE::GetTaskInterface()) {
					const RE::FormID targetFormID = safeTarget->GetFormID();
					const RE::FormID attackerFormID = safeAttacker ? safeAttacker->GetFormID() : 0u;
					tasks->AddTask([targetFormID, attackerFormID, adjustedDamage, deferredConversion, now, capturedHitData]() {
						auto* target = RE::TESForm::LookupByID<RE::Actor>(targetFormID);
						auto* attacker = attackerFormID != 0u ?
							                 RE::TESForm::LookupByID<RE::Actor>(attackerFormID) :
							                 nullptr;
						const RE::HitData* hitDataPtr = capturedHitData->has_value() ? &capturedHitData->value() : nullptr;
						ExecutePostHealthDamageActions(target, attacker, adjustedDamage, deferredConversion, now, hitDataPtr);
					});
				} else {
					ExecutePostHealthDamageActions(safeTarget, safeAttacker, adjustedDamage, deferredConversion, now, preHitData);
				}
			}

			static void ThunkActor(RE::Actor* a_this, RE::Actor* a_attacker, float a_damage)
			{
				ThunkImpl(_originalActor, a_this, a_attacker, a_damage, "Actor");
			}

			static void ThunkCharacter(RE::Actor* a_this, RE::Actor* a_attacker, float a_damage)
			{
				ThunkImpl(_originalCharacter, a_this, a_attacker, a_damage, "Character");
			}

			static void ThunkPlayerCharacter(RE::Actor* a_this, RE::Actor* a_attacker, float a_damage)
			{
				ThunkImpl(_originalPlayerCharacter, a_this, a_attacker, a_damage, "PlayerCharacter");
			}

			static inline ThunkFn* _originalActor{ nullptr };
			static inline ThunkFn* _originalCharacter{ nullptr };
			static inline ThunkFn* _originalPlayerCharacter{ nullptr };
			static inline std::size_t _vfuncIndex{ 0 };
			static inline bool _installed{ false };
			static inline bool _hookedActor{ false };
			static inline bool _hookedCharacter{ false };
			static inline bool _hookedPlayerCharacter{ false };
		};
	}

	void Install()
	{
		ActorHandleHealthDamageHook::Install();
	}

	bool IsHandleHealthDamageHooked(const RE::Actor* a_actor) noexcept
	{
		return ActorHandleHealthDamageHook::IsHooked(a_actor);
	}
}
