#include "CalamityAffixes/ResyncScheduler.h"

using CalamityAffixes::ResyncScheduler;

static_assert([] {
	ResyncScheduler s{ .nextAtMs = 0, .intervalMs = 5000 };
	return s.ShouldRun(0);
}());

static_assert(![] {
	ResyncScheduler s{ .nextAtMs = 0, .intervalMs = 5000 };
	(void)s.ShouldRun(0);
	return s.ShouldRun(1000);
}());

static_assert([] {
	ResyncScheduler s{ .nextAtMs = 0, .intervalMs = 5000 };
	(void)s.ShouldRun(0);
	return s.ShouldRun(5000);
}());
