#include "CalamityAffixes/Hooks.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <mutex>
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

		class ActorHandleHealthDamageHook
		{
		public:
			static void Install()
			{
				if (_installed) {
					return;
				}

				const std::size_t idx = REL::Module::IsAE() ? 0x106 : 0x104;
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
				REL::Relocation<ThunkFn>& a_original,
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

				a_original = a_vtbl.write_vfunc(a_index, a_thunk);
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

				static void ThunkImpl(
					const REL::Relocation<ThunkFn>& a_original,
					RE::Actor* a_this,
					RE::Actor* a_attacker,
					float a_damage)
			{
				static thread_local bool inHook = false;
				if (inHook) {
					a_original(a_this, a_attacker, a_damage);
					return;
				}

				ScopedFlag guard(inHook);

				if (!ShouldProcessHealthDamageHookPointers(a_this, a_attacker)) {
					a_original(a_this, a_attacker, a_damage);
					return;
				}

				auto* safeTarget = SanitizeObjectPointer(a_this);
				auto* safeAttacker = SanitizeObjectPointer(a_attacker);
				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge) {
					a_original(a_this, a_attacker, a_damage);
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
						false)) {
					a_original(a_this, a_attacker, a_damage);
					return;
				}

				const auto* hitData = HitDataUtil::GetLastHitData(safeTarget);
				float adjustedDamage = a_damage;
				const float critMult = bridge->GetCritDamageMultiplier(
					safeAttacker, hitData);
				if (critMult > 1.0f) {
					adjustedDamage *= critMult;
				}
				const auto conversion = bridge->EvaluateConversion(
					safeAttacker,
					safeTarget,
					hitData,
					adjustedDamage);
				const auto mindOverMatter = bridge->EvaluateMindOverMatter(
					safeTarget,
					safeAttacker,
					hitData,
					adjustedDamage);
				(void)mindOverMatter;

				a_original(a_this, a_attacker, adjustedDamage);

				if (conversion.spell && conversion.convertedDamage > 0.0f && safeAttacker && safeTarget) {
					if (auto* magicCaster = safeAttacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
						magicCaster->CastSpellImmediate(
							conversion.spell,
							conversion.noHitEffectArt,
							safeTarget,
							conversion.effectiveness,
							false,
							conversion.convertedDamage,
							safeAttacker);
					}
				}

					const auto coc = bridge->EvaluateCastOnCrit(
						safeAttacker,
						safeTarget,
						hitData);
					if (coc.spell && safeAttacker && safeTarget) {
						bool casted = false;
						if (auto* magicCaster = safeAttacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
							magicCaster->CastSpellImmediate(
								coc.spell,
								coc.noHitEffectArt,
								safeTarget,
								coc.effectiveness,
								false,
								coc.magnitudeOverride,
								safeAttacker);
							casted = true;
						}
						if (casted && coc.noHitEffectArt) {
							PlayCastOnCritProcFeedbackSfx(coc.spell);
							PlayCastOnCritProcFeedbackVfxSafe(safeTarget, coc.spell, now);
						}
					}

					bridge->OnHealthDamage(safeTarget, safeAttacker, hitData, adjustedDamage);
				}

			static void ThunkActor(RE::Actor* a_this, RE::Actor* a_attacker, float a_damage)
			{
				ThunkImpl(_originalActor, a_this, a_attacker, a_damage);
			}

			static void ThunkCharacter(RE::Actor* a_this, RE::Actor* a_attacker, float a_damage)
			{
				ThunkImpl(_originalCharacter, a_this, a_attacker, a_damage);
			}

			static void ThunkPlayerCharacter(RE::Actor* a_this, RE::Actor* a_attacker, float a_damage)
			{
				ThunkImpl(_originalPlayerCharacter, a_this, a_attacker, a_damage);
			}

			static inline REL::Relocation<ThunkFn> _originalActor;
			static inline REL::Relocation<ThunkFn> _originalCharacter;
			static inline REL::Relocation<ThunkFn> _originalPlayerCharacter;
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
