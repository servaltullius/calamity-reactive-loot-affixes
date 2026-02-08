#pragma once

#include <RE/Skyrim.h>

namespace CalamityAffixes::HitDataUtil
{
	[[nodiscard]] inline const RE::HitData* GetLastHitData(RE::Actor* a_target)
	{
		if (!a_target) {
			return nullptr;
		}

		const auto& runtime = a_target->GetActorRuntimeData();
		auto* process = runtime.currentProcess;
		if (!process || !process->middleHigh) {
			return nullptr;
		}

		return process->middleHigh->lastHitData;
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

