#include "CalamityAffixes/PausedTimeClock.h"

namespace
{
	using CalamityAffixes::PausedTimeClock;

	constexpr bool CheckRunsWithWallTimeWhenNeverPaused()
	{
		PausedTimeClock clock{};
		return clock.Now(1000) == 1000 && clock.Now(2500) == 2500;
	}
	static_assert(CheckRunsWithWallTimeWhenNeverPaused());

	constexpr bool CheckStopsWhilePaused()
	{
		// An 8 s Exposed recorded at 1000 ms, then 10 s in a pausing menu.
		PausedTimeClock clock{};
		clock.SetPaused(true, 2000);
		const bool frozen = clock.Now(5000) == 2000 && clock.Now(12000) == 2000;
		clock.SetPaused(false, 12000);
		// One more real second of play is one more second on the clock.
		return frozen && clock.Now(13000) == 3000 && !clock.IsPaused();
	}
	static_assert(CheckStopsWhilePaused());

	constexpr bool CheckRepeatedCallsAndPausesAddUp()
	{
		PausedTimeClock clock{};
		clock.SetPaused(true, 100);
		clock.SetPaused(true, 400);  // already paused: the pause still starts at 100
		clock.SetPaused(false, 600);
		clock.SetPaused(false, 700);  // already running: nothing to add
		clock.SetPaused(true, 1000);
		clock.SetPaused(false, 1300);
		// Paused 500 + 300 ms.
		return clock.Now(2000) == 1200;
	}
	static_assert(CheckRepeatedCallsAndPausesAddUp());

	constexpr bool CheckNeverRunsBackwards()
	{
		PausedTimeClock clock{};
		clock.SetPaused(true, 1000);
		const auto during = clock.Now(1500);
		clock.SetPaused(false, 1500);
		return clock.Now(1500) == during && clock.Now(1600) > during;
	}
	static_assert(CheckNeverRunsBackwards());
}
