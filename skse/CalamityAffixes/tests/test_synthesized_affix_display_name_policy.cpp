#include "CalamityAffixes/SynthesizedAffixDisplayNamePolicy.h"

using CalamityAffixes::detail::ResolveSynthesizedAffixDisplayName;
using CalamityAffixes::detail::ResolveSynthesizedLocalizedDisplayName;

static_assert(ResolveSynthesizedAffixDisplayName("Display", "Label", "id") == "Display");
static_assert(ResolveSynthesizedAffixDisplayName("", "Label", "id") == "Label");
static_assert(ResolveSynthesizedAffixDisplayName("", "", "id") == "id");
static_assert(ResolveSynthesizedLocalizedDisplayName("Localized", "Fallback") == "Localized");
static_assert(ResolveSynthesizedLocalizedDisplayName("", "Fallback") == "Fallback");
