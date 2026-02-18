Scriptname CalamityAffixes_ModeControl Hidden

; Global helpers for manual mode-cycle switching.
; Called from MCM keybinds / UI actions.

Function Emit(string a_eventName, string a_strArg = "", float a_numArg = 0.0) global
	if a_eventName == ""
		return
	endif

	; C++ plugin listens via SKSE::GetModCallbackEventSource().
	; We call a tiny native wrapper to avoid depending on SKSE script sources at compile time.
	CalamityAffixes_ModEventEmitter.Send(a_eventName, a_strArg, a_numArg)
EndFunction

Function SetEnabled(bool a_enabled) global
	if a_enabled
		Emit("CalamityAffixes_MCM_SetEnabled", "", 1.0)
	else
		Emit("CalamityAffixes_MCM_SetEnabled", "", 0.0)
	endif
EndFunction

Function SetDebugNotificationsEnabled(bool a_enabled) global
	if a_enabled
		Emit("CalamityAffixes_MCM_SetDebugNotifications", "", 1.0)
	else
		Emit("CalamityAffixes_MCM_SetDebugNotifications", "", 0.0)
	endif
EndFunction

Function SetValidationIntervalSeconds(float a_seconds) global
	Emit("CalamityAffixes_MCM_SetValidationInterval", "", a_seconds)
EndFunction

Function SetProcChanceMultiplier(float a_mult) global
	Emit("CalamityAffixes_MCM_SetProcChanceMult", "", a_mult)
EndFunction

Function SetRunewordFragmentChancePercent(float a_percent) global
	Emit("CalamityAffixes_MCM_SetRunewordFragmentChance", "", a_percent)
EndFunction

Function SetReforgeOrbChancePercent(float a_percent) global
	Emit("CalamityAffixes_MCM_SetReforgeOrbChance", "", a_percent)
EndFunction

Function SetLootChancePercent(float a_percent) global
	; Legacy no-op (pickup affix auto-assignment removed by design).
	; Kept for backward compatibility with older MCM configs that may still call this function.
EndFunction

Function SetDotSafetyAutoDisable(bool a_enabled) global
	if a_enabled
		Emit("CalamityAffixes_MCM_SetDotSafetyAutoDisable", "", 1.0)
	else
		Emit("CalamityAffixes_MCM_SetDotSafetyAutoDisable", "", 0.0)
	endif
EndFunction

Function SetAllowNonHostileFirstHitProc(bool a_enabled) global
	if a_enabled
		Emit("CalamityAffixes_MCM_SetAllowNonHostileFirstHitProc", "", 1.0)
	else
		Emit("CalamityAffixes_MCM_SetAllowNonHostileFirstHitProc", "", 0.0)
	endif
EndFunction

Function SpawnTestItem() global
	Emit("CalamityAffixes_MCM_SpawnTestItem", "", 1.0)
EndFunction

Function CycleModeNext() global
	Emit("CalamityAffixes_ModeCycle_Next", "", 0.0)
EndFunction

Function CycleModePrev() global
	Emit("CalamityAffixes_ModeCycle_Prev", "", 0.0)
EndFunction

Function RunewordBaseNext() global
	Emit("CalamityAffixes_Runeword_Base_Next", "", 0.0)
EndFunction

Function RunewordBasePrev() global
	Emit("CalamityAffixes_Runeword_Base_Prev", "", 0.0)
EndFunction

Function RunewordRecipeNext() global
	Emit("CalamityAffixes_Runeword_Recipe_Next", "", 0.0)
EndFunction

Function RunewordRecipePrev() global
	Emit("CalamityAffixes_Runeword_Recipe_Prev", "", 0.0)
EndFunction

Function RunewordInsertRune() global
	Emit("CalamityAffixes_Runeword_InsertRune", "", 0.0)
EndFunction

Function RunewordStatus() global
	Emit("CalamityAffixes_Runeword_Status", "", 0.0)
EndFunction

Function RunewordGrantNextRune() global
	Emit("CalamityAffixes_Runeword_GrantNextRune", "", 1.0)
EndFunction

Function RunewordGrantRecipeSet() global
	Emit("CalamityAffixes_Runeword_GrantRecipeSet", "", 1.0)
EndFunction

Function SetPrismaControlPanel(bool a_open) global
	if a_open
		Emit("CalamityAffixes_UI_SetPanel", "", 1.0)
	else
		Emit("CalamityAffixes_UI_SetPanel", "", 0.0)
	endif
EndFunction

Function TogglePrismaControlPanel() global
	Emit("CalamityAffixes_UI_TogglePanel", "", 0.0)
EndFunction

Function PanelHotkey_NoOp() global
	; Intentionally empty. The SKSE plugin reads the configured keycode from MCM settings and toggles the panel.
EndFunction
