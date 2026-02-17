Scriptname SKI_ConfigBase extends SKI_QuestBase hidden

string Property ModName Auto
string[] Property Pages Auto

Event OnGameReload()
EndEvent

Event OnConfigManagerReady(string a_eventName, string a_strArg, float a_numArg, Form a_sender)
EndEvent

Event OnConfigManagerReset(string a_eventName, string a_strArg, float a_numArg, Form a_sender)
EndEvent

Event OnConfigInit()
EndEvent

Event OnConfigOpen()
EndEvent

Event OnConfigClose()
EndEvent
