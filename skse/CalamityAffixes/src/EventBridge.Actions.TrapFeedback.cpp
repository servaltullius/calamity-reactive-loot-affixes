#include "CalamityAffixes/EventBridge.h"

#include <algorithm>

namespace CalamityAffixes
{
	void EventBridge::PlayTrapFeedbackCue(
		const TrapInstance& a_trap,
		const TrapFeedbackCue& a_cue) const noexcept
	{
		if (a_cue.art && a_cue.durationSeconds > 0.0f && a_trap.cell) {
			const auto* model = a_cue.art->GetModel();
			if (model && *model) {
				RE::BSTempEffectParticle::Spawn(
					a_trap.cell,
					a_cue.durationSeconds,
					model,
					RE::NiPoint3{},
					a_trap.position,
					a_cue.scale,
					0u,
					nullptr);
			}
		}
		if (a_cue.sound) {
			PlaySpatialSound(a_cue.sound, a_trap.position);
		}
	}

	void EventBridge::StopTrapMarker(TrapInstance& a_trap) const noexcept
	{
		if (a_trap.markerEffect) {
			a_trap.markerEffect->lifetime = std::min(a_trap.markerEffect->lifetime, a_trap.markerEffect->age);
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
		if (!a_trap.feedback.configured || !a_trap.feedback.markerArt || !a_trap.cell) {
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
		if (auto* particle = RE::BSTempEffectParticle::Spawn(
				a_trap.cell,
				lifetime,
				model,
				RE::NiPoint3{},
				a_trap.position,
				scale,
				0u,
				nullptr)) {
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
