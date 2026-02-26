#include "CalamityAffixes/PointerSafety.h"

#include <string_view>

using CalamityAffixes::SafeCStringView;

static_assert(SafeCStringView(nullptr).empty(),
	"SafeCStringView: null pointer must map to empty string_view");

static_assert(SafeCStringView("CAFF_Test") == std::string_view("CAFF_Test"),
	"SafeCStringView: valid c-string must be preserved");
