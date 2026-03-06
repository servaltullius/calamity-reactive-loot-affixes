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

		const auto it = _lootState.previewAffixes.find(a_instanceKey);
		if (it == _lootState.previewAffixes.end()) {
			return nullptr;
		}

		return std::addressof(it->second);
	}

	void EventBridge::RememberLootPreviewSlots(std::uint64_t a_instanceKey, const InstanceAffixSlots& a_slots)
	{
		if (a_instanceKey == 0u) {
			return;
		}

		const auto [_, inserted] = _lootState.previewAffixes.insert_or_assign(a_instanceKey, a_slots);
		if (!inserted) {
			return;
		}

		_lootState.previewRecent.push_back(a_instanceKey);
		while (_lootState.previewAffixes.size() > kLootPreviewCacheMax && !_lootState.previewRecent.empty()) {
			const auto oldest = _lootState.previewRecent.front();
			_lootState.previewRecent.pop_front();
			if (oldest != 0u) {
				_lootState.previewAffixes.erase(oldest);
			}
		}
	}

	void EventBridge::ForgetLootPreviewSlots(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0u) {
			return;
		}
		_lootState.previewAffixes.erase(a_instanceKey);
		std::erase(_lootState.previewRecent, a_instanceKey);
	}
}
