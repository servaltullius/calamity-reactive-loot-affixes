#include "CalamityAffixes/TriggerGuards.h"

using CalamityAffixes::ComputePerTargetCooldownNextAllowedMs;
using CalamityAffixes::DuplicateHitKey;
using CalamityAffixes::IsPerTargetCooldownBlockedMs;
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
