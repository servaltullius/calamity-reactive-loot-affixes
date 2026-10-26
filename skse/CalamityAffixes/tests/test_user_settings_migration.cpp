#include "CalamityAffixes/UserSettingsMigration.h"

namespace
{
	using namespace CalamityAffixes::UserSettingsMigration;

	// The old default moves to the new one once; a 2 chosen later stays.
	static_assert(MigrateScouringOrbChancePercent(0, 2.0) == 4.0);
	static_assert(MigrateScouringOrbChancePercent(1, 2.0) == 4.0);
	static_assert(MigrateScouringOrbChancePercent(2, 2.0) == 2.0);
	static_assert(MigrateScouringOrbChancePercent(1, 3.0) == 3.0);

	constexpr std::string_view kIni =
		"; comment\r\n"
		"[General]\r\n"
		"bEnabled=0\r\n"
		"fScouringOrbChancePercent = 2.0\r\n"
		"[Other]\n"
		"bEnabled=1\n";

	static_assert(FindIniValue(kIni, "General", "bEnabled") == "0");
	static_assert(FindIniValue(kIni, "general", "fscouringorbchancepercent") == "2.0");
	static_assert(FindIniValue(kIni, "Other", "bEnabled") == "1");
	static_assert(FindIniValue(kIni, "General", "fMissing").empty());
	static_assert(FindIniValue("", "General", "bEnabled").empty());
}
