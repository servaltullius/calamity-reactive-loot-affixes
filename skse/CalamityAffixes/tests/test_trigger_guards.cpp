#include "CalamityAffixes/TriggerGuards.h"

using CalamityAffixes::ComputePerTargetCooldownNextAllowedMs;
using CalamityAffixes::DuplicateHitKey;
using CalamityAffixes::IsPerTargetCooldownBlockedMs;
using CalamityAffixes::ResolveTriggerProcCooldownMs;
using CalamityAffixes::ShouldProcessHealthDamageProcPath;
using CalamityAffixes::ShouldSendPlayerOwnedHitEvent;
using CalamityAffixes::ShouldSuppressDuplicateHitWindow;

static_assert([] {
	constexpr DuplicateHitKey current{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	constexpr DuplicateHitKey last{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	return ShouldSuppressDuplicateHitWindow(current, last, 150u, 100u, 100u);
}(),
	"ShouldSuppressDuplicateHitWindow: suppresses exact duplicate hits within the time window");

static_assert([] {
	constexpr DuplicateHitKey current{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	constexpr DuplicateHitKey last{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	return !ShouldSuppressDuplicateHitWindow(current, last, 200u, 100u, 100u);
}(),
	"ShouldSuppressDuplicateHitWindow: does not suppress at window boundary");

static_assert([] {
	constexpr DuplicateHitKey current{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 4u };
	constexpr DuplicateHitKey last{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	return !ShouldSuppressDuplicateHitWindow(current, last, 150u, 100u, 100u);
}(),
	"ShouldSuppressDuplicateHitWindow: allows non-identical hits even inside window");

static_assert([] {
	constexpr DuplicateHitKey current{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	constexpr DuplicateHitKey last{ .outgoing = true, .aggressor = 1u, .target = 2u, .source = 3u };
	return !ShouldSuppressDuplicateHitWindow(current, last, 90u, 100u, 100u);
}(),
	"ShouldSuppressDuplicateHitWindow: does not suppress when clock moves backwards");

static_assert([] {
	const auto nextAllowed = ComputePerTargetCooldownNextAllowedMs(1000u, 250, 0xA5u, 0x14u);
	return nextAllowed.has_value() && *nextAllowed == 1250u;
}(),
	"ComputePerTargetCooldownNextAllowedMs: computes next allowed tick for valid inputs");

static_assert([] {
	return !ComputePerTargetCooldownNextAllowedMs(1000u, 0, 0xA5u, 0x14u).has_value();
}(),
	"ComputePerTargetCooldownNextAllowedMs: rejects zero ICD");

static_assert([] {
	return !ComputePerTargetCooldownNextAllowedMs(1000u, 250, 0u, 0x14u).has_value();
}(),
	"ComputePerTargetCooldownNextAllowedMs: rejects zero token");

static_assert([] {
	return !ComputePerTargetCooldownNextAllowedMs(1000u, 250, 0xA5u, 0u).has_value();
}(),
	"ComputePerTargetCooldownNextAllowedMs: rejects zero target");

static_assert([] {
	return IsPerTargetCooldownBlockedMs(1200u, 1300u);
}(),
	"IsPerTargetCooldownBlockedMs: blocks before next allowed time");

static_assert([] {
	return !IsPerTargetCooldownBlockedMs(1300u, 1300u);
}(),
	"IsPerTargetCooldownBlockedMs: allows exactly at next allowed time");

static_assert([] {
	return ResolveTriggerProcCooldownMs(600, false, 100.0f, 120) == 600;
}(),
	"ResolveTriggerProcCooldownMs: keeps configured positive ICD");

static_assert([] {
	return ResolveTriggerProcCooldownMs(0, true, 100.0f, 120) == 0;
}(),
	"ResolveTriggerProcCooldownMs: keeps zero ICD when per-target ICD is enabled");

static_assert([] {
	return ResolveTriggerProcCooldownMs(0, false, 100.0f, 120) == 120;
}(),
	"ResolveTriggerProcCooldownMs: applies zero-ICD safety guard for guaranteed proc chance");

static_assert([] {
	return ResolveTriggerProcCooldownMs(0, false, 35.0f, 120) == 0;
}(),
	"ResolveTriggerProcCooldownMs: leaves non-guaranteed chance unchanged when ICD is zero");

static_assert([] {
	return ShouldSendPlayerOwnedHitEvent(true, true, true, false, false);
}(),
	"ShouldSendPlayerOwnedHitEvent: sends only when player-owned hit is hostile");

static_assert([] {
	return !ShouldSendPlayerOwnedHitEvent(true, true, false, false, false);
}(),
	"ShouldSendPlayerOwnedHitEvent: blocks non-hostile player-owned hits");

static_assert([] {
	return !ShouldSendPlayerOwnedHitEvent(true, false, true, false, false);
}(),
	"ShouldSendPlayerOwnedHitEvent: blocks unresolved player owner");

static_assert([] {
	return ShouldSendPlayerOwnedHitEvent(true, true, false, true, false);
}(),
	"ShouldSendPlayerOwnedHitEvent: optionally allows non-hostile outgoing events for non-player targets");

static_assert([] {
	return !ShouldSendPlayerOwnedHitEvent(true, true, false, true, true);
}(),
	"ShouldSendPlayerOwnedHitEvent: keeps non-hostile incoming-to-player blocked");

static_assert([] {
	return ShouldProcessHealthDamageProcPath(
		true,
		true,
		true,
		false,
		false,
		true,
		false);
}(),
	"ShouldProcessHealthDamageProcPath: processes incoming hostile damage to player");

static_assert([] {
	return ShouldProcessHealthDamageProcPath(
		true,
		true,
		false,
		true,
		true,
		true,
		false);
}(),
	"ShouldProcessHealthDamageProcPath: processes outgoing hostile damage from player-owned attacker");

static_assert([] {
	return !ShouldProcessHealthDamageProcPath(
		true,
		true,
		false,
		true,
		true,
		false,
		false);
}(),
	"ShouldProcessHealthDamageProcPath: blocks non-hostile pairs");

static_assert([] {
	return !ShouldProcessHealthDamageProcPath(
		true,
		false,
		true,
		false,
		false,
		false,
		false);
}(),
	"ShouldProcessHealthDamageProcPath: blocks events without attacker");

static_assert([] {
	return !ShouldProcessHealthDamageProcPath(
		true,
		true,
		false,
		false,
		false,
		true,
		false);
}(),
	"ShouldProcessHealthDamageProcPath: blocks non-player-owned outgoing paths");

static_assert([] {
	return ShouldProcessHealthDamageProcPath(
		true,
		true,
		false,
		true,
		true,
		false,
		true);
}(),
	"ShouldProcessHealthDamageProcPath: allows optional non-hostile outgoing player-owned path");
