Scriptname CalamityAffixes_MCMConfig extends MCM_ConfigBase

; Minimal MCM Helper config script.
; Bind this script to a Quest in the Creation Kit (see docs / README).
; General actions and runeword/manual controls are forwarded to SKSE via ModEvent.
;
; Only the generated canonical MCM quest (FormID 0x000800) should register.
; This filters out stale quest instances left in old saves and prevents duplicate MCM entries.
int Property ExpectedMcmQuestFormId = 0x000800 AutoReadOnly Hidden
int Property LocalFormIdModulo = 16777216 AutoReadOnly Hidden ; 0x01000000
bool _isCanonicalMcmQuest = false

Event OnConfigInit()
	int localFormId = GetFormID()
	; Papyrus int is signed. ESL/FE load-order FormIDs can be negative here,
	; so normalize first before taking local (lower 24-bit) id.
	while localFormId < 0
		localFormId += LocalFormIdModulo
	endWhile
	localFormId = localFormId % LocalFormIdModulo
	_isCanonicalMcmQuest = (localFormId == ExpectedMcmQuestFormId)
EndEvent

Event OnConfigManagerReady(string a_eventName, string a_strArg, float a_numArg, Form a_sender)
	SKI_ConfigManager manager = a_sender as SKI_ConfigManager
	SKI_ConfigBase selfConfig = self as SKI_ConfigBase
	if manager == None || selfConfig == None
		return
	endif

	if !_isCanonicalMcmQuest
		manager.UnregisterMod(selfConfig)
		return
	endif

	if ModName == ""
		return
	endif

	; Normalize any stale slot for this instance before re-registering.
	manager.UnregisterMod(selfConfig)
	manager.RegisterMod(selfConfig, ModName)
EndEvent

Event OnConfigManagerReset(string a_eventName, string a_strArg, float a_numArg, Form a_sender)
	; No-op. Canonical/non-canonical registration is resolved in OnConfigManagerReady.
EndEvent

Function SetEnabled(bool a_enabled)
	CalamityAffixes_ModeControl.SetEnabled(a_enabled)
EndFunction

Function SetDebugNotificationsEnabled(bool a_enabled)
	CalamityAffixes_ModeControl.SetDebugNotificationsEnabled(a_enabled)
EndFunction

Function SetValidationIntervalSeconds(float a_seconds)
	CalamityAffixes_ModeControl.SetValidationIntervalSeconds(a_seconds)
EndFunction

Function SetProcChanceMultiplier(float a_mult)
	CalamityAffixes_ModeControl.SetProcChanceMultiplier(a_mult)
EndFunction

Function SetLootChancePercent(float a_percent)
	CalamityAffixes_ModeControl.SetLootChancePercent(a_percent)
EndFunction

Function SetDotSafetyAutoDisable(bool a_enabled)
	CalamityAffixes_ModeControl.SetDotSafetyAutoDisable(a_enabled)
EndFunction

Function SetAllowNonHostileFirstHitProc(bool a_enabled)
	CalamityAffixes_ModeControl.SetAllowNonHostileFirstHitProc(a_enabled)
EndFunction

Function SetPrismaControlPanel(bool a_open)
	CalamityAffixes_ModeControl.SetPrismaControlPanel(a_open)
EndFunction

Function SpawnTestItem()
	CalamityAffixes_ModeControl.SpawnTestItem()
EndFunction

Function CycleModeNext()
	CalamityAffixes_ModeControl.CycleModeNext()
EndFunction

Function CycleModePrev()
	CalamityAffixes_ModeControl.CycleModePrev()
EndFunction

Function RunewordBaseNext()
	CalamityAffixes_ModeControl.RunewordBaseNext()
EndFunction

Function RunewordBasePrev()
	CalamityAffixes_ModeControl.RunewordBasePrev()
EndFunction

Function RunewordRecipeNext()
	CalamityAffixes_ModeControl.RunewordRecipeNext()
EndFunction

Function RunewordRecipePrev()
	CalamityAffixes_ModeControl.RunewordRecipePrev()
EndFunction

Function RunewordInsertRune()
	CalamityAffixes_ModeControl.RunewordInsertRune()
EndFunction

Function RunewordStatus()
	CalamityAffixes_ModeControl.RunewordStatus()
EndFunction

Function RunewordGrantNextRune()
	CalamityAffixes_ModeControl.RunewordGrantNextRune()
EndFunction

Function RunewordGrantRecipeSet()
	CalamityAffixes_ModeControl.RunewordGrantRecipeSet()
EndFunction
