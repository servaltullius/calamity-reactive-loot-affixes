#pragma once

#include "RE/I/IMenu.h"
#include "RE/M/MenuEventHandler.h"
#include "RE/N/NiColor.h"
#include "RE/S/SimpleAnimationGraphManagerHolder.h"

namespace RE
{
	// menuDepth = 8
	// flags = kDisablePauseMenu | kAllowSaving | kDontHideCursorWhenTopmost
	// context = kNone
	class MistMenu :
#ifndef SKYRIM_CROSS_VR
		public IMenu,                              // 00
		public SimpleAnimationGraphManagerHolder,  // 30, 40
		public MenuEventHandler                    // 48, 58
#else
		public IMenu  // 00
#endif
	{
	public:
		inline static constexpr auto      RTTI = RTTI_MistMenu;
		constexpr static std::string_view MENU_NAME = "Mist Menu";

		struct Colors
		{
			enum Color
			{
				kXNeg,
				kXPos,
				kYNeg,
				kYPos,
				kZNeg,
				kZPos,

				kTotal
			};
		};

		struct RUNTIME_DATA
		{
#define RUNTIME_DATA_CONTENT                                           \
	NiColor       ambientColors[Colors::kTotal]; /* 000 */             \
	std::uint32_t unk0A0;                        /* 048 */             \
	std::uint32_t unk0A4;                        /* 04C */             \
	std::uint64_t unk0A8;                        /* 050 */             \
	void*         unk0B0;                        /* 058 - smart ptr */ \
	void*         unk0B8;                        /* 060 */             \
	void*         unk0C0;                        /* 068 */             \
	void*         unk0C8;                        /* 070 - smart ptr */ \
	void*         unk0D0;                        /* 078 - smart ptr */ \
	void*         unk0D8;                        /* 080 - smart ptr */ \
	void*         unk0E0;                        /* 088 - smart ptr */ \
	std::uint64_t unk0E8;                        /* 090 */             \
	std::uint64_t unk0F0;                        /* 098 */             \
	std::uint64_t unk0F8;                        /* 0A0 */             \
	std::uint32_t unk100;                        /* 0A8 */             \
	std::uint32_t unk104;                        /* 0AC */             \
	std::uint32_t unk108;                        /* 0B0 */             \
	std::uint32_t unk10C;                        /* 0B4 */             \
	float         unk110;                        /* 0B8 */             \
	std::uint32_t unk114;                        /* 0BC */             \
	std::uint64_t unk118;                        /* 0C0 */             \
	float         unk120;                        /* 0C8 */             \
	std::uint32_t unk124;                        /* 0CC */             \
	std::uint64_t unk128;                        /* 0D0 */             \
	float         unk130;                        /* 0D8 */             \
	std::uint8_t  unk134;                        /* 0DC */             \
	std::uint8_t  unk135;                        /* 0DD */             \
	std::uint16_t unk136;                        /* 0DE */             \
	std::uint32_t unk138;                        /* 0E0 */             \
	std::uint32_t pad13C;                        /* 0E4 */

			RUNTIME_DATA_CONTENT
		};
		static_assert(sizeof(RUNTIME_DATA) == 0xE8);

		~MistMenu() override;  // 00

		// override (IMenu)
		UI_MESSAGE_RESULTS ProcessMessage(UIMessage& a_message) override;                         // 04
		void               AdvanceMovie(float a_interval, std::uint32_t a_currentTime) override;  // 05
		void               PostDisplay() override;                                                // 06

		// override (MenuEventHandler)
#ifndef SKYRIM_CROSS_VR
		bool CanProcess(InputEvent* a_event) override;              // 01
		bool ProcessThumbstick(ThumbstickEvent* a_event) override;  // 03
		bool ProcessMouseMove(MouseMoveEvent* a_event) override;    // 04
		bool ProcessButton(ButtonEvent* a_event) override;          // 05
#endif

		[[nodiscard]] SimpleAnimationGraphManagerHolder* AsSimpleAnimationGraphManagerHolder() noexcept
		{
			return &REL::RelocateMember<SimpleAnimationGraphManagerHolder>(this, 0x30, 0x40);
		}

		[[nodiscard]] const SimpleAnimationGraphManagerHolder* AsSimpleAnimationGraphManagerHolder() const noexcept
		{
			return const_cast<MistMenu*>(this)->AsSimpleAnimationGraphManagerHolder();
		}

		[[nodiscard]] MenuEventHandler* AsMenuEventHandler() noexcept
		{
			return &REL::RelocateMember<MenuEventHandler>(this, 0x48, 0x58);
		}

		[[nodiscard]] const MenuEventHandler* AsMenuEventHandler() const noexcept
		{
			return const_cast<MistMenu*>(this)->AsMenuEventHandler();
		}

		[[nodiscard]] inline RUNTIME_DATA& GetRuntimeData() noexcept
		{
			return REL::RelocateMember<RUNTIME_DATA>(this, 0x58, 0x68);
		}

		[[nodiscard]] inline const RUNTIME_DATA& GetRuntimeData() const noexcept
		{
			return REL::RelocateMember<RUNTIME_DATA>(this, 0x58, 0x68);
		}

		// members
#ifndef SKYRIM_CROSS_VR
		RUNTIME_DATA_CONTENT  // 58, 68
#endif
	};
#ifndef ENABLE_SKYRIM_VR
	static_assert(sizeof(MistMenu) == 0x140);
#elif !defined(ENABLE_SKYRIM_AE) && !defined(ENABLE_SKYRIM_SE)
	static_assert(sizeof(MistMenu) == 0x150);
#endif
}
#undef RUNTIME_DATA_CONTENT
