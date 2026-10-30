#include "CalamityAffixes/ActorValueMax.h"

namespace
{
	using CalamityAffixes::CombineActorValueMax;

	constexpr bool Near(float a_value, float a_expected)
	{
		const float diff = a_value - a_expected;
		return diff < 0.001f && diff > -0.001f;
	}

	// 300 base + 150 Fortify Health: a 40% threshold is 180 HP, not 120.
	static_assert(Near(CombineActorValueMax(300.0f, 150.0f), 450.0f));
	static_assert(Near(CombineActorValueMax(300.0f, 0.0f), 300.0f));
	// A drained maximum (negative temporary modifier) still counts.
	static_assert(Near(CombineActorValueMax(300.0f, -100.0f), 200.0f));
	// Archmage: 200 magicka + 100 Fortify Magicka, 10% of max is 30.
	static_assert(Near(CombineActorValueMax(200.0f, 100.0f) * 0.10f, 30.0f));
	// Never zero or negative while the permanent value is positive.
	static_assert(Near(CombineActorValueMax(300.0f, -400.0f), 300.0f));
	static_assert(Near(CombineActorValueMax(-5.0f, 0.0f), 0.0f));
}
