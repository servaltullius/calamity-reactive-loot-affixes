#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/TrapCellPolicy.h"

#include <algorithm>
#include <array>
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

		// Mirror the call shape of po3's Papyrus Extender SpawnParticleEffect —
		// the one BSTempEffectParticle caller known to render in shipping mods:
		// NiMatrix3 overload (engine id 29219), flags 7, and a NON-NULL attach
		// node (the reference's 3D root). Overload, flags, model paths (BSA
		// verified), position (player's feet), and thread (task queue) were all
		// eliminated across the 2026-08-10 sessions with spawns accepted but
		// never drawn; the null attach node is the last remaining difference.
		[[nodiscard]] RE::BSTempEffectParticle* SpawnTrapParticle(
			RE::TESObjectCELL* a_cell,
			float a_lifetime,
			const char* a_model,
			const RE::NiPoint3& a_position,
			float a_scale,
			RE::NiAVObject* a_attachNode,
			const RE::NiMatrix3& a_rotation = RE::NiMatrix3{})
		{
			return RE::BSTempEffectParticle::Spawn(
				a_cell,
				a_lifetime,
				a_model,
				a_rotation,
				a_position,
				a_scale,
				kTempEffectParticleFlags,
				a_attachNode);
		}

		// Traps only exist in cells near the player, so the player's world model
		// is a valid anchor for every marker. If the probe shows anchored spawns
		// following the player instead of staying put, revert to positional
		// spawning and keep hunting.
		[[nodiscard]] RE::NiAVObject* ResolvePlayerAnchor3D()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			return player ? player->Get3D(false) : nullptr;
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
				auto* anchor = ResolvePlayerAnchor3D();
				const auto* particle = SpawnTrapParticle(
					a_trap.cell,
					a_cue.durationSeconds,
					model,
					a_trap.position,
					a_cue.scale,
					anchor);
				if (_loot.debugLog) {
					SKSE::log::debug(
						"CalamityAffixes: trap cue spawn (model={}, duration={}, scale={}, anchored={}, spawned={}).",
						model,
						a_cue.durationSeconds,
						a_cue.scale,
						anchor != nullptr,
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
		auto* anchor = ResolvePlayerAnchor3D();
		auto* particle = SpawnTrapParticle(
			a_trap.cell,
			lifetime,
			model,
			a_trap.position,
			scale,
			anchor);
		if (_loot.debugLog) {
			// The marker layer has never been confirmed on screen (every earlier
			// session predated the MODL prefix fix), so log the engine's answer:
			// spawned=false means the temp-effect path rejected this model,
			// spawned=true with nothing visible points at flags/position/NIF.
			SKSE::log::debug(
				"CalamityAffixes: trap marker spawn (model={}, state={}, lifetime={}, scale={}, pos=({:.1f}, {:.1f}, {:.1f}), anchored={}, spawned={}).",
				model,
				a_state == TrapVisualState::kUnarmed ? "unarmed" : "armed",
				lifetime,
				scale,
				a_trap.position.x,
				a_trap.position.y,
				a_trap.position.z,
				anchor != nullptr,
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

			// Variant matrix: 2 BSA-verified vanilla models x {free, anchored},
			// plus one exact replica of po3's call (anchored + the node's world
			// rotation). Whichever column renders names the missing ingredient.
			struct ProbeVariant
			{
				const char* tag;
				const char* model;
				bool anchored;
				bool nodeRotation;
			};
			static constexpr std::array<ProbeVariant, 5> kProbeVariants{{
				{ "beartrap-free", "Traps\\BearTrap\\BearTrap01.nif", false, false },
				{ "beartrap-anchored", "Traps\\BearTrap\\BearTrap01.nif", true, false },
				{ "soultrap-free", "Magic\\SoulTrapTargetPointFX.nif", false, false },
				{ "soultrap-anchored", "Magic\\SoulTrapTargetPointFX.nif", true, false },
				{ "soultrap-po3exact", "Magic\\SoulTrapTargetPointFX.nif", true, true },
			}};

			auto* root = player->Get3D(false);
			const auto base = player->GetPosition();
			float offset = 0.0f;
			for (const auto& variant : kProbeVariants) {
				RE::NiPoint3 position = base;
				position.x += offset;
				offset += 96.0f;
				auto* anchor = variant.anchored ? root : nullptr;
				const RE::NiMatrix3 rotation = (variant.nodeRotation && root) ?
					root->world.rotate : RE::NiMatrix3{};
				const auto* particle = SpawnTrapParticle(
					cell, 10.0f, variant.model, position, 1.5f, anchor, rotation);
				SKSE::log::info(
					"CalamityAffixes: trap marker probe (variant={}, model={}, anchored={}, pos=({:.1f}, {:.1f}, {:.1f}), spawned={}).",
					variant.tag,
					variant.model,
					anchor != nullptr,
					position.x,
					position.y,
					position.z,
					particle != nullptr);
			}
			EmitHudNotification("Calamity: marker probe spawned 5 variants at your feet (10s).");
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
