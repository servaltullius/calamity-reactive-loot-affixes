#include "CalamityAffixes/PluginEditorIds.h"

#include "CalamityAffixes/PluginEditorIdParser.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/RuntimePaths.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <unordered_map>

#include <SKSE/SKSE.h>

namespace CalamityAffixes::PluginEditorIds
{
	namespace
	{
		struct Index
		{
			std::unordered_map<std::string, RE::TESForm*> byEditorId;  // lower-case keys
			std::unordered_map<RE::FormID, std::string> byFormId;
		};

		Index& GetIndex() noexcept
		{
			static Index index;
			return index;
		}

		[[nodiscard]] std::string ToLower(std::string_view a_text)
		{
			std::string lowered(a_text);
			std::ranges::transform(lowered, lowered.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return lowered;
		}

		using Po3GetFormEditorID = const char* (*)(std::uint32_t);

		[[nodiscard]] Po3GetFormEditorID ResolvePo3GetFormEditorID() noexcept
		{
			const auto tweaks = GetModuleHandle(L"po3_Tweaks.dll");
			if (!tweaks) {
				return nullptr;
			}
			return reinterpret_cast<Po3GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));
		}
	}

	void Load()
	{
		auto& index = GetIndex();
		index.byEditorId.clear();
		index.byFormId.clear();

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kPluginName)) {
			SKSE::log::warn("CalamityAffixes: editor ID index skipped ({} is not loaded).", kPluginName);
			return;
		}

		const auto path = RuntimePaths::ResolveRuntimeRelativePath(std::string("Data/") + std::string(kPluginName));
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			SKSE::log::error("CalamityAffixes: editor ID index failed (cannot open {}).", path.string());
			return;
		}
		const std::string bytes{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

		const auto parsed = PluginEditorIdParser::Parse(bytes);
		if (!parsed.ok) {
			SKSE::log::error(
				"CalamityAffixes: editor ID index failed ({} is malformed after {} records).",
				kPluginName,
				parsed.entries.size());
			return;
		}

		std::size_t unresolved = 0;
		for (const auto& entry : parsed.entries) {
			auto* form = handler->LookupForm(entry.localFormId, kPluginName);
			if (!form) {
				++unresolved;
				continue;
			}
			index.byEditorId.emplace(ToLower(entry.editorId), form);
			index.byFormId.emplace(form->GetFormID(), entry.editorId);
		}

		SKSE::log::info(
			"CalamityAffixes: editor ID index built ({} forms from {}, {} unresolved, {} compressed skipped).",
			index.byFormId.size(),
			kPluginName,
			unresolved,
			parsed.compressedSkipped);
	}

	RE::TESForm* FindOwn(std::string_view a_editorId) noexcept
	{
		if (a_editorId.empty()) {
			return nullptr;
		}
		try {
			const auto& index = GetIndex();
			const auto it = index.byEditorId.find(ToLower(a_editorId));
			return it != index.byEditorId.end() ? it->second : nullptr;
		} catch (...) {
			return nullptr;
		}
	}

	std::string_view OwnEditorIdOf(const RE::TESForm* a_form) noexcept
	{
		if (!a_form) {
			return {};
		}
		const auto& index = GetIndex();
		const auto it = index.byFormId.find(a_form->GetFormID());
		return it != index.byFormId.end() ? std::string_view(it->second) : std::string_view{};
	}

	std::string EditorIdOf(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}
		if (const auto own = OwnEditorIdOf(a_form); !own.empty()) {
			return std::string(own);
		}
		if (const auto engine = SafeCStringView(a_form->GetFormEditorID()); !engine.empty()) {
			return std::string(engine);
		}
		static const auto po3GetFormEditorID = ResolvePo3GetFormEditorID();
		if (po3GetFormEditorID) {
			return std::string(SafeCStringView(po3GetFormEditorID(a_form->GetFormID())));
		}
		return {};
	}
}
