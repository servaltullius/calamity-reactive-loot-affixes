#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RuntimeContract.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace CalamityAffixes
{
	bool EventBridge::ApplyRuntimeTriggerConfigFromJson(
		const nlohmann::json& a_runtime,
		std::string_view a_actionType,
		AffixRuntime& a_out) const
	{
		auto parseTrigger = [](std::string_view a_trigger) -> std::optional<Trigger> {
			if (a_trigger == RuntimeContract::kTriggerHit) {
				return Trigger::kHit;
			}
			if (a_trigger == RuntimeContract::kTriggerIncomingHit) {
				return Trigger::kIncomingHit;
			}
			if (a_trigger == RuntimeContract::kTriggerDotApply) {
				return Trigger::kDotApply;
			}
			if (a_trigger == RuntimeContract::kTriggerKill) {
				return Trigger::kKill;
			}
			if (a_trigger == RuntimeContract::kTriggerLowHealth) {
				return Trigger::kLowHealth;
			}
			return std::nullopt;
		};

		auto isSpecialActionType = [](std::string_view a_type) -> bool {
			return a_type == RuntimeContract::kActionCastOnCrit ||
			       a_type == RuntimeContract::kActionConvertDamage ||
			       a_type == RuntimeContract::kActionMindOverMatter ||
			       a_type == RuntimeContract::kActionArchmage ||
			       a_type == RuntimeContract::kActionCorpseExplosion ||
			       a_type == RuntimeContract::kActionSummonCorpseExplosion;
		};

		const auto triggerStr = a_runtime.value("trigger", std::string{});
		const auto trigger = parseTrigger(triggerStr);
		if (!trigger) {
			if (isSpecialActionType(a_actionType)) {
				// Special actions use dedicated execution paths and do not consume Trigger routing.
				// Keep loading them even if trigger text is missing/invalid.
				a_out.trigger = Trigger::kHit;
				SKSE::log::warn(
					"CalamityAffixes: special action uses invalid trigger (affixId={}, actionType={}, trigger='{}'); defaulting to Hit.",
					a_out.id,
					a_actionType,
					triggerStr);
			} else {
				SKSE::log::error(
					"CalamityAffixes: invalid trigger '{}' (affixId={}, actionType={}); skipping affix.",
					triggerStr,
					a_out.id,
					a_actionType);
				return false;
			}
		} else {
			a_out.trigger = *trigger;
		}

		a_out.procChancePct = std::clamp(a_runtime.value("procChancePercent", 0.0f), 0.0f, 100.0f);
		if (const auto lwIt = a_runtime.find("lootWeight"); lwIt != a_runtime.end() && lwIt->is_number()) {
			a_out.lootWeight = lwIt->get<float>();
		}

		const float icdSeconds = std::clamp(a_runtime.value("icdSeconds", 0.0f), 0.0f, 600.0f);
		if (icdSeconds > 0.0f) {
			a_out.icd = std::chrono::milliseconds(static_cast<std::int64_t>(icdSeconds * 1000.0f));
		}

		const float perTargetIcdSeconds = std::clamp(a_runtime.value("perTargetICDSeconds", 0.0f), 0.0f, 600.0f);
		if (perTargetIcdSeconds > 0.0f) {
			a_out.perTargetIcd = std::chrono::milliseconds(static_cast<std::int64_t>(perTargetIcdSeconds * 1000.0f));
		}

		const auto& conditions = a_runtime.value("conditions", nlohmann::json::object());
		auto readRuntimeNumber = [&](std::string_view a_key, double a_fallback = 0.0) -> double {
			const auto key = std::string(a_key);
			if (const auto it = a_runtime.find(key); it != a_runtime.end() && it->is_number()) {
				return it->get<double>();
			}
			if (conditions.is_object()) {
				if (const auto it = conditions.find(key); it != conditions.end() && it->is_number()) {
					return it->get<double>();
				}
			}
			return a_fallback;
		};

		const double requireRecentlyHitSeconds = std::clamp(readRuntimeNumber("requireRecentlyHitSeconds"), 0.0, 600.0);
		if (requireRecentlyHitSeconds > 0.0) {
			a_out.requireRecentlyHit = std::chrono::milliseconds(static_cast<std::int64_t>(requireRecentlyHitSeconds * 1000.0));
		}

		const double requireRecentlyKillSeconds = std::clamp(readRuntimeNumber("requireRecentlyKillSeconds"), 0.0, 600.0);
		if (requireRecentlyKillSeconds > 0.0) {
			a_out.requireRecentlyKill = std::chrono::milliseconds(static_cast<std::int64_t>(requireRecentlyKillSeconds * 1000.0));
		}

		const double requireNotHitRecentlySeconds = std::clamp(readRuntimeNumber("requireNotHitRecentlySeconds"), 0.0, 600.0);
		if (requireNotHitRecentlySeconds > 0.0) {
			a_out.requireNotHitRecently = std::chrono::milliseconds(static_cast<std::int64_t>(requireNotHitRecentlySeconds * 1000.0));
		}

		const float lowHealthThresholdPct = static_cast<float>(std::clamp(readRuntimeNumber("lowHealthThresholdPercent", 35.0), 1.0, 95.0));
		const float lowHealthRearmPct = static_cast<float>(std::clamp(
			readRuntimeNumber("lowHealthRearmPercent", static_cast<double>(lowHealthThresholdPct + 10.0f)),
			static_cast<double>(lowHealthThresholdPct + 1.0f),
			100.0));
		a_out.lowHealthThresholdPct = lowHealthThresholdPct;
		a_out.lowHealthRearmPct = lowHealthRearmPct;

		a_out.luckyHitChancePct = static_cast<float>(std::clamp(readRuntimeNumber("luckyHitChancePercent"), 0.0, 100.0));
		a_out.luckyHitProcCoefficient = static_cast<float>(std::clamp(readRuntimeNumber("luckyHitProcCoefficient", 1.0), 0.0, 5.0));

		return true;
	}
}
