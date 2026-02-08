Scriptname CalamityAffixes_SkseBridge extends Quest

; Receives SKSE ModCallbackEvent(s) and forwards them to the central Papyrus manager.
;
; Notes:
; - ModEvent registrations are not persisted across loads; re-register after each game load.
; - Keep these callbacks lightweight (the heavy logic should live in the manager with ICD/guards).

CalamityAffixes_AffixManager Property Manager Auto Const

Bool _registered = false

Event OnInit()
	EnsureRegistered()
EndEvent

Event OnPlayerLoadGame()
	_registered = false
	EnsureRegistered()
EndEvent

Function EnsureRegistered()
	if _registered
		return
	endif

	_registered = true

	; C++ plugin emits these events (see skse/CalamityAffixes/src/EventBridge.cpp)
	RegisterForModEvent("CalamityAffixes_Hit", "OnCalamityHit")
	RegisterForModEvent("CalamityAffixes_DotApply", "OnCalamityDotApply")
EndFunction

Event OnCalamityHit(string eventName, string strArg, float numArg, Form sender)
	if !Manager
		return
	endif

	Actor target = sender as Actor
	if !target
		return
	endif

	; Outgoing hit-like trigger (any weapon/spell/projectile, including player-owned summons).
	Manager.ProcessOutgoingHit(target)
EndEvent

Event OnCalamityDotApply(string eventName, string strArg, float numArg, Form sender)
	if !Manager
		return
	endif

	Actor target = sender as Actor
	if !target
		return
	endif

	; PoE-style DoT trigger (apply/refresh only; rate-limited in C++).
	; For now, we feed it into the same outgoing pipeline. If you later want distinct triggers,
	; extend the manager/effect interfaces and route this event separately.
	Manager.ProcessOutgoingHit(target)
EndEvent

