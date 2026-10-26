#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace CalamityAffixes::HotkeyNames
{
	// SKSE/MCM Helper key codes: 0-255 DirectInput scan codes, 256-265 mouse
	// (buttons 1-8, wheel up/down), 266-281 gamepad buttons.
	inline constexpr std::uint32_t kMouseOffset = 256u;
	inline constexpr std::uint32_t kGamepadOffset = 266u;

	enum class KeyDevice : std::uint8_t
	{
		kNone,
		kKeyboard,
		kMouse,
		kGamepad,
	};

	[[nodiscard]] constexpr KeyDevice ClassifyKeyCode(std::uint32_t a_code) noexcept
	{
		if (a_code == 0u) {
			return KeyDevice::kNone;
		}
		if (a_code >= kGamepadOffset) {
			return KeyDevice::kGamepad;
		}
		if (a_code >= kMouseOffset) {
			return KeyDevice::kMouse;
		}
		return KeyDevice::kKeyboard;
	}

	// Display name of a DirectInput scan code; empty when the code has none.
	[[nodiscard]] constexpr std::string_view KeyboardKeyName(std::uint32_t a_dik) noexcept
	{
		struct Entry
		{
			std::uint8_t code;
			std::string_view name;
		};
		constexpr std::array<Entry, 107> kNames{ {
			{ 0x01, "Esc" }, { 0x02, "1" }, { 0x03, "2" }, { 0x04, "3" }, { 0x05, "4" }, { 0x06, "5" },
			{ 0x07, "6" }, { 0x08, "7" }, { 0x09, "8" }, { 0x0A, "9" }, { 0x0B, "0" }, { 0x0C, "-" },
			{ 0x0D, "=" }, { 0x0E, "Backspace" }, { 0x0F, "Tab" }, { 0x10, "Q" }, { 0x11, "W" }, { 0x12, "E" },
			{ 0x13, "R" }, { 0x14, "T" }, { 0x15, "Y" }, { 0x16, "U" }, { 0x17, "I" }, { 0x18, "O" },
			{ 0x19, "P" }, { 0x1A, "[" }, { 0x1B, "]" }, { 0x1C, "Enter" }, { 0x1D, "LCtrl" }, { 0x1E, "A" },
			{ 0x1F, "S" }, { 0x20, "D" }, { 0x21, "F" }, { 0x22, "G" }, { 0x23, "H" }, { 0x24, "J" },
			{ 0x25, "K" }, { 0x26, "L" }, { 0x27, ";" }, { 0x28, "'" }, { 0x29, "`" }, { 0x2A, "LShift" },
			{ 0x2B, "\\" }, { 0x2C, "Z" }, { 0x2D, "X" }, { 0x2E, "C" }, { 0x2F, "V" }, { 0x30, "B" },
			{ 0x31, "N" }, { 0x32, "M" }, { 0x33, "," }, { 0x34, "." }, { 0x35, "/" }, { 0x36, "RShift" },
			{ 0x37, "Num*" }, { 0x38, "LAlt" }, { 0x39, "Space" }, { 0x3A, "CapsLock" }, { 0x3B, "F1" }, { 0x3C, "F2" },
			{ 0x3D, "F3" }, { 0x3E, "F4" }, { 0x3F, "F5" }, { 0x40, "F6" }, { 0x41, "F7" }, { 0x42, "F8" },
			{ 0x43, "F9" }, { 0x44, "F10" }, { 0x45, "NumLock" }, { 0x46, "ScrollLock" }, { 0x47, "Num7" }, { 0x48, "Num8" },
			{ 0x49, "Num9" }, { 0x4A, "Num-" }, { 0x4B, "Num4" }, { 0x4C, "Num5" }, { 0x4D, "Num6" }, { 0x4E, "Num+" },
			{ 0x4F, "Num1" }, { 0x50, "Num2" }, { 0x51, "Num3" }, { 0x52, "Num0" }, { 0x53, "Num." }, { 0x57, "F11" },
			{ 0x58, "F12" }, { 0x64, "F13" }, { 0x65, "F14" }, { 0x66, "F15" }, { 0x9C, "NumEnter" }, { 0x9D, "RCtrl" },
			{ 0xB5, "Num/" }, { 0xB7, "PrtSc" }, { 0xB8, "RAlt" }, { 0xC5, "Pause" }, { 0xC7, "Home" }, { 0xC8, "Up" },
			{ 0xC9, "PgUp" }, { 0xCB, "Left" }, { 0xCD, "Right" }, { 0xCF, "End" }, { 0xD0, "Down" }, { 0xD1, "PgDn" },
			{ 0xD2, "Insert" }, { 0xD3, "Delete" }, { 0xDB, "LWin" }, { 0xDC, "RWin" }, { 0xDD, "Apps" },
		} };
		for (const auto& entry : kNames) {
			if (entry.code == a_dik) {
				return entry.name;
			}
		}
		return {};
	}

	[[nodiscard]] constexpr std::string_view MouseKeyName(std::uint32_t a_code) noexcept
	{
		constexpr std::array<std::string_view, 10> kNames{
			"Mouse1", "Mouse2", "Mouse3", "Mouse4", "Mouse5", "Mouse6", "Mouse7", "Mouse8", "WheelUp", "WheelDown",
		};
		if (a_code < kMouseOffset || a_code >= kGamepadOffset) {
			return {};
		}
		return kNames[a_code - kMouseOffset];
	}

	// True when the DirectInput key types a character into a text field:
	// letters, the digit row, punctuation, space and the numpad digits and
	// operators. The panel hotkey yields to text entry only for these keys;
	// F-keys, navigation keys (Insert/Home/Delete/PgUp/PgDn/End, arrows),
	// modifiers, Esc/Tab/Enter/Backspace and mouse buttons always toggle.
	[[nodiscard]] constexpr bool IsCharacterTypingKey(std::uint32_t a_code) noexcept
	{
		if (ClassifyKeyCode(a_code) != KeyDevice::kKeyboard) {
			return false;
		}
		return (a_code >= 0x02u && a_code <= 0x0Du) ||  // 1-0 - =
		       (a_code >= 0x10u && a_code <= 0x1Bu) ||  // Q-P [ ]
		       (a_code >= 0x1Eu && a_code <= 0x29u) ||  // A-L ; ' `
		       (a_code >= 0x2Bu && a_code <= 0x35u) ||  // \ Z-M , . /
		       a_code == 0x37u ||                       // Num*
		       a_code == 0x39u ||                       // Space
		       (a_code >= 0x47u && a_code <= 0x53u) ||  // Num7-Num. (digits, - +)
		       a_code == 0xB5u;                         // Num/
	}

	// Gamepad buttons cannot drive the panel hotkey (documented limitation).
	inline constexpr std::string_view kGamepadUnsupportedName = "Gamepad (not supported)";
}
