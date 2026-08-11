#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/TrapCellPolicy.h"
#include "CalamityAffixes/TrapMarkerAnimationPolicy.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string_view>

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

	void EventBridge::QueueTrapMarkerAnimation(
		TrapInstance& a_trap,
		TrapMarkerAnimationPhase a_phase,
		std::chrono::steady_clock::time_point a_now) const noexcept
	{
		if (a_phase == TrapMarkerAnimationPhase::kNone ||
			!a_trap.feedback.worldMarkerAnimation || !a_trap.markerReference) {
			if (a_phase == TrapMarkerAnimationPhase::kRearm) {
				a_trap.markerRearmAnimationResolved = true;
			}
			return;
		}
		if (a_trap.markerAnimationPhase != TrapMarkerAnimationPhase::kNone) {
			// Preserve phase order. A late Trigger01 retry must settle before
			// Reset01 can be queued, and an initial StartOpen retry cannot be
			// overwritten by an early trigger scan.
			return;
		}

		a_trap.markerAnimationPhase = a_phase;
		a_trap.markerAnimationAttempts = 0u;
		a_trap.markerAnimationNextAttemptAt = a_now;
		if (a_phase == TrapMarkerAnimationPhase::kRearm) {
			a_trap.markerRearmAnimationResolved = false;
		}
	}

	void EventBridge::ProcessTrapMarkerAnimation(
		TrapInstance& a_trap,
		std::chrono::steady_clock::time_point a_now) const noexcept
	{
		const auto phase = a_trap.markerAnimationPhase;
		if (phase == TrapMarkerAnimationPhase::kNone ||
			(a_trap.markerAnimationNextAttemptAt.time_since_epoch().count() != 0 &&
				a_now < a_trap.markerAnimationNextAttemptAt)) {
			return;
		}
		std::string_view phaseName{ "none" };
		switch (phase) {
		case TrapMarkerAnimationPhase::kInitial:
			phaseName = "initial";
			break;
		case TrapMarkerAnimationPhase::kTrigger:
			phaseName = "trigger";
			break;
		case TrapMarkerAnimationPhase::kRearm:
			phaseName = "rearm";
			break;
		case TrapMarkerAnimationPhase::kNone:
		default:
			break;
		}

		const auto clearPending = [&]() noexcept {
			a_trap.markerAnimationPhase = TrapMarkerAnimationPhase::kNone;
			a_trap.markerAnimationAttempts = 0u;
			a_trap.markerAnimationNextAttemptAt = {};
		};
		const auto* animation = a_trap.feedback.worldMarkerAnimation ?
			std::addressof(*a_trap.feedback.worldMarkerAnimation) : nullptr;
		const std::string* event = nullptr;
		if (animation) {
			switch (phase) {
			case TrapMarkerAnimationPhase::kInitial:
				event = std::addressof(animation->initialEvent);
				break;
			case TrapMarkerAnimationPhase::kTrigger:
				event = std::addressof(animation->triggerEvent);
				break;
			case TrapMarkerAnimationPhase::kRearm:
				event = std::addressof(animation->rearmEvent);
				break;
			case TrapMarkerAnimationPhase::kNone:
			default:
				break;
			}
		}
		if (!event || event->empty()) {
			if (phase == TrapMarkerAnimationPhase::kRearm) {
				a_trap.markerRearmAnimationResolved = true;
			}
			clearPending();
			return;
		}

		auto marker = a_trap.markerReferenceOwner;
		if (!marker && a_trap.markerReference) {
			marker = a_trap.markerReference.get();
		}
		auto* markerCell = marker ? marker->GetParentCell() : nullptr;
		const bool cellAttached = markerCell && markerCell->IsAttached();
		const bool referenceUsable = marker &&
			detail::ShouldReusePlacedTrapMarker(
				true,
				marker->IsDisabled(),
				marker->IsMarkedForDeletion());
		const bool referenceRetryable = referenceUsable && (!markerCell || cellAttached);
		auto* marker3D = referenceRetryable ? marker->Get3D(false) : nullptr;
		const bool has3D = marker3D != nullptr;
		RE::BSTSmartPointer<RE::BSAnimationGraphManager> graphManager{};
		const bool hasGraphManager = has3D && marker->GetAnimationGraphManager(graphManager) && graphManager;
		const bool hasLoadedGraph = hasGraphManager &&
			(!graphManager->graphs.empty() || !graphManager->subManagers.empty());
		const bool graphReady = detail::IsTrapMarkerAnimationGraphReady(
			has3D,
			hasGraphManager,
			hasLoadedGraph);
		const bool accepted = graphReady && marker->NotifyAnimationGraph(RE::BSFixedString{ event->c_str() });
		const auto attempt = static_cast<std::uint8_t>(a_trap.markerAnimationAttempts + 1u);
		const bool retryScheduled = detail::ShouldRetryTrapMarkerAnimation(
			referenceRetryable,
			accepted,
			attempt);

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: trap world marker animation (phase={}, event={}, ref=0x{:X}, cellAttached={}, has3D={}, graphReady={}, accepted={}, attempt={} / {}, retryScheduled={}).",
				phaseName,
				*event,
				marker ? marker->GetFormID() : 0u,
				cellAttached,
				has3D,
				graphReady,
				accepted,
				attempt,
				detail::kMaxTrapMarkerAnimationAttempts,
				retryScheduled);
		}

		if (accepted) {
			if (phase == TrapMarkerAnimationPhase::kInitial) {
				// Keep StartOpen visible for the configured gate before the first
				// gameplay trigger can deliver Trigger01.
				a_trap.armedAt = detail::ExtendTrapArmedAtForAcceptedOpenAnimation(
					a_trap.armedAt,
					a_now,
					std::chrono::milliseconds(animation->openGateMilliseconds));
			} else if (phase == TrapMarkerAnimationPhase::kRearm) {
				a_trap.markerRearmAnimationResolved = true;
				const auto openGate = std::chrono::milliseconds(animation->openGateMilliseconds);
				a_trap.armedAt = detail::ExtendTrapArmedAtForAcceptedOpenAnimation(
					a_trap.armedAt,
					a_now,
					openGate);
			}
			clearPending();
			return;
		}
		if (retryScheduled) {
			a_trap.markerAnimationAttempts = attempt;
			a_trap.markerAnimationNextAttemptAt = a_now + detail::kTrapMarkerAnimationRetryDelay;
			return;
		}

		if (phase == TrapMarkerAnimationPhase::kRearm) {
			// Animation is presentation only. Never leave the gameplay trap stuck
			// disarmed if its visual graph cannot accept Reset01.
			a_trap.markerRearmAnimationResolved = true;
		}
		SKSE::log::warn(
			"CalamityAffixes: trap world marker animation abandoned (phase={}, event={}, ref=0x{:X}, attempt={}, reason={}); gameplay continues fail-open.",
			phaseName,
			*event,
			marker ? marker->GetFormID() : 0u,
			attempt,
			referenceRetryable ? "attempt-cap" : "reference-unusable");
		clearPending();
	}

	void EventBridge::ProcessPendingTrapMarkerCleanup() noexcept
	{
		for (auto& pendingHandle : _trapState.pendingMarkerCleanup) {
			if (!pendingHandle) {
				continue;
			}
			const auto nativeHandle = pendingHandle.native_handle();
			auto marker = pendingHandle.get();
			if (!marker) {
				continue;
			}

			auto* markerCell = marker->GetParentCell();
			const bool cellAttached = markerCell && markerCell->IsAttached();
			bool safetyIssued = false;
			// Apply interaction/physics safety while 3D is valid. A detached cell
			// permits only the non-3D SetDelete retirement path.
			if (!markerCell || cellAttached) {
				marker->SetActivationBlocked(true);
				marker->SetCollision(false);
				safetyIssued = true;
			}
			const auto cleanupPolicy = detail::ResolvePlacedTrapMarkerCleanupPolicy(
				true,
				markerCell != nullptr,
				cellAttached);
			bool disableIssued = false;
			if (cleanupPolicy.disable) {
				marker->Disable();
				disableIssued = true;
			}
			marker->SetDelete(true);
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: trap world marker deferred cleanup resolved (reason=deferred-resolved, handle=0x{:X}, resolved=true, ref=0x{:X}, cellAttached={}, activationBlockIssued={}, collisionDisableIssued={}, disableIssued={}, deleteIssued=true).",
					nativeHandle,
					marker->GetFormID(),
					cellAttached,
					safetyIssued,
					safetyIssued,
					disableIssued);
			}
			pendingHandle.reset();
		}
		_trapState.RefreshRuntimeWorkFlag();
	}

	void EventBridge::StopTrapMarker(TrapInstance& a_trap, std::string_view a_reason) noexcept
	{
		a_trap.markerAnimationPhase = TrapMarkerAnimationPhase::kNone;
		a_trap.markerAnimationAttempts = 0u;
		a_trap.markerAnimationNextAttemptAt = {};
		a_trap.markerRearmAnimationResolved = false;
		if (a_trap.markerEffect) {
			const bool cellUsable = detail::IsTrapCellUsable(
				a_trap.cell != nullptr,
				a_trap.cell && a_trap.cell->IsAttached());
			if (cellUsable) {
				a_trap.markerEffect->lifetime = std::min(a_trap.markerEffect->lifetime, a_trap.markerEffect->age);
			}
			a_trap.markerEffect.reset();
		}

		if (a_trap.markerReference || a_trap.markerReferenceOwner) {
			const auto nativeHandle = a_trap.markerReference ? a_trap.markerReference.native_handle() : 0u;
			auto marker = a_trap.markerReferenceOwner;
			if (!marker && a_trap.markerReference) {
				marker = a_trap.markerReference.get();
			}
			const bool resolved = marker != nullptr;
			const bool strongOwnerRetained = a_trap.markerReferenceOwner != nullptr;
			const RE::FormID referenceFormID = marker ? marker->GetFormID() : 0u;
			bool cellAttached = false;
			bool disableIssued = false;
			bool deleteIssued = false;
			RE::TESObjectCELL* markerCell = nullptr;
			if (marker) {
				markerCell = marker->GetParentCell();
				cellAttached = markerCell && markerCell->IsAttached();
			}
			const auto cleanupPolicy = detail::ResolvePlacedTrapMarkerCleanupPolicy(
				resolved,
				markerCell != nullptr,
				cellAttached);
			if (marker) {
				// Disable touches the loaded 3D, so avoid it after a cell has detached.
				// SetDelete is still required to retire the generated reference and keep
				// it out of future saves if the handle remains resolvable.
				if (cleanupPolicy.disable) {
					marker->Disable();
					disableIssued = true;
				}
				if (cleanupPolicy.markForDeletion) {
					marker->SetDelete(true);
					deleteIssued = true;
				}
			}
			const bool deferredQueued = detail::ShouldDeferUnresolvedPlacedTrapMarker(
				static_cast<bool>(a_trap.markerReference),
				resolved) &&
				_trapState.QueuePendingMarkerCleanup(a_trap.markerReference);
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: trap world marker cleanup (reason={}, handle=0x{:X}, resolved={}, strongOwnerRetained={}, ref=0x{:X}, cellAttached={}, disableIssued={}, deleteIssued={}, deferredQueued={}).",
					a_reason,
					nativeHandle,
					resolved,
					strongOwnerRetained,
					referenceFormID,
					cellAttached,
					disableIssued,
					deleteIssued,
					deferredQueued);
			}
			if (a_trap.markerReference && !resolved && !deferredQueued) {
				SKSE::log::critical(
					"CalamityAffixes: unresolved trap world marker could not enter the fixed cleanup queue (reason={}, handle=0x{:X}); retaining it on the trap fail-closed.",
					a_reason,
					nativeHandle);
				a_trap.visualState = TrapVisualState::kNone;
				return;
			}
			a_trap.markerReferenceOwner.reset();
			a_trap.markerReference.reset();
		}
		a_trap.visualState = TrapVisualState::kNone;
	}

	void EventBridge::StartTrapMarker(
		TrapInstance& a_trap,
		TrapVisualState a_state,
		std::chrono::steady_clock::time_point a_now) noexcept
	{
		const bool cellUsable = detail::IsTrapCellUsable(
			a_trap.cell != nullptr,
			a_trap.cell && a_trap.cell->IsAttached());
		if (!a_trap.feedback.configured ||
			(!a_trap.feedback.markerWorldObject && !a_trap.feedback.markerArt) ||
			!cellUsable) {
			return;
		}

		if (a_trap.feedback.markerWorldObject) {
			ProcessPendingTrapMarkerCleanup();
			// A placed world marker is phase-independent. Arming/rearming only
			// updates logical state; recreating the REFR would churn handles and
			// unnecessarily enlarge the save-change surface.
			if (a_trap.markerReference) {
				auto existing = a_trap.markerReferenceOwner;
				if (!existing) {
					existing = a_trap.markerReference.get();
					if (existing) {
						a_trap.markerReferenceOwner = existing;
						existing->SetActivationBlocked(true);
						existing->SetCollision(false);
					}
				}
				if (detail::ShouldReusePlacedTrapMarker(
						existing != nullptr,
						existing && existing->IsDisabled(),
						existing && existing->IsMarkedForDeletion())) {
					a_trap.visualState = a_state;
					if (_loot.debugLog) {
						SKSE::log::debug(
							"CalamityAffixes: trap world marker reused for state transition (ref=0x{:X}, state={}).",
							existing->GetFormID(),
							a_state == TrapVisualState::kUnarmed ? "unarmed" : "armed");
					}
					return;
				}
				StopTrapMarker(a_trap, "stale-world-reference");
			} else if (a_trap.markerEffect) {
				StopTrapMarker(a_trap, "switch-to-world-reference");
			}

			const auto placedCount = static_cast<std::size_t>(std::count_if(
				_trapState.activeTraps.begin(),
				_trapState.activeTraps.end(),
				[](const TrapInstance& a_activeTrap) { return static_cast<bool>(a_activeTrap.markerReference); }));
			const auto pendingCleanupCount = _trapState.PendingMarkerCleanupCount();
			if (!detail::CanSpawnPlacedTrapMarker(placedCount, pendingCleanupCount)) {
				a_trap.visualState = a_state;
				if (_loot.debugLog) {
					SKSE::log::warn(
						"CalamityAffixes: trap world marker spawn skipped (reason=placed-ref-cap, cap={}, active={}, pendingCleanup={}).",
						detail::kMaxPlacedTrapMarkers,
						placedCount,
						pendingCleanupCount);
				}
				return;
			}

			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			auto* worldspace = a_trap.cell->IsExteriorCell() ? a_trap.cell->GetRuntimeData().worldSpace : nullptr;
			RE::ObjectRefHandle markerHandle{};
			if (dataHandler) {
				markerHandle = dataHandler->CreateReferenceAtLocation(
					a_trap.feedback.markerWorldObject,
					a_trap.position,
					RE::NiPoint3{},
					a_trap.cell,
					worldspace,
					nullptr,
					nullptr,
					RE::ObjectRefHandle{},
					false,
					true);
			}
			auto marker = markerHandle.get();
			const bool spawned = marker != nullptr;
			const bool handleAllocated = static_cast<bool>(markerHandle);
			bool deferredCleanupQueued = false;
			if (marker) {
				a_trap.markerReference = markerHandle;
				a_trap.markerReferenceOwner = marker;
				marker->SetActivationBlocked(true);
				marker->SetCollision(false);
				QueueTrapMarkerAnimation(a_trap, TrapMarkerAnimationPhase::kInitial, a_now);
				ProcessTrapMarkerAnimation(a_trap, a_now);
			} else if (detail::ShouldDeferUnresolvedPlacedTrapMarker(handleAllocated, spawned)) {
				deferredCleanupQueued = _trapState.QueuePendingMarkerCleanup(markerHandle);
				if (!deferredCleanupQueued) {
					// The cap calculation guarantees a free queue slot. Preserve the
					// handle on the live trap if that invariant is ever violated.
					a_trap.markerReference = markerHandle;
					SKSE::log::critical(
						"CalamityAffixes: unresolved newly-created trap marker could not enter the fixed cleanup queue (handle=0x{:X}); retaining it on the trap fail-closed.",
						markerHandle.native_handle());
				}
			}
			// Record the attempted phase even when allocation is capped or fails.
			// This permits one unarmed attempt and one armed attempt without retrying
			// CreateReferenceAtLocation every trap tick.
			a_trap.visualState = a_state;
			if (_loot.debugLog) {
				const auto* baseEditorID = a_trap.feedback.markerWorldObject->GetFormEditorID();
				SKSE::log::debug(
					"CalamityAffixes: trap world marker spawn (base={}, baseForm=0x{:X}, ref=0x{:X}, handle=0x{:X}, pos=({:.1f}, {:.1f}, {:.1f}), forcePersist=false, handleAllocated={}, resolved={}, strongOwnerRetained={}, deferredCleanupQueued={}, retainedForCleanup={}, activationBlockIssued={}, collisionDisableIssued={}, spawned={}).",
					baseEditorID ? baseEditorID : "<none>",
					a_trap.feedback.markerWorldObject->GetFormID(),
					marker ? marker->GetFormID() : 0u,
					markerHandle ? markerHandle.native_handle() : 0u,
					a_trap.position.x,
					a_trap.position.y,
					a_trap.position.z,
					handleAllocated,
					spawned,
					static_cast<bool>(a_trap.markerReferenceOwner),
					deferredCleanupQueued,
					static_cast<bool>(a_trap.markerReference) || deferredCleanupQueued,
					spawned,
					spawned,
					spawned);
			}
			return;
		}

		StopTrapMarker(a_trap, "particle-state-transition");

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
		std::string_view removalReason{ "unknown" };
		switch (a_reason) {
		case TrapRemovalReason::kExpired:
			removalReason = "expired";
			break;
		case TrapRemovalReason::kConsumed:
			removalReason = "consumed";
			break;
		case TrapRemovalReason::kPerAffixCap:
			removalReason = "per-affix-cap";
			break;
		case TrapRemovalReason::kGlobalCap:
			removalReason = "global-cap";
			break;
		case TrapRemovalReason::kInvalid:
			removalReason = "invalid";
			break;
		case TrapRemovalReason::kReset:
			removalReason = "reset";
			break;
		}
		StopTrapMarker(trap, removalReason);
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
		_trapState.RefreshRuntimeWorkFlag();
	}

	void EventBridge::SpawnTrapMarkerProbe()
	{
		// Debug-only production-contract probe. It resolves the six shipping trap
		// feedback definitions and drives the same placed-reference, animation,
		// and cleanup functions as live traps. Logs are engine-path observations;
		// only an eyes-on game session can decide whether a marker was visible.
		if (!(_loot.debugHudNotifications || _loot.debugLog)) {
			return;
		}
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return;
		}
		// Placed-reference creation and animation graph access are main-thread
		// operations. Normal TrapSystem ticks remain the sole retry/cleanup owner.
		tasks->AddTask([this]() {
			const std::scoped_lock lock(_stateMutex);
			if (_runtimeSettings.disableTrapSystemTick) {
				SKSE::log::warn(
					"CalamityAffixes: trap world marker probe skipped (reason=trap-system-tick-disabled).");
				EmitHudNotification("Calamity: world-ref probe needs TrapSystem tick enabled.");
				return;
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* cell = player ? player->GetParentCell() : nullptr;
			if (!player || !cell || !cell->IsAttached()) {
				EmitHudNotification("Calamity: world-ref probe needs an attached player cell.");
				return;
			}

			struct ProbeContract
			{
				std::string_view affixId;
				std::uint64_t sourceToken;
			};
			static constexpr std::array<ProbeContract, 6> kProbeContracts{{
				{ "bear_trap", 0x4341464650524F01ull },
				{ "rune_trap", 0x4341464650524F02ull },
				{ "plague_spore", 0x4341464650524F03ull },
				{ "tar_blight", 0x4341464650524F04ull },
				{ "siphon_spore", 0x4341464650524F05ull },
				{ "chaos_rune", 0x4341464650524F06ull },
			}};
			const auto probeAlreadyRunning = std::any_of(
				_trapState.activeTraps.begin(),
				_trapState.activeTraps.end(),
				[](const TrapInstance& a_trap) {
					return std::any_of(
						kProbeContracts.begin(),
						kProbeContracts.end(),
						[&](const ProbeContract& a_contract) {
							return a_trap.sourceToken == a_contract.sourceToken;
						});
				});
			if (probeAlreadyRunning) {
				EmitHudNotification("Calamity: production trap world-ref probe is already running.");
				return;
			}

			ProcessPendingTrapMarkerCleanup();
			const auto logicalTrapCount = _trapState.activeTraps.size();
			if (!detail::CanReserveLogicalTrapSlots(
					logicalTrapCount,
					static_cast<std::size_t>(_loot.trapGlobalMaxActive),
					kProbeContracts.size())) {
				SKSE::log::warn(
					"CalamityAffixes: trap world marker probe skipped (reason=logical-trap-headroom, active={}, requested={}, cap={}).",
					logicalTrapCount,
					kProbeContracts.size(),
					_loot.trapGlobalMaxActive);
				EmitHudNotification("Calamity: world-ref probe skipped; logical trap cap has insufficient headroom.");
				return;
			}
			const auto activePlacedCount = static_cast<std::size_t>(std::count_if(
				_trapState.activeTraps.begin(),
				_trapState.activeTraps.end(),
				[](const TrapInstance& a_trap) { return static_cast<bool>(a_trap.markerReference); }));
			const auto pendingCleanupCount = _trapState.PendingMarkerCleanupCount();
			if (activePlacedCount + pendingCleanupCount + kProbeContracts.size() >
				detail::kMaxPlacedTrapMarkers) {
				SKSE::log::warn(
					"CalamityAffixes: trap world marker probe skipped (reason=placed-ref-headroom, active={}, pendingCleanup={}, requested={}, cap={}).",
					activePlacedCount,
					pendingCleanupCount,
					kProbeContracts.size(),
					detail::kMaxPlacedTrapMarkers);
				EmitHudNotification("Calamity: world-ref probe skipped; marker cap has insufficient headroom.");
				return;
			}

			const auto basePos = player->GetPosition();
			const auto heading = player->GetAngleZ();
			const auto headingSin = std::sin(heading);
			const auto headingCos = std::cos(heading);
			const auto now = std::chrono::steady_clock::now();
			const auto probeWindow = detail::BuildTrapWorldMarkerProbeWindow(now);
			std::size_t configuredCount = 0u;
			for (std::size_t index = 0; index < kProbeContracts.size(); ++index) {
				const auto& contract = kProbeContracts[index];
				const auto affixId = contract.affixId;
				const auto affixIt = _affixRuntimeState.affixRegistry.affixIndexById.find(std::string(affixId));
				const bool affixResolved = affixIt != _affixRuntimeState.affixRegistry.affixIndexById.end() &&
					affixIt->second < _affixRuntimeState.affixes.size();
				const auto* affix = affixResolved ? std::addressof(_affixRuntimeState.affixes[affixIt->second]) : nullptr;
				const bool configured = affix &&
					affix->action.type == ActionType::kSpawnTrap &&
					affix->action.spell &&
					affix->action.trapRadius > 0.0f &&
					affix->action.trapFeedback.configured &&
					affix->action.trapFeedback.markerWorldObject;
				if (!configured) {
					SKSE::log::info(
						"CalamityAffixes: trap world marker probe observation (affixId={}, configured=false, handleAllocated=false, resolved=false, animationConfigured=false).",
						affixId);
					continue;
				}

				TrapInstance probe{};
				probe.sourceToken = contract.sourceToken;
				probe.ownerFormID = player->GetFormID();
				probe.position = basePos;
				const auto rightOffset = (static_cast<float>(index % 3u) - 1.0f) * 128.0f;
				const auto forwardOffset = (static_cast<float>(index / 3u) + 1.0f) * 128.0f;
				probe.position.x += headingCos * rightOffset + headingSin * forwardOffset;
				probe.position.y += -headingSin * rightOffset + headingCos * forwardOffset;
				probe.cell = cell;
				probe.radius = affix->action.trapRadius;
				probe.spell = affix->action.spell;
				// The normal trap tick owns animation retries and cleanup. Its prune
				// pass runs before casts, and this probe's arm time is deliberately
				// after expiry, so no gameplay spell can fire in its live window.
				probe.expiresAt = probeWindow.expiresAt;
				probe.armedAt = probeWindow.armedAt;
				probe.createdAt = now + std::chrono::nanoseconds(index);
				probe.feedback = affix->action.trapFeedback;
				_trapState.activeTraps.push_back(std::move(probe));
				_trapState.hasActiveTraps.store(true, std::memory_order_relaxed);
				auto& storedProbe = _trapState.activeTraps.back();
				StartTrapMarker(storedProbe, TrapVisualState::kUnarmed, now);

				const bool handleAllocated = static_cast<bool>(storedProbe.markerReference);
				const bool resolved = storedProbe.markerReferenceOwner != nullptr ||
					(handleAllocated && storedProbe.markerReference.get() != nullptr);
				const auto* markerEditorId = storedProbe.feedback.markerWorldObject->GetFormEditorID();
				SKSE::log::info(
					"CalamityAffixes: trap world marker probe observation (affixId={}, marker={}, configured=true, handleAllocated={}, resolved={}, animationConfigured={}).",
					affixId,
					markerEditorId ? markerEditorId : "<none>",
					handleAllocated,
					resolved,
					storedProbe.feedback.worldMarkerAnimation.has_value());
				++configuredCount;
			}

			if (configuredCount == 0u) {
				EmitHudNotification("Calamity: world-ref probe found no configured production traps.");
				return;
			}
			EmitHudNotification("Calamity: production trap world-ref probe started; auto-cleanup in 3s.");
		});
	}

	void EventBridge::ClearTrapRuntimeState(
		std::string_view a_reason,
		bool a_discardUnresolvedForWorldTransition) noexcept
	{
		ProcessPendingTrapMarkerCleanup();
		for (auto& trap : _trapState.activeTraps) {
			StopTrapMarker(trap, a_reason);
		}
		_trapState.Reset();
		ProcessPendingTrapMarkerCleanup();
		if (a_discardUnresolvedForWorldTransition) {
			const auto discardedCount = _trapState.PendingMarkerCleanupCount();
			if (_loot.debugLog) {
				for (auto& pendingHandle : _trapState.pendingMarkerCleanup) {
					if (pendingHandle) {
						SKSE::log::debug(
							"CalamityAffixes: trap world marker cleanup (reason={}, handle=0x{:X}, resolved=false, discardedForWorldTransition=true).",
							a_reason,
							pendingHandle.native_handle());
					}
				}
				if (discardedCount != 0u) {
					SKSE::log::debug(
						"CalamityAffixes: discarded unresolved trap marker handles for world transition (reason={}, count={}).",
						a_reason,
						discardedCount);
				}
			}
			_trapState.DiscardPendingMarkerCleanup();
		}
	}
}
