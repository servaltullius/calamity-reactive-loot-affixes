#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cstdint>

namespace CalamityAffixes
{
	const InstanceAffixSlots* EventBridge::FindLootPreviewSlots(std::uint64_t a_instanceKey) const
	{
		if (a_instanceKey == 0u) {
			return nullptr;
		}

		const auto it = _lootPreviewAffixes.find(a_instanceKey);
		if (it == _lootPreviewAffixes.end()) {
			return nullptr;
		}

		return std::addressof(it->second);
	}

	void EventBridge::RememberLootPreviewSlots(std::uint64_t a_instanceKey, const InstanceAffixSlots& a_slots)
	{
		if (a_instanceKey == 0u) {
			return;
		}

		const auto [_, inserted] = _lootPreviewAffixes.insert_or_assign(a_instanceKey, a_slots);
		if (!inserted) {
			return;
		}

		_lootPreviewRecent.push_back(a_instanceKey);
		while (_lootPreviewAffixes.size() > kLootPreviewCacheMax && !_lootPreviewRecent.empty()) {
			const auto oldest = _lootPreviewRecent.front();
			_lootPreviewRecent.pop_front();
			if (oldest != 0u) {
				_lootPreviewAffixes.erase(oldest);
			}
		}
	}

	void EventBridge::ForgetLootPreviewSlots(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0u) {
			return;
		}
		_lootPreviewAffixes.erase(a_instanceKey);
		std::erase(_lootPreviewRecent, a_instanceKey);
	}
}
