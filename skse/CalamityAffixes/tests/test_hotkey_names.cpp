#include "CalamityAffixes/HotkeyNames.h"

using namespace CalamityAffixes::HotkeyNames;

static_assert(KeyboardKeyName(0x57) == "F11", "the default panel key");
static_assert(KeyboardKeyName(0xD2) == "Insert" && KeyboardKeyName(0xC7) == "Home" && KeyboardKeyName(0xD3) == "Delete",
	"navigation keys have names, not Key210");
static_assert(KeyboardKeyName(0x29) == "`" && KeyboardKeyName(0x27) == ";" && KeyboardKeyName(0x2B) == "\\");
static_assert(KeyboardKeyName(0x4F) == "Num1" && KeyboardKeyName(0xB5) == "Num/");
static_assert(KeyboardKeyName(0x64) == "F13");
static_assert(KeyboardKeyName(0x00).empty() && KeyboardKeyName(0xFE).empty(), "unknown codes have no name");

static_assert(ClassifyKeyCode(0u) == KeyDevice::kNone);
static_assert(ClassifyKeyCode(0x57u) == KeyDevice::kKeyboard);
static_assert(ClassifyKeyCode(256u) == KeyDevice::kMouse && ClassifyKeyCode(265u) == KeyDevice::kMouse);
static_assert(ClassifyKeyCode(266u) == KeyDevice::kGamepad, "MCM Helper gamepad codes are not mouse buttons");

static_assert(MouseKeyName(256u) == "Mouse1" && MouseKeyName(263u) == "Mouse8");
static_assert(MouseKeyName(264u) == "WheelUp" && MouseKeyName(265u) == "WheelDown");
static_assert(MouseKeyName(266u).empty(), "a gamepad code is never named as a mouse button");

static_assert(IsCharacterTypingKey(0x1Eu) && IsCharacterTypingKey(0x10u) && IsCharacterTypingKey(0x32u), "letters type");
static_assert(IsCharacterTypingKey(0x02u) && IsCharacterTypingKey(0x0Bu), "digit row types");
static_assert(IsCharacterTypingKey(0x29u) && IsCharacterTypingKey(0x27u) && IsCharacterTypingKey(0x2Bu) &&
	IsCharacterTypingKey(0x35u) && IsCharacterTypingKey(0x0Cu), "punctuation types");
static_assert(IsCharacterTypingKey(0x39u), "space types");
static_assert(IsCharacterTypingKey(0x52u) && IsCharacterTypingKey(0x47u) && IsCharacterTypingKey(0x4Fu), "numpad digits type");
static_assert(!IsCharacterTypingKey(0x57u) && !IsCharacterTypingKey(0x3Bu) && !IsCharacterTypingKey(0x44u) &&
	!IsCharacterTypingKey(0x64u), "F-keys always toggle");
static_assert(!IsCharacterTypingKey(0xD2u) && !IsCharacterTypingKey(0xC7u) && !IsCharacterTypingKey(0xD3u) &&
	!IsCharacterTypingKey(0xC9u) && !IsCharacterTypingKey(0xD1u) && !IsCharacterTypingKey(0xCFu),
	"Insert/Home/Delete/PgUp/PgDn/End always toggle");
static_assert(!IsCharacterTypingKey(0xC8u) && !IsCharacterTypingKey(0xCBu) && !IsCharacterTypingKey(0xCDu) &&
	!IsCharacterTypingKey(0xD0u), "arrows always toggle");
static_assert(!IsCharacterTypingKey(0x01u) && !IsCharacterTypingKey(0x0Fu) && !IsCharacterTypingKey(0x1Cu) &&
	!IsCharacterTypingKey(0x1Du) && !IsCharacterTypingKey(0x2Au) && !IsCharacterTypingKey(0x38u) &&
	!IsCharacterTypingKey(0x45u) && !IsCharacterTypingKey(0x9Cu), "Esc/Tab/Enter/modifiers/NumLock do not type");
static_assert(!IsCharacterTypingKey(0u) && !IsCharacterTypingKey(256u) && !IsCharacterTypingKey(266u),
	"mouse and gamepad codes never type");
