Scriptname CalamityAffixes_ModEventEmitter Hidden

; Minimal native bridge to emit SKSE ModCallbackEvent-style mod events from Papyrus.
; This avoids hard dependency on SKSE script sources for compilation (WSL CI/tooling).

Function Send(string a_eventName, string a_strArg = "", float a_numArg = 0.0) global native

