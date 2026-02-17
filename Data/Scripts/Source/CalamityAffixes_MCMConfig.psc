Scriptname CalamityAffixes_MCMConfig extends MCM_ConfigBase

; Minimal MCM Helper config script.
; Bind this script to a Quest in the Creation Kit (see docs / README).
; General actions and runeword/manual controls are forwarded to SKSE via ModEvent.
;
; Only the canonical quest instance that still exists in the currently loaded plugin should register.
; This filters stale quest instances left in saves after generated FormID churn.
int Property LocalFormIdModulo = 16777216 AutoReadOnly Hidden ; 0x01000000
string Property PluginFileName = "CalamityAffixes.esp" AutoReadOnly Hidden
string Property LeaderTokenSettingName = "iMcmLeaderToken:General" AutoReadOnly Hidden

bool _didLeaderElection = false
bool _isSessionLeader = false

bool Function IsCanonicalMcmQuest()
	int localFormId = GetFormID()
	; Papyrus int is signed. ESL/FE load-order FormIDs can be negative here,
	; so normalize first before taking local (lower 24-bit) id.
	while localFormId < 0
		localFormId += LocalFormIdModulo
	endWhile
	localFormId = localFormId % LocalFormIdModulo

	Form resolved = Game.GetFormFromFile(localFormId, PluginFileName)
	Quest resolvedQuest = resolved as Quest
	Quest selfQuest = self as Quest
	if resolvedQuest == None || selfQuest == None
		return false
	endif

	return (resolvedQuest == selfQuest)
EndFunction

bool Function EnsureSessionLeader()
	if _didLeaderElection
		return _isSessionLeader
	endif

	_didLeaderElection = true

	int token = Utility.RandomInt(1, 2147483646)
	SetModSettingInt(LeaderTokenSettingName, token)
	Utility.WaitMenuMode(0.05)

	int chosen = GetModSettingInt(LeaderTokenSettingName)
	_isSessionLeader = (chosen == token)
	return _isSessionLeader
EndFunction

Event OnGameReload()
	; Leave stale quest instances inert so only canonical MCM quest participates.
	if !IsCanonicalMcmQuest()
		return
	endif

	if !EnsureSessionLeader()
		return
	endif

	Parent.OnGameReload()
EndEvent

Event OnConfigManagerReady(string a_eventName, string a_strArg, float a_numArg, Form a_sender)
	; Hard gate registration path: only canonical quest instance can reach parent registration logic.
	if !IsCanonicalMcmQuest()
		return
	endif

	if !EnsureSessionLeader()
		return
	endif

	Parent.OnConfigManagerReady(a_eventName, a_strArg, a_numArg, a_sender)
EndEvent

Event OnConfigManagerReset(string a_eventName, string a_strArg, float a_numArg, Form a_sender)
	if !IsCanonicalMcmQuest()
		return
	endif

	bool wasLeader = _isSessionLeader
	_didLeaderElection = false
	_isSessionLeader = false
	if !wasLeader
		return
	endif

	Parent.OnConfigManagerReset(a_eventName, a_strArg, a_numArg, a_sender)
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
