#include "CalamityAffixes/CorpseHitFilter.h"

namespace
{
	using CalamityAffixes::CorpseHitFilter;

	constexpr bool CheckKillingBlowThenCorpseHits()
	{
		CorpseHitFilter filter{};
		return filter.AdmitDeadTargetHit(0x10u, 0) && !filter.AdmitDeadTargetHit(0x10u, 200) &&
		       !filter.AdmitDeadTargetHit(0x10u, 29000);
	}

	constexpr bool CheckTargetsAreSeparate()
	{
		CorpseHitFilter filter{};
		return filter.AdmitDeadTargetHit(0x10u, 0) && filter.AdmitDeadTargetHit(0x20u, 100);
	}

	constexpr bool CheckMemoryExpires()
	{
		// A target dead again long after (revived and killed) gets a new killing blow.
		CorpseHitFilter filter{};
		return filter.AdmitDeadTargetHit(0x10u, 0) && filter.AdmitDeadTargetHit(0x10u, CorpseHitFilter::kMemoryMs);
	}

	constexpr bool CheckRingReusesOldest()
	{
		CorpseHitFilter filter{};
		for (std::uint32_t i = 1u; i <= CorpseHitFilter::kCapacity; ++i) {
			(void)filter.AdmitDeadTargetHit(i, 0);
		}
		// Target 1 was pushed out by the 33rd, so it reads as new again.
		(void)filter.AdmitDeadTargetHit(100u, 0);
		return filter.AdmitDeadTargetHit(1u, 0) && !filter.AdmitDeadTargetHit(3u, 0);
	}

	constexpr bool CheckNullTargetNeverAdmitted()
	{
		CorpseHitFilter filter{};
		return !filter.AdmitDeadTargetHit(0u, 0);
	}
}

static_assert(CheckKillingBlowThenCorpseHits(), "the first hit on a dead target passes, later ones are corpse hits");
static_assert(CheckTargetsAreSeparate(), "each dead target gets its own killing blow");
static_assert(CheckMemoryExpires(), "a dead target is forgotten after the memory window");
static_assert(CheckRingReusesOldest(), "a full ring forgets the oldest target");
static_assert(CheckNullTargetNeverAdmitted(), "a hit without a target is never admitted");

namespace
{
	using CalamityAffixes::ClassifyDeadTargetHit;
	using CalamityAffixes::DeadTargetHitVerdict;
	using CalamityAffixes::kKillingBlowAfterDeathWindowMs;
}

static_assert(ClassifyDeadTargetHit(true, 1100, 1000) == DeadTargetHitVerdict::kKillingBlow,
	"a hit right after the observed death is the killing blow");
static_assert(ClassifyDeadTargetHit(true, 990, 1000) == DeadTargetHitVerdict::kKillingBlow,
	"a hit observed just before its death event is the killing blow");
static_assert(ClassifyDeadTargetHit(true, 1000 + kKillingBlowAfterDeathWindowMs + 1, 1000) == DeadTargetHitVerdict::kCorpseHit,
	"a first hit on a body that died earlier (DoT, companion) is a corpse hit");
static_assert(ClassifyDeadTargetHit(true, 36000, 1000) == DeadTargetHitVerdict::kCorpseHit,
	"a corpse struck 35 s later is still a corpse");
static_assert(ClassifyDeadTargetHit(false, 36000, 0) == DeadTargetHitVerdict::kUnknownDeath,
	"without a death record the first-hit memory decides");
