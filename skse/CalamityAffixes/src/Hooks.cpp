#include "CalamityAffixes/Hooks.h"

#include <chrono>
#include <cstddef>

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
		[[nodiscard]] RE::TESEffectShader* GetEnchantmentHitShader(RE::ActorValue a_resistValue) noexcept
		{
			switch (a_resistValue) {
			case RE::ActorValue::kResistFire: {
				static RE::TESEffectShader* shader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x0001B212);
				return shader;
			}
			case RE::ActorValue::kResistFrost: {
				static RE::TESEffectShader* shader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x000435A3);
				return shader;
			}
			case RE::ActorValue::kResistShock: {
				static RE::TESEffectShader* shader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x00057C67);
				return shader;
			}
			default:
				return nullptr;
			}
		}

		[[nodiscard]] RE::TESEffectShader* ResolveCastOnCritProcFeedbackShader(const RE::SpellItem* a_spell) noexcept
		{
			if (!a_spell) {
				return nullptr;
			}

			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				const auto resist = effect->baseEffect->data.resistVariable;
				if (auto* shader = GetEnchantmentHitShader(resist)) {
					return shader;
				}
			}

			return nullptr;
		}

		void PlayCastOnCritProcFeedbackVfx(RE::Actor* a_target, const RE::SpellItem* a_spell) noexcept
		{
			if (!a_target || a_target->IsDead()) {
				return;
			}

			auto* shader = ResolveCastOnCritProcFeedbackShader(a_spell);
			if (!shader) {
				return;
			}

			a_target->InstantiateHitShader(shader, 0.18f, nullptr, false, false, nullptr, false);
		}

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

				const bool allowNeutralOutgoing =
					ShouldResolveNonHostileOutgoingFirstHitAllowance(
						context.hasPlayerOwner,
						context.targetIsPlayer,
						bridge->AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
					bridge->ResolveNonHostileOutgoingFirstHitAllowance(context.playerOwner, safeTarget, context.hostileEitherDirection, now);

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
							PlayCastOnCritProcFeedbackVfx(safeTarget, coc.spell);
						}
					}

					bridge->OnHealthDamage(safeTarget, safeAttacker, hitData, a_damage);
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
