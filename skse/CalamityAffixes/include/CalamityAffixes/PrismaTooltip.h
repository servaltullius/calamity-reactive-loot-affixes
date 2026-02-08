#pragma once

namespace CalamityAffixes::PrismaTooltip
{
	void Install();
	[[nodiscard]] bool IsAvailable();
	void SetControlPanelOpen(bool a_open);
	void ToggleControlPanel();
}
