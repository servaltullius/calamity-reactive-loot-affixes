#include "CalamityAffixes/AdaptiveElement.h"

using CalamityAffixes::AdaptiveElement;
using CalamityAffixes::AdaptiveElementMode;
using CalamityAffixes::PickAdaptiveElement;

// WeakestResist picks the lowest resistance.
static_assert(
	PickAdaptiveElement(/*fire=*/10.0f, /*frost=*/20.0f, /*shock=*/30.0f, AdaptiveElementMode::kWeakestResist) ==
		AdaptiveElement::kFire,
	"WeakestResist: fire");

static_assert(
	PickAdaptiveElement(/*fire=*/30.0f, /*frost=*/10.0f, /*shock=*/20.0f, AdaptiveElementMode::kWeakestResist) ==
		AdaptiveElement::kFrost,
	"WeakestResist: frost");

// Tie-breaking is stable: Fire > Frost > Shock.
static_assert(
	PickAdaptiveElement(/*fire=*/10.0f, /*frost=*/10.0f, /*shock=*/20.0f, AdaptiveElementMode::kWeakestResist) ==
		AdaptiveElement::kFire,
	"WeakestResist: tie");

// StrongestResist picks the highest resistance.
static_assert(
	PickAdaptiveElement(/*fire=*/10.0f, /*frost=*/20.0f, /*shock=*/30.0f, AdaptiveElementMode::kStrongestResist) ==
		AdaptiveElement::kShock,
	"StrongestResist: shock");

// Tie-breaking is stable: Fire > Frost > Shock.
static_assert(
	PickAdaptiveElement(/*fire=*/10.0f, /*frost=*/30.0f, /*shock=*/30.0f, AdaptiveElementMode::kStrongestResist) ==
		AdaptiveElement::kFrost,
	"StrongestResist: tie");

