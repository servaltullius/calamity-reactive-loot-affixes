Scriptname MCM_ConfigBase extends SKI_ConfigBase hidden

Event OnSettingChange(string a_ID)
EndEvent

Event OnPageSelect(string a_page)
EndEvent

Event OnGameReload()
EndEvent

Event OnConfigInit()
EndEvent

Event OnConfigOpen()
EndEvent

Event OnConfigClose()
EndEvent

int Function GetModSettingInt(string a_settingName)
	Return 0
EndFunction

Function SetModSettingInt(string a_settingName, int a_value)
EndFunction
