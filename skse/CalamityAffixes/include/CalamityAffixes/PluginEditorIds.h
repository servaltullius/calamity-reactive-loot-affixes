#pragma once

#include <string>
#include <string_view>

#include <RE/Skyrim.h>

// Editor ID lookups that work without powerofthree's Tweaks.
//
// The game discards the editor IDs of spells, magic effects, misc items, leveled lists
// and art objects, so RE::TESForm::LookupByEditorID and GetFormEditorID() see nothing for
// them unless po3 Tweaks restores them (and GetFormEditorID() stays empty even then).
// Load() reads CalamityAffixes.esp once at kDataLoaded and indexes its records; after
// that the index is read-only, so lookups are safe from any thread.
namespace CalamityAffixes::PluginEditorIds
{
	inline constexpr std::string_view kPluginName = "CalamityAffixes.esp";

	// Builds the index. Call once at kDataLoaded, before the runtime config loads.
	void Load();

	// A form this plugin defines, by editor ID (case-insensitive), or nullptr.
	[[nodiscard]] RE::TESForm* FindOwn(std::string_view a_editorId) noexcept;

	// The editor ID of a form this plugin defines, or empty for any other form.
	[[nodiscard]] std::string_view OwnEditorIdOf(const RE::TESForm* a_form) noexcept;

	// The editor ID of any form: this plugin's index first, then the engine's (kept for
	// keywords, quests, globals and a few more), then po3 Tweaks' export when installed.
	[[nodiscard]] std::string EditorIdOf(const RE::TESForm* a_form);

	// Drop-in for RE::TESForm::LookupByEditorID: this plugin's forms resolve from the index,
	// anything else falls back to the engine.
	template <class T>
	[[nodiscard]] T* Lookup(std::string_view a_editorId)
	{
		if (auto* form = FindOwn(a_editorId)) {
			return form->As<T>();
		}
		return RE::TESForm::LookupByEditorID<T>(a_editorId);
	}
}
