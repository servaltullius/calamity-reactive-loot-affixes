#pragma once

namespace CalamityAffixes::PrismaTooltip
{
	void Install();
	[[nodiscard]] bool IsAvailable();
	void ReportStatus();
	void SetControlPanelOpen(bool a_open);
	void ToggleControlPanel();
	// A save is about to load: drop the open-panel request so the loaded game
	// does not reopen the panel and take focus. Door and fast-travel loading
	// screens keep it (they only hide the panel while gameplay is unavailable).
	void OnPreLoadGame();
	// A game finished loading (or a new game started): create the view now, while
	// the loading screen is up, instead of on the first inventory open (2-4 s
	// hitch measured there, 2026-10-26). DOM ready follows on the worker ticks.
	void PrewarmView();
}
