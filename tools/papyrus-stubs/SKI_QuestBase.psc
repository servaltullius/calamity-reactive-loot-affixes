Scriptname SKI_QuestBase extends Quest hidden

int Property CurrentVersion Auto Hidden

Event OnInit()
EndEvent

Event OnGameReload()
EndEvent

Function CheckVersion()
EndFunction

int Function GetVersion()
	Return 1
EndFunction

Event OnVersionUpdate(int a_version)
EndEvent
