#include "CalamityAffixes/StatusLedger.h"

#include <array>

namespace
{
	using CalamityAffixes::DoomPending;
	using CalamityAffixes::DoomQueue;
	using CalamityAffixes::ParseStatusKind;
	using CalamityAffixes::StatusEntry;
	using CalamityAffixes::StatusKind;
	using CalamityAffixes::StatusLedger;

	constexpr std::uint32_t kTarget = 0x1234u;
	constexpr std::uint32_t kOther = 0x5678u;
	constexpr std::uint32_t kFireRes = 41u;
	constexpr std::uint32_t kShockRes = 43u;

	constexpr StatusEntry Entry(std::uint32_t a_target, StatusKind a_kind, std::uint32_t a_spell, std::uint32_t a_av,
		float a_magnitude, std::int64_t a_expires)
	{
		return { a_target, a_kind, a_spell, a_av, a_magnitude, a_expires };
	}

	constexpr bool CheckCountsDistinctKinds()
	{
		StatusLedger ledger{};
		ledger.Record(Entry(kTarget, StatusKind::kExposed, 1u, kFireRes, 15.0f, 4000));
		ledger.Record(Entry(kTarget, StatusKind::kExposed, 2u, kShockRes, 25.0f, 4000));
		if (ledger.CountDistinctKinds(kTarget, 1000) != 1u) return false;  // two Exposed count once
		ledger.Record(Entry(kTarget, StatusKind::kDoom, 3u, 0u, 50.0f, 2500));
		if (ledger.CountDistinctKinds(kTarget, 1000) != 2u) return false;
		if (ledger.CountDistinctKinds(kTarget, 3000) != 1u) return false;  // Doom burst time passed
		if (ledger.CountDistinctKinds(kOther, 1000) != 0u) return false;
		return ledger.CountDistinctKinds(kTarget, 5000) == 0u;
	}

	constexpr bool CheckStrongestAndRefresh()
	{
		StatusLedger ledger{};
		ledger.Record(Entry(kTarget, StatusKind::kExposed, 1u, kFireRes, 15.0f, 4000));
		ledger.Record(Entry(kTarget, StatusKind::kExposed, 2u, kFireRes, 25.0f, 6000));
		const auto best = ledger.Strongest(kTarget, StatusKind::kExposed, kFireRes, 1000);
		if (!best || best->spellFormID != 2u) return false;
		const auto other = ledger.Strongest(kTarget, StatusKind::kExposed, kFireRes, 1000, 2u);
		if (!other || other->spellFormID != 1u) return false;
		if (ledger.Strongest(kTarget, StatusKind::kExposed, kShockRes, 1000)) return false;
		// Re-applying the same spell refreshes instead of adding a row.
		ledger.Record(Entry(kTarget, StatusKind::kExposed, 1u, kFireRes, 15.0f, 9000));
		if (ledger.Size() != 2u) return false;
		ledger.EraseSpell(kTarget, 1u, kFireRes);
		return ledger.Size() == 1u;
	}

	constexpr bool CheckCollectAndPrune()
	{
		StatusLedger ledger{};
		ledger.Record(Entry(kTarget, StatusKind::kExposed, 1u, kFireRes, 15.0f, 4000));
		ledger.Record(Entry(kTarget, StatusKind::kDoom, 3u, 0u, 50.0f, 2000));
		ledger.Record(Entry(kOther, StatusKind::kExposed, 1u, kFireRes, 15.0f, 4000));
		std::array<StatusEntry, 8> out{};
		if (ledger.Collect(kTarget, 1000, out) != 2u) return false;
		if (ledger.Collect(kTarget, 3000, out) != 1u) return false;
		ledger.Prune(3000);
		if (ledger.Size() != 2u) return false;
		ledger.EraseTarget(kTarget);
		return ledger.Size() == 1u;
	}

	constexpr bool CheckFullLedgerEvictsSoonestToExpire()
	{
		StatusLedger ledger{};
		for (std::uint32_t i = 0; i < StatusLedger::kCapacity; ++i) {
			ledger.Record(Entry(100u + i, StatusKind::kExposed, 1u, kFireRes, 1.0f, 10000 + i));
		}
		ledger.Record(Entry(kTarget, StatusKind::kDoom, 3u, 0u, 9.0f, 5000));
		return ledger.Size() == StatusLedger::kCapacity &&
			ledger.CountDistinctKinds(kTarget, 0) == 1u &&
			ledger.CountDistinctKinds(100u, 0) == 0u;  // 100 expired soonest
	}

	constexpr bool CheckDoomRefreshesWithoutStacking()
	{
		DoomQueue queue{};
		if (!queue.Mark({ 7u, kTarget, 40.0f, 1500 })) return false;
		if (!queue.Mark({ 7u, kTarget, 25.0f, 2500 })) return false;
		if (queue.Size() != 1u) return false;
		const auto pending = queue.Find(kTarget);
		if (!pending || pending->magnitude != 40.0f || pending->fireAtMs != 2500) return false;
		std::array<DoomPending, 4> due{};
		if (queue.TakeDue(2000, due) != 0u) return false;
		if (queue.TakeDue(2500, due) != 1u || due[0].target != kTarget) return false;
		return queue.Empty();
	}

	constexpr bool CheckDoomQueueBound()
	{
		DoomQueue queue{};
		for (std::uint32_t i = 0; i < DoomQueue::kCapacity; ++i) {
			if (!queue.Mark({ 1u, 100u + i, 1.0f, 1000 })) return false;
		}
		return !queue.Mark({ 1u, kTarget, 1.0f, 1000 }) && queue.Mark({ 1u, 100u, 2.0f, 1200 });
	}
}

static_assert(ParseStatusKind("Exposed") == StatusKind::kExposed && ParseStatusKind("Doom") == StatusKind::kDoom &&
	ParseStatusKind("Burning") == StatusKind::kBurning && ParseStatusKind("exposed") == StatusKind::kNone,
	"status names are exact");
static_assert(CheckCountsDistinctKinds(), "statuses count by kind, not by spell, and expire on time");
static_assert(CheckStrongestAndRefresh(), "the ledger finds the strongest of a kind and refreshes re-applied spells");
static_assert(CheckCollectAndPrune(), "Contagion collects only a target's live statuses");
static_assert(CheckFullLedgerEvictsSoonestToExpire(), "a full ledger drops the entry closest to expiring");
static_assert(CheckDoomRefreshesWithoutStacking(), "re-marking Doom moves the timer and keeps the larger burst");
static_assert(CheckDoomQueueBound(), "the Doom queue is bounded but still refreshes marked targets");
