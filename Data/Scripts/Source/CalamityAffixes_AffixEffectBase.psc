Scriptname CalamityAffixes_AffixEffectBase extends Quest

; Template “affix effect” definition.
; In a real mod you may represent effects purely as spells/MGEFs and skip scripts entirely.

Keyword Property AffixKeyword Auto Const

Float Property ProcChancePct = 10.0 Auto Const
Float Property ICDSeconds = 2.0 Auto Const

Bool Property TriggerOnOutgoingHit = true Auto Const
Bool Property TriggerOnIncomingHit = false Auto Const

Spell Property ProcSpell Auto Const

Bool Function CanTrigger(bool isOutgoing)
	if isOutgoing
		return TriggerOnOutgoingHit
	endif

	return TriggerOnIncomingHit
EndFunction

Function OnProcOutgoing(Actor akPlayer, Actor akTarget, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
	if ProcSpell && akPlayer && akTarget
		ProcSpell.Cast(akPlayer, akTarget)
	endif
EndFunction

Function OnProcIncoming(Actor akPlayer, Actor akAttacker, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
	; Override in derived scripts if needed.
EndFunction
