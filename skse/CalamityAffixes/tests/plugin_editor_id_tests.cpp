// Host checks for the editor ID index that frees the DLL from powerofthree's Tweaks.
//
// 1. The parser reads the shipped CalamityAffixes.esp: every record it defines, with the
//    FormIDs the generator tests pin.
// 2. It skips master overrides and compressed records and rejects truncated files.
// 3. No source calls the engine's editor ID functions directly any more: they miss
//    spells, magic effects, misc items, leveled lists and art objects without po3 Tweaks,
//    and GetFormEditorID() misses them even with it.

#include "CalamityAffixes/PluginEditorIdParser.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <string_view>

namespace
{
	namespace fs = std::filesystem;
	using CalamityAffixes::PluginEditorIdParser::Parse;

	int failures = 0;

	void Expect(bool a_ok, std::string_view a_what)
	{
		if (!a_ok) {
			std::cerr << "plugin_editor_id_tests: FAILED: " << a_what << '\n';
			++failures;
		}
	}

	std::string ReadFile(const fs::path& a_path)
	{
		std::ifstream in(a_path, std::ios::binary);
		return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
	}

	void Append(std::string& a_out, std::string_view a_bytes) { a_out.append(a_bytes); }

	void AppendU32(std::string& a_out, std::uint32_t a_value)
	{
		for (int i = 0; i < 4; ++i) {
			a_out.push_back(static_cast<char>((a_value >> (8 * i)) & 0xFF));
		}
	}

	void AppendU16(std::string& a_out, std::uint16_t a_value)
	{
		a_out.push_back(static_cast<char>(a_value & 0xFF));
		a_out.push_back(static_cast<char>(a_value >> 8));
	}

	std::string Subrecord(std::string_view a_type, std::string_view a_data)
	{
		std::string out;
		Append(out, a_type);
		AppendU16(out, static_cast<std::uint16_t>(a_data.size()));
		Append(out, a_data);
		return out;
	}

	std::string Record(std::string_view a_type, std::uint32_t a_flags, std::uint32_t a_formId, std::string_view a_body)
	{
		std::string out;
		Append(out, a_type);
		AppendU32(out, static_cast<std::uint32_t>(a_body.size()));
		AppendU32(out, a_flags);
		AppendU32(out, a_formId);
		AppendU32(out, 0);
		AppendU32(out, 0);
		Append(out, a_body);
		return out;
	}

	std::string Group(std::string_view a_label, std::string_view a_contents)
	{
		std::string out = "GRUP";
		AppendU32(out, static_cast<std::uint32_t>(24 + a_contents.size()));
		Append(out, a_label);
		AppendU32(out, 0);
		AppendU32(out, 0);
		AppendU32(out, 0);
		Append(out, a_contents);
		return out;
	}

	void CheckShippedPlugin(const fs::path& a_repoRoot)
	{
		const auto bytes = ReadFile(a_repoRoot / "Data" / "CalamityAffixes.esp");
		Expect(!bytes.empty(), "Data/CalamityAffixes.esp is readable");
		const auto parsed = Parse(bytes);
		Expect(parsed.ok, "shipped plugin parses");
		Expect(parsed.masterCount == 1, "shipped plugin has one master (Skyrim.esm)");
		Expect(parsed.compressedSkipped == 0, "shipped plugin has no compressed records");

		std::map<std::string, std::uint32_t> byEditorId;
		std::set<std::uint32_t> formIds;
		for (const auto& entry : parsed.entries) {
			Expect(byEditorId.emplace(entry.editorId, entry.localFormId).second, "editor IDs are unique: " + entry.editorId);
			Expect(formIds.insert(entry.localFormId).second, "FormIDs are unique");
		}
		// RepoSpecRegressionTests pins 793 records and these tail FormIDs.
		Expect(parsed.entries.size() == 793, "all 793 records are indexed");
		const std::map<std::string, std::uint32_t> pinned{
			{ "CAFF_ARTO_VFX_SHADOW_ECHO_HIT", 0x000B17 },
			{ "CAFF_SNDR_RW_SHADOW_PUNCH", 0x000B18 },
		};
		for (const auto& [editorId, formId] : pinned) {
			const auto it = byEditorId.find(editorId);
			Expect(it != byEditorId.end() && it->second == formId, "pinned FormID for " + editorId);
		}
		// Forms the DLL looks up by name and the engine forgets.
		for (const char* editorId : { "CAFF_Misc_ReforgeOrb", "CAFF_Misc_IdentifyScroll", "CAFF_Misc_ScouringOrb",
				 "CAFF_RuneFrag_El", "CAFF_SPEL_RW_SHADOW_BOXER_ECHO", "CAFF_MGEF_RW_SHADOW_BOXER_ECHO" }) {
			Expect(byEditorId.contains(editorId), std::string("indexed: ") + editorId);
		}
	}

	void CheckSyntheticPlugins()
	{
		std::string header = Subrecord("HEDR", std::string(12, '\0')) + Subrecord("MAST", std::string("Skyrim.esm\0", 11)) +
		                     Subrecord("DATA", std::string(8, '\0'));
		const std::string tes4 = Record("TES4", 0x200, 0, header);
		const std::string own = Record("SPEL", 0, 0x01000801, Subrecord("EDID", std::string("CAFF_Own\0", 9)));
		const std::string override_ = Record("SPEL", 0, 0x00012FCD, Subrecord("EDID", std::string("Flames\0", 7)));
		const std::string compressed = Record("SPEL", 0x00040000, 0x01000802, std::string(10, 'x'));
		const std::string plugin = tes4 + Group("SPEL", own + override_ + compressed);

		const auto parsed = Parse(plugin);
		Expect(parsed.ok, "synthetic plugin parses");
		Expect(parsed.entries.size() == 1 && parsed.entries[0].editorId == "CAFF_Own" && parsed.entries[0].localFormId == 0x000801,
			"only the plugin's own record is indexed, without the load-order byte");
		Expect(parsed.compressedSkipped == 1, "compressed record is counted and skipped");

		Expect(!Parse(plugin.substr(0, plugin.size() - 3)).ok, "truncated plugin is rejected");
		Expect(!Parse("TES3").ok, "non-plugin bytes are rejected");
	}

	void CheckNoDirectEngineEditorIdCalls(const fs::path& a_projectRoot)
	{
		for (const auto* dir : { "src", "include" }) {
			for (const auto& entry : fs::recursive_directory_iterator(a_projectRoot / dir)) {
				if (!entry.is_regular_file()) {
					continue;
				}
				const auto name = entry.path().filename().string();
				if (name.starts_with("PluginEditorId")) {
					continue;
				}
				const auto text = ReadFile(entry.path());
				Expect(text.find("TESForm::LookupByEditorID") == std::string::npos,
					name + " uses PluginEditorIds::Lookup, not TESForm::LookupByEditorID");
				Expect(text.find("GetFormEditorID()") == std::string::npos,
					name + " uses PluginEditorIds::OwnEditorIdOf/EditorIdOf, not GetFormEditorID()");
			}
		}
	}
}

int main()
{
	const fs::path projectRoot = fs::path(__FILE__).parent_path().parent_path();
	const fs::path repoRoot = projectRoot.parent_path().parent_path();
	CheckShippedPlugin(repoRoot);
	CheckSyntheticPlugins();
	CheckNoDirectEngineEditorIdCalls(projectRoot);
	if (failures != 0) {
		return 1;
	}
	std::cout << "plugin_editor_id_tests: OK\n";
	return 0;
}
