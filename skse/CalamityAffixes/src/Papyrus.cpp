#include "CalamityAffixes/Papyrus.h"

#include <string>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace CalamityAffixes::Papyrus
{
	namespace
	{
		void SendModCallbackEvent(RE::StaticFunctionTag*, std::string a_eventName, std::string a_strArg, float a_numArg)
		{
			if (a_eventName.empty()) {
				return;
			}

			auto* source = SKSE::GetModCallbackEventSource();
			if (!source) {
				return;
			}

			const std::string eventName = std::move(a_eventName);
			const std::string strArg = std::move(a_strArg);

			SKSE::ModCallbackEvent event{
				RE::BSFixedString(eventName.c_str()),
				RE::BSFixedString(strArg.c_str()),
				a_numArg,
				nullptr
			};
			source->SendEvent(&event);
		}
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		if (!a_vm) {
			return false;
		}

		a_vm->RegisterFunction("Send", "CalamityAffixes_ModEventEmitter", SendModCallbackEvent);
		return true;
	}
}

