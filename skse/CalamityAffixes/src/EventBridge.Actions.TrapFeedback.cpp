#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/TrapCellPolicy.h"

#include <algorithm>
#include <cstdint>

namespace CalamityAffixes
{
	namespace
	{
		// BSTempEffectParticle flag semantics are undocumented, but the engine's
		// own impact-effect callers and shipping SKSE mods pass 7 — and with 0
		// every spawn in the 2026-08-10 session returned non-null while nothing
		// ever rendered on screen. Match the known-working value.
		constexpr std::uint32_t kTempEffectParticleFlags = 7u;
	}

	void EventBridge::PlayTrapFeedbackCue(
		const TrapInstance& a_trap,
		const TrapFeedbackCue& a_cue) const noexcept
	{
		const bool cellUsable = detail::IsTrapCellUsable(
			a_trap.cell != nullptr,
			a_trap.cell && a_trap.cell->IsAttached());
		if (a_cue.art && a_cue.durationSeconds > 0.0f && cellUsable) {
			const auto* model = a_cue.art->GetModel();
			if (model && *model) {
				const auto* particle = RE::BSTempEffectParticle::Spawn(
					a_trap.cell,
					a_cue.durationSeconds,
					model,
					RE::NiPoint3{},
					a_trap.position,
					a_cue.scale,
					kTempEffectParticleFlags,
					nullptr);
				if (_loot.debugLog) {
					SKSE::log::debug(
						"CalamityAffixes: trap cue spawn (model={}, duration={}, scale={}, spawned={}).",
						model,
						a_cue.durationSeconds,
						a_cue.scale,
						particle != nullptr);
				}
			}
		} else if (a_cue.art && _loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: trap cue spawn skipped (duration={}, cellUsable={}).",
				a_cue.durationSeconds,
				cellUsable);
		}
		if (a_cue.sound) {
			PlaySpatialSound(a_cue.sound, a_trap.position);
		}
	}

	void EventBridge::StopTrapMarker(TrapInstance& a_trap) const noexcept
	{
		if (a_trap.markerEffect) {
			const bool cellUsable = detail::IsTrapCellUsable(
				a_trap.cell != nullptr,
				a_trap.cell && a_trap.cell->IsAttached());
			if (cellUsable) {
				a_trap.markerEffect->lifetime = std::min(a_trap.markerEffect->lifetime, a_trap.markerEffect->age);
			}
			a_trap.markerEffect.reset();
		}
		a_trap.visualState = TrapVisualState::kNone;
	}

	void EventBridge::StartTrapMarker(
		TrapInstance& a_trap,
		TrapVisualState a_state,
		std::chrono::steady_clock::time_point a_now) const noexcept
	{
		StopTrapMarker(a_trap);
		const bool cellUsable = detail::IsTrapCellUsable(
			a_trap.cell != nullptr,
			a_trap.cell && a_trap.cell->IsAttached());
		if (!a_trap.feedback.configured || !a_trap.feedback.markerArt || !cellUsable) {
			return;
		}

		const auto* model = a_trap.feedback.markerArt->GetModel();
		if (!model || !*model) {
			return;
		}
		const auto endAt = a_state == TrapVisualState::kUnarmed ? a_trap.armedAt : a_trap.expiresAt;
		const float lifetime = std::max(
			0.05f,
			std::chrono::duration_cast<std::chrono::duration<float>>(endAt - a_now).count());
		const float scale = a_state == TrapVisualState::kUnarmed ?
			a_trap.feedback.unarmedScale : a_trap.feedback.armedScale;
		auto* particle = RE::BSTempEffectParticle::Spawn(
			a_trap.cell,
			lifetime,
			model,
			RE::NiPoint3{},
			a_trap.position,
			scale,
			kTempEffectParticleFlags,
			nullptr);
		if (_loot.debugLog) {
			// The marker layer has never been confirmed on screen (every earlier
			// session predated the MODL prefix fix), so log the engine's answer:
			// spawned=false means the temp-effect path rejected this model,
			// spawned=true with nothing visible points at flags/position/NIF.
			SKSE::log::debug(
				"CalamityAffixes: trap marker spawn (model={}, state={}, lifetime={}, scale={}, pos=({:.1f}, {:.1f}, {:.1f}), spawned={}).",
				model,
				a_state == TrapVisualState::kUnarmed ? "unarmed" : "armed",
				lifetime,
				scale,
				a_trap.position.x,
				a_trap.position.y,
				a_trap.position.z,
				particle != nullptr);
		}
		if (particle) {
			a_trap.markerEffect = RE::NiPointer<RE::BSTempEffectParticle>{ particle };
			a_trap.visualState = a_state;
		}
	}

	void EventBridge::RemoveTrapAt(std::size_t a_index, TrapRemovalReason a_reason) noexcept
	{
		auto& activeTraps = _trapState.activeTraps;
		if (a_index >= activeTraps.size()) {
			return;
		}

		auto& trap = activeTraps[a_index];
		StopTrapMarker(trap);
		if (a_reason == TrapRemovalReason::kExpired) {
			PlayTrapFeedbackCue(trap, trap.feedback.expired);
		}
		activeTraps.erase(activeTraps.begin() + static_cast<std::ptrdiff_t>(a_index));
		if (_trapState.tickCursor > a_index) {
			_trapState.tickCursor -= 1u;
		}
		if (_trapState.tickCursor >= activeTraps.size()) {
			_trapState.tickCursor = 0u;
		}
		_trapState.hasActiveTraps.store(!activeTraps.empty(), std::memory_order_relaxed);
	}

	void EventBridge::ClearTrapRuntimeState() noexcept
	{
		for (auto& trap : _trapState.activeTraps) {
			StopTrapMarker(trap);
		}
		_trapState.Reset();
	}
}
