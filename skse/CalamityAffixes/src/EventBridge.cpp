#include "CalamityAffixes/EventBridge.h"

#include <memory>

namespace CalamityAffixes
{
	EventBridge* EventBridge::GetSingleton()
	{
		static EventBridge singleton;
		return std::addressof(singleton);
	}

	void EventBridge::Register()
	{
		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		holder->AddEventSink<RE::TESHitEvent>(this);
		holder->AddEventSink<RE::TESDeathEvent>(this);
		holder->AddEventSink<RE::TESEquipEvent>(this);
		holder->AddEventSink<RE::TESActivateEvent>(this);
		holder->AddEventSink<RE::TESMagicEffectApplyEvent>(this);
		holder->AddEventSink<RE::TESContainerChangedEvent>(this);
		holder->AddEventSink<RE::TESUniqueIDChangeEvent>(this);

		if (auto* modCallbackSource = SKSE::GetModCallbackEventSource()) {
			modCallbackSource->AddEventSink(this);
		}
	}
}
