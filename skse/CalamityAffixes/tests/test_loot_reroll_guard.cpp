#include "CalamityAffixes/LootRerollGuard.h"

using CalamityAffixes::LootRerollGuard;

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;
	constexpr std::uint64_t key = 0xDEADBEEF1234ull;

	// Drop: player -> world
	g.NotePlayerDrop(player, player, LootRerollGuard::kWorldContainer, 1, ref, key);

	// Pickup: world -> player should be recognized and consumed (skip reroll).
	return g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);
}());

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;
	constexpr std::uint64_t key = 0xDEADBEEF1234ull;

	g.NotePlayerDrop(player, player, LootRerollGuard::kWorldContainer, 1, ref, key);
	(void)g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);

	// The same reference should not be skipped twice.
	return !g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);
}());

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;

	// Pickup without a prior player drop should not be skipped.
	return !g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);
}());

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::FormID someContainer = 0xDEADBEEF;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;
	constexpr std::uint64_t key = 0xDEADBEEF1234ull;

	// Transfers out of the player into a container shouldn't be treated as a "drop" for this guard.
	g.NotePlayerDrop(player, player, someContainer, 1, ref, key);
	return !g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);
}());

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;
	constexpr std::uint64_t key = 0xDEADBEEF1234ull;

	g.NotePlayerDrop(player, player, LootRerollGuard::kWorldContainer, 1, ref, key);
	g.Reset();

	// Reset clears tracked refs.
	return !g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);
}());

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;
	constexpr std::uint64_t key = 0xDEADBEEF1234ull;

	// Drop: player -> world.
	g.NotePlayerDrop(player, player, LootRerollGuard::kWorldContainer, 1, ref, key);

	// If the dropped world reference is deleted before being re-picked up, the key should be returned once.
	const auto erased1 = g.ConsumeIfPlayerDropDeleted(ref);
	const auto erased2 = g.ConsumeIfPlayerDropDeleted(ref);

	return erased1.has_value() && *erased1 == key && !erased2.has_value();
}());

static_assert([] {
	LootRerollGuard g{};
	constexpr LootRerollGuard::FormID player = 0x14;
	constexpr LootRerollGuard::RefHandle ref = 0x1234;
	constexpr std::uint64_t key = 0xDEADBEEF1234ull;

	// Drop then immediate pickup: deletion consumption should not see the ref afterwards.
	g.NotePlayerDrop(player, player, LootRerollGuard::kWorldContainer, 1, ref, key);
	(void)g.ConsumeIfPlayerDropPickup(player, LootRerollGuard::kWorldContainer, player, 1, ref);

	return !g.ConsumeIfPlayerDropDeleted(ref).has_value();
}());
