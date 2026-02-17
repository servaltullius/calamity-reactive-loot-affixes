Scriptname CalamityAffixes_MCMConfig extends MCM_ConfigBase

; Minimal MCM Helper config script.
; Bind this script to a Quest in the Creation Kit (see docs / README).
; General actions and runeword/manual controls are forwarded to SKSE via ModEvent.

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
