#include "CalamityAffixes/Hooks.h"
#include "Hooks.Dispatch.h"

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

				if (inHook || detail::IsInProcDispatchGuard()) {
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

				const auto* rawHitData = HitDataUtil::GetLastHitData(safeTarget);
				const auto* preHitData = detail::ResolveStableHitDataForSpecialActions(
					rawHitData,
					safeTarget,
					safeAttacker);

				if (!detail::ShouldAllowProcDispatch(safeTarget, safeAttacker, preHitData, a_damage, now)) {
					CallOriginal(a_original, safeTarget, safeAttacker, a_damage, a_hookLabel);
					return;
				}

				const auto adj = detail::AdjustDamageAndEvaluateSpecials(
					bridge, safeAttacker, safeTarget, preHitData, a_damage);

				// Restore original sign for the engine.
				const bool damageWasNegative = (a_damage < 0.0f);
				const float finalDamage = damageWasNegative ? -adj.adjustedDamage : adj.adjustedDamage;
				CallOriginal(a_original, safeTarget, safeAttacker, finalDamage, a_hookLabel);

				detail::SchedulePostHealthDamageActions(safeTarget, safeAttacker, adj, now, preHitData);
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

	void ClearRuntimeState() noexcept
	{
		detail::ClearDispatchRuntimeState();
	}
}
