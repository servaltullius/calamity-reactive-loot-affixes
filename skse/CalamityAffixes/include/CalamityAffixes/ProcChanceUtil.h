#pragma once

#include <mutex>
#include <random>

namespace CalamityAffixes
{
	inline bool RollProcChance(std::mt19937& a_rng, std::mutex& a_rngMutex, float a_chancePct)
	{
		if (a_chancePct >= 100.0f) {
			return true;
		}
		if (a_chancePct <= 0.0f) {
			return false;
		}

		std::lock_guard<std::mutex> lock(a_rngMutex);
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);
		return dist(a_rng) < a_chancePct;
	}
}
