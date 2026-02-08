#pragma once

#include "RE/M/MapMenu.h"

namespace RE
{
	class FastTravelConfirmCallback
	{
	public:
		inline static constexpr auto RTTI = RTTI___FastTravelConfirmCallback;
		inline static constexpr auto VTABLE = VTABLE___FastTravelConfirmCallback;

		virtual ~FastTravelConfirmCallback();  // 00

		//add
		virtual bool Run(bool a_arg1);  //01

		//members
		std::uint64_t refCount;  // 08
		MapMenu*      mapMenu;   // 10
		std::uint32_t unk18;     // 18
		std::uint32_t unk1C;     // 1C
	};
	static_assert(sizeof(FastTravelConfirmCallback) == 0x20);
}
