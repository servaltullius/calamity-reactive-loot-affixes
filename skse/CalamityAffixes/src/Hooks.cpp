#include "CalamityAffixes/Hooks.h"

#include <chrono>
#include <cstddef>

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/TriggerGuards.h"

namespace CalamityAffixes::Hooks
{
	namespace
	{
		class ActorHandleHealthDamageHook
		{
		public:
			static void Install()
			{
				const std::size_t idx = REL::Module::IsAE() ? 0x106 : 0x104;
				_vfuncIndex = idx;

				// In practice, most in-world actors are Character / PlayerCharacter, not a bare Actor.
				// Hook the relevant vtables so we reliably observe health damage across actor types.
				REL::Relocation<std::uintptr_t> actorVtbl{ RE::VTABLE_Actor[0] };
				_originalActor = actorVtbl.write_vfunc(idx, ThunkActor);

				REL::Relocation<std::uintptr_t> characterVtbl{ RE::VTABLE_Character[0] };
				_originalCharacter = characterVtbl.write_vfunc(idx, ThunkCharacter);

				REL::Relocation<std::uintptr_t> playerVtbl{ RE::VTABLE_PlayerCharacter[0] };
				_originalPlayerCharacter = playerVtbl.write_vfunc(idx, ThunkPlayerCharacter);

				_installed = true;
				SKSE::log::info("CalamityAffixes: installed HandleHealthDamage vfunc hooks (idx=0x{:X}).", idx);
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
				return current == reinterpret_cast<const void*>(ThunkActor) ||
				       current == reinterpret_cast<const void*>(ThunkCharacter) ||
				       current == reinterpret_cast<const void*>(ThunkPlayerCharacter);
			}

		private:
			using ThunkFn = void(RE::Actor*, RE::Actor*, float);

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

					const auto context = BuildCombatTriggerContext(a_this, a_attacker);
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					const auto now = std::chrono::steady_clock::now();

					const bool allowNeutralOutgoing =
						ShouldResolveNonHostileOutgoingFirstHitAllowance(
							context.hasPlayerOwner,
							context.targetIsPlayer,
							bridge->AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
						bridge->ResolveNonHostileOutgoingFirstHitAllowance(context.playerOwner, a_this, context.hostileEitherDirection, now);

					if (!ShouldProcessHealthDamageProcPath(
							context.hasTarget,
							context.hasAttacker,
							context.targetIsPlayer,
							context.attackerIsPlayerOwned,
							context.hasPlayerOwner,
							context.hostileEitherDirection,
							allowNeutralOutgoing)) {
					a_original(a_this, a_attacker, a_damage);
					return;
				}

					const auto* hitData = HitDataUtil::GetLastHitData(a_this);
					float adjustedDamage = a_damage;
					const float critMult = bridge->GetCritDamageMultiplier(
						a_attacker, hitData);
					if (critMult > 1.0f) {
						adjustedDamage *= critMult;
					}
					const auto conversion = bridge->EvaluateConversion(
						a_attacker,
						a_this,
						hitData,
						adjustedDamage);

				a_original(a_this, a_attacker, adjustedDamage);

				if (conversion.spell && conversion.convertedDamage > 0.0f && a_attacker && a_this) {
					if (auto* magicCaster = a_attacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
						magicCaster->CastSpellImmediate(
							conversion.spell,
							conversion.noHitEffectArt,
							a_this,
							conversion.effectiveness,
							false,
							conversion.convertedDamage,
							a_attacker);
					}
				}

					const auto coc = bridge->EvaluateCastOnCrit(
						a_attacker,
						a_this,
						hitData);
				if (coc.spell && a_attacker && a_this) {
					if (auto* magicCaster = a_attacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
						magicCaster->CastSpellImmediate(
							coc.spell,
							coc.noHitEffectArt,
							a_this,
							coc.effectiveness,
							false,
							coc.magnitudeOverride,
							a_attacker);
					}
				}

					bridge->OnHealthDamage(a_this, a_attacker, hitData, a_damage);
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
