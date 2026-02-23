#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Config.Shared.h"

#include <algorithm>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		using ConfigShared::Trim;

		std::string DeriveAffixLabel(std::string_view a_displayName)
		{
			// Typical convention: "Label (details): description".
			// We want a short label for list display / renaming.
			auto text = Trim(a_displayName);
			if (text.empty()) {
				return {};
			}

			const auto posColon = text.find(':');
			const auto posParen = text.find('(');

			std::size_t cut = std::string_view::npos;
			if (posColon != std::string_view::npos) {
				cut = posColon;
			}
			if (posParen != std::string_view::npos) {
				cut = (cut == std::string_view::npos) ? posParen : std::min(cut, posParen);
			}

			if (cut != std::string_view::npos) {
				text = text.substr(0, cut);
			}

			text = Trim(text);
			std::string label(text);

			auto replaceAll = [&](std::string_view a_from, std::string_view a_to) {
				if (a_from.empty()) {
					return;
				}

				std::size_t pos = 0;
				while ((pos = label.find(a_from, pos)) != std::string::npos) {
					label.replace(pos, a_from.size(), a_to);
					pos += a_to.size();
				}
			};

			// Some UI/font stacks render arrow glyphs as tofu squares.
			// Normalize into ASCII so item labels remain readable in all menus.
			replaceAll("→", "->");
			replaceAll("⇒", "->");
			replaceAll("➜", "->");
			replaceAll("↔", "<->");
			replaceAll("⟶", "->");
			return label;
		}
	}

	bool EventBridge::InitializeAffixFromJson(const nlohmann::json& a_affix, AffixRuntime& a_out) const
	{
		a_out = AffixRuntime{};
		a_out.id = a_affix.value("id", std::string{});
		a_out.token = a_out.id.empty() ? 0u : MakeAffixToken(a_out.id);
		a_out.action.sourceToken = a_out.token;
		a_out.keywordEditorId = a_affix.value("editorId", std::string{});
		if (a_out.keywordEditorId.empty()) {
			return false;
		}

		a_out.displayName = a_affix.value("name", std::string{});
		if (a_out.displayName.empty()) {
			a_out.displayName = a_out.keywordEditorId;
		}
		a_out.displayNameEn = a_affix.value("nameEn", std::string{});
		a_out.displayNameKo = a_affix.value("nameKo", std::string{});
		if (a_out.displayNameEn.empty()) {
			a_out.displayNameEn = a_out.displayName;
		}
		if (a_out.displayNameKo.empty()) {
			a_out.displayNameKo = a_out.displayName;
		}
		a_out.label = DeriveAffixLabel(a_out.displayName);
		if (a_out.label.empty()) {
			a_out.label = a_out.displayName;
		}

		a_out.keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(a_out.keywordEditorId);
		return a_out.keyword != nullptr;
	}

	void EventBridge::ApplyAffixSlotAndFamilyFromJson(const nlohmann::json& a_affix, AffixRuntime& a_out) const
	{
		auto parseAffixSlot = [](std::string_view a_slot) -> AffixSlot {
			if (a_slot == "Prefix" || a_slot == "prefix") {
				return AffixSlot::kPrefix;
			}
			if (a_slot == "Suffix" || a_slot == "suffix") {
				return AffixSlot::kSuffix;
			}
			return AffixSlot::kNone;
		};

		const auto slotIt = a_affix.find("slot");
		if (slotIt != a_affix.end() && slotIt->is_string()) {
			a_out.slot = parseAffixSlot(slotIt->get<std::string>());
		}

		const auto famIt = a_affix.find("family");
		if (famIt != a_affix.end() && famIt->is_string()) {
			a_out.family = famIt->get<std::string>();
		}
	}

	void EventBridge::ApplyAffixKidLootFromJson(const nlohmann::json& a_affix, AffixRuntime& a_out, float& a_outKidChancePct) const
	{
		const auto& kid = a_affix.value("kid", nlohmann::json::object());
		a_outKidChancePct = kid.is_object() ?
			static_cast<float>(kid.value("chance", 0.0)) :
			0.0f;

		if (a_outKidChancePct > 0.0f) {
			// Loot-time drop weighting is sourced from explicit loot weight or legacy kid.chance.
			// Keep this independent from runtime trigger proc chance.
			a_out.lootWeight = a_outKidChancePct;
		}

		if (!kid.is_object()) {
			return;
		}

		const auto type = kid.value("type", std::string{});
		a_out.lootType = ParseLootItemType(type);
	}

	bool EventBridge::ApplyAffixRuntimeGateFromJson(
		const nlohmann::json& a_runtime,
		const nlohmann::json& a_action,
		float a_kidChancePct,
		std::string_view a_actionType,
		AffixRuntime& a_out) const
	{
		if (a_out.slot == AffixSlot::kSuffix) {
			a_out.trigger = Trigger::kHit;
			a_out.procChancePct = (a_kidChancePct > 0.0f) ? a_kidChancePct : 1.0f;

			const auto passiveSpellId = a_action.value("passiveSpellEditorId", std::string{});
			if (!passiveSpellId.empty()) {
				a_out.passiveSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>(passiveSpellId);
				if (!a_out.passiveSpell) {
					SKSE::log::warn(
						"CalamityAffixes: suffix passive spell not found (affixId={}, spellEditorId={}).",
						a_out.id, passiveSpellId);
				}
			}
			a_out.critDamageBonusPct = static_cast<float>(a_action.value("critDamageBonusPct", 0.0));
			return true;
		}

		return ApplyRuntimeTriggerConfigFromJson(a_runtime, a_actionType, a_out);
	}

	void EventBridge::ApplyScrollNoConsumeChanceFromJson(const nlohmann::json& a_action, AffixRuntime& a_out) const
	{
		const float rawScrollNoConsumeChancePct = static_cast<float>(a_action.value("scrollNoConsumeChancePct", 0.0));
		a_out.scrollNoConsumeChancePct = std::clamp(rawScrollNoConsumeChancePct, 0.0f, 100.0f);
	}

	void EventBridge::NormalizeParsedAffixRuntimePolicy(AffixRuntime& a_out, std::string_view a_actionType) const
	{
		const bool isSpecialAction =
			a_out.action.type == ActionType::kCastOnCrit ||
			a_out.action.type == ActionType::kConvertDamage ||
			a_out.action.type == ActionType::kMindOverMatter ||
			a_out.action.type == ActionType::kArchmage ||
			a_out.action.type == ActionType::kCorpseExplosion ||
			a_out.action.type == ActionType::kSummonCorpseExplosion;

		if (a_out.luckyHitChancePct > 0.0f) {
			bool luckyHitSupported = false;
			switch (a_out.action.type) {
			case ActionType::kCastOnCrit:
			case ActionType::kConvertDamage:
			case ActionType::kMindOverMatter:
			case ActionType::kArchmage:
				luckyHitSupported = true;
				break;
			case ActionType::kCorpseExplosion:
			case ActionType::kSummonCorpseExplosion:
				luckyHitSupported = false;
				break;
			default:
				luckyHitSupported = (a_out.trigger == Trigger::kHit || a_out.trigger == Trigger::kIncomingHit);
				break;
			}

			if (!luckyHitSupported) {
				SKSE::log::warn(
					"CalamityAffixes: luckyHitChancePercent is ignored for unsupported affix context (affixId={}, actionType={}, trigger={}).",
					a_out.id,
					a_actionType,
					static_cast<std::uint32_t>(a_out.trigger));
				a_out.luckyHitChancePct = 0.0f;
				a_out.luckyHitProcCoefficient = 1.0f;
			}
		}

		if (isSpecialAction && a_out.perTargetIcd.count() > 0) {
			SKSE::log::warn(
				"CalamityAffixes: perTargetICDSeconds is ignored for special action (affixId={}, actionType={}).",
				a_out.id,
				a_actionType);
		}
	}

	void EventBridge::ParseConfiguredAffixesFromJson(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler)
	{
		for (const auto& a : a_affixes) {
			if (!a.is_object()) {
				continue;
			}

			AffixRuntime out{};
			if (!InitializeAffixFromJson(a, out)) {
				continue;
			}

			ApplyAffixSlotAndFamilyFromJson(a, out);

			float kidChancePct = 0.0f;
			ApplyAffixKidLootFromJson(a, out, kidChancePct);

			const auto& runtime = a.value("runtime", nlohmann::json::object());
			if (!runtime.is_object()) {
				continue;
			}

			const auto& action = runtime.value("action", nlohmann::json::object());
			if (!action.is_object()) {
				SKSE::log::warn(
					"CalamityAffixes: runtime.action must be an object (affixId={}); skipping affix.",
					out.id);
				continue;
			}
			out.action.debugNotify = action.value("debugNotify", false);
			const auto type = action.value("type", std::string{});

			if (!ApplyAffixRuntimeGateFromJson(runtime, action, kidChancePct, type, out)) {
				continue;
			}

			ApplyScrollNoConsumeChanceFromJson(action, out);

			if (!ParseRuntimeActionFromJson(action, type, a_handler, out)) {
				continue;
			}

			NormalizeParsedAffixRuntimePolicy(out, type);

			_affixes.push_back(std::move(out));
			const auto idx = _affixes.size() - 1;

			IndexAffixTriggerBucket(_affixes[idx], idx);
			IndexAffixSpecialActionBucket(_affixes[idx], idx);
		}
	}
}
