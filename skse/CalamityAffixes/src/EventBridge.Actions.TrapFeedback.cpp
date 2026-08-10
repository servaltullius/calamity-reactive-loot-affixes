#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/TrapCellPolicy.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

namespace CalamityAffixes
{
	namespace
	{
		// BSTempEffectParticle flag semantics are undocumented, but the engine's
		// own impact-effect callers and shipping SKSE mods pass 7 — and with 0
		// every spawn in the 2026-08-10 session returned non-null while nothing
		// ever rendered on screen. Match the known-working value.
		constexpr std::uint32_t kTempEffectParticleFlags = 7u;

		// Mirror the exact call shape of callers that demonstrably render:
		// the NiMatrix3 (identity) overload — engine id 29219 — not the euler
		// NiPoint3 variant (29218), which still produced accepted-but-invisible
		// spawns with flags=7 in the 2026-08-10 follow-up session.
		[[nodiscard]] RE::BSTempEffectParticle* SpawnTrapParticle(
			RE::TESObjectCELL* a_cell,
			float a_lifetime,
			const char* a_model,
			const RE::NiPoint3& a_position,
			float a_scale)
		{
			return RE::BSTempEffectParticle::Spawn(
				a_cell,
				a_lifetime,
				a_model,
				RE::NiMatrix3{},
				a_position,
				a_scale,
				kTempEffectParticleFlags,
				nullptr);
		}
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
				const auto* particle = SpawnTrapParticle(
					a_trap.cell,
					a_cue.durationSeconds,
					model,
					a_trap.position,
					a_cue.scale);
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
		auto* particle = SpawnTrapParticle(
			a_trap.cell,
			lifetime,
			model,
			a_trap.position,
			scale);
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

	void EventBridge::SpawnTrapMarkerProbe()
	{
		// Debug-only render probe: spawns the three marker model families at the
		// player's feet with a fat scale and lifetime, removing every gameplay
		// variable (target position, arm timing, TTL) from the "does this path
		// draw at all?" question. Look down; results also land in the log.
		if (!(_loot.debugHudNotifications || _loot.debugLog)) {
			return;
		}
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}
		// Every rendering Spawn in this codebase runs on the main thread via the
		// task queue (trap ticks are marshalled in TrapSystem.cpp); the probe
		// must not introduce a new thread context as an extra variable.
		tasks->AddTask([this]() {
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* cell = player ? player->GetParentCell() : nullptr;
			if (!player || !cell) {
				EmitHudNotification("Calamity: probe needs a loaded player cell.");
				return;
			}

			static constexpr std::array<std::pair<const char*, float>, 3> kProbeModels{{
				{ "Traps\\BearTrap\\BearTrap01.nif", 0.0f },
				{ "Magic\\SoulTrapTargetPointFX.nif", 96.0f },
				{ "CalamityAffixes\\VFX\\RuneTrapMarker_Calamity.nif", 192.0f },
			}};

			const auto base = player->GetPosition();
			for (const auto& [model, offset] : kProbeModels) {
				RE::NiPoint3 position = base;
				position.x += offset;
				const auto* particle = SpawnTrapParticle(cell, 10.0f, model, position, 1.5f);
				SKSE::log::info(
					"CalamityAffixes: trap marker probe (model={}, pos=({:.1f}, {:.1f}, {:.1f}), spawned={}).",
					model,
					position.x,
					position.y,
					position.z,
					particle != nullptr);
			}
			EmitHudNotification("Calamity: marker probe spawned at your feet (10s).");
		});
	}

	void EventBridge::ClearTrapRuntimeState() noexcept
	{
		for (auto& trap : _trapState.activeTraps) {
			StopTrapMarker(trap);
		}
		_trapState.Reset();
	}
}
