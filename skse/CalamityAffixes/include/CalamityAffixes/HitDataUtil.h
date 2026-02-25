#pragma once

#include "CalamityAffixes/PointerSafety.h"

#include <RE/Skyrim.h>

namespace CalamityAffixes::HitDataUtil
{
	[[nodiscard]] inline const RE::HitData* GetLastHitData(RE::Actor* a_target)
	{
		if (!a_target) {
			return nullptr;
		}

		const auto& runtime = a_target->GetActorRuntimeData();
		auto* process = SanitizeObjectPointer(runtime.currentProcess);
		if (!process) {
			return nullptr;
		}

		auto* middleHigh = SanitizeObjectPointer(process->middleHigh);
		if (!middleHigh) {
			return nullptr;
		}

		return SanitizeObjectPointer(middleHigh->lastHitData);
	}

	[[nodiscard]] inline RE::FormID GetHitSourceFormID(const RE::HitData* a_hitData) noexcept
	{
		if (!a_hitData) {
			return 0;
		}
		if (a_hitData->weapon) {
			return a_hitData->weapon->GetFormID();
		}
		if (a_hitData->attackDataSpell) {
			return a_hitData->attackDataSpell->GetFormID();
		}
		return 0;
	}
}
