#include "CalamityAffixes/Utf8Text.h"

#include <string_view>

using namespace CalamityAffixes::Utf8Text;
using namespace std::string_view_literals;

static_assert(IsValidUtf8("Iron Sword"sv), "ASCII is valid UTF-8");
static_assert(IsValidUtf8("\xEC\xB2\xA0 \xEA\xB2\x80"sv), "Korean UTF-8 is valid");
static_assert(IsValidUtf8(""sv), "an empty name is valid");
static_assert(!IsValidUtf8("Mj\xF6lnir"sv), "a Windows-1252 o-umlaut is not UTF-8");
static_assert(!IsValidUtf8("Blade \x96 Refined"sv), "a Windows-1252 en dash is not UTF-8");
static_assert(!IsValidUtf8("\xC0\xAF"sv), "overlong forms are rejected");
static_assert(!IsValidUtf8("\xED\xA0\x80"sv), "UTF-16 surrogates are rejected");
static_assert(!IsValidUtf8("\xE2\x82"sv), "a truncated sequence is rejected");

static_assert(RepairToUtf8("Mj\xF6lnir"sv) == "Mj\xC3\xB6lnir"sv, "o-umlaut becomes U+00F6");
static_assert(RepairToUtf8("Blade \x96 Refined"sv) == "Blade \xE2\x80\x93 Refined"sv, "0x96 becomes an en dash");
static_assert(RepairToUtf8("Caf\xE9"sv) == "Caf\xC3\xA9"sv, "e-acute becomes U+00E9");
static_assert(RepairToUtf8("\x81"sv) == "\xEF\xBF\xBD"sv, "an unassigned byte becomes U+FFFD");
static_assert(RepairToUtf8("Mj\xF6lnir \xEC\xB2\xA0"sv) == "Mj\xC3\xB6lnir \xEC\xB2\xA0"sv,
	"valid UTF-8 inside a mixed name is kept");
static_assert(IsValidUtf8(RepairToUtf8("\xFF\xFE\x80\x9F\xC3"sv)), "repair always yields valid UTF-8");
static_assert(RepairToUtf8("Iron Sword"sv) == "Iron Sword"sv, "valid text is unchanged");
