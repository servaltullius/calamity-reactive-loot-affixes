Scriptname CalamityAffixes_PlayerAlias extends ReferenceAlias

CalamityAffixes_AffixManager Property Manager Auto Const

Event OnPlayerLoadGame()
	if Manager
		Manager.Initialize()
		Manager.ForceResync()
	endif
EndEvent

Event OnObjectEquipped(Form akBaseObject, ObjectReference akReference)
	if Manager
		Manager.HandleObjectEquipped(akBaseObject, akReference)
	endif
EndEvent

Event OnObjectUnequipped(Form akBaseObject, ObjectReference akReference)
	if Manager
		Manager.HandleObjectUnequipped(akBaseObject, akReference)
	endif
EndEvent

Event OnHit(ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
	if Manager
		Manager.ProcessIncomingHit(akAggressor, akSource, akProjectile, abPowerAttack, abSneakAttack, abBashAttack, abHitBlocked)
	endif
EndEvent

