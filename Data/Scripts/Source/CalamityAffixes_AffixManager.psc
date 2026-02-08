Scriptname CalamityAffixes_AffixManager extends Quest

; Central manager template for keyword-driven affixes + proc/ICD handling.

Actor Property PlayerRef Auto Const
FormList Property AffixKeywordList Auto Const ; Keyword list; index = affix id
CalamityAffixes_AffixEffectBase[] Property AffixEffects Auto Const ; Must align with AffixKeywordList indices

Form Property TestItem Auto Const

Bool Property bEnabled = true Auto
Bool Property bDebugNotifications = false Auto
Float Property fValidationInterval = 8.0 Auto
Float Property fProcChanceMult = 1.0 Auto

Float Property DuplicateSuppressWindow = 0.10 Auto Const

Int[] _activeCounts
Float[] _nextAllowedRealTime

Bool _initialized = false
Bool _dispatchingHit = false

Float _lastHitRealTime = -9999.0
Actor _lastHitAggressor
Actor _lastHitVictim
Form _lastHitSource
Bool _lastHitOutgoing

; ----------------------------
; MCM Helper action targets (optional)
; ----------------------------

Function SetEnabled(bool a_enabled)
	bEnabled = a_enabled
	Log("Enabled=" + a_enabled)
EndFunction

Function SetDebugNotificationsEnabled(bool a_enabled)
	bDebugNotifications = a_enabled
	Log("DebugNotifications=" + a_enabled)
EndFunction

Function SetValidationIntervalSeconds(float a_seconds)
	if a_seconds < 0.0
		a_seconds = 0.0
	endif
	fValidationInterval = a_seconds
	Log("ValidationInterval=" + a_seconds)

	; Re-arm the timer with the new interval.
	UnregisterForUpdate()
	if fValidationInterval > 0.0
		RegisterForSingleUpdate(fValidationInterval)
	endif
EndFunction

Function SetProcChanceMultiplier(float a_mult)
	if a_mult < 0.0
		a_mult = 0.0
	endif
	fProcChanceMult = a_mult
	Log("ProcChanceMult=" + a_mult)
EndFunction

Function SpawnTestItem()
	Actor player = GetPlayer()
	if !player || !TestItem
		return
	endif

	player.AddItem(TestItem, 1, true)
	Log("Spawned test item.")
EndFunction

; ----------------------------
; Lifecycle
; ----------------------------

Event OnInit()
	Initialize()
EndEvent

Event OnPlayerLoadGame()
	Initialize()
	ForceResync()
EndEvent

Function Initialize()
	if _initialized
		return
	endif

	if !AffixKeywordList
		return
	endif

	Int n = AffixKeywordList.GetSize()
	_activeCounts = new Int[n]
	_nextAllowedRealTime = new Float[n]

	_initialized = true

	if fValidationInterval > 0.0
		RegisterForSingleUpdate(fValidationInterval)
	endif
EndFunction

Event OnUpdate()
	if bEnabled && fValidationInterval > 0.0
		ForceResync()
		RegisterForSingleUpdate(fValidationInterval)
	endif
EndEvent

; ----------------------------
; Equip tracking
; ----------------------------

Function HandleObjectEquipped(Form akBaseObject, ObjectReference akReference)
	if !_initialized || !bEnabled
		return
	endif

	AdjustCountsForItem(akBaseObject, 1)
EndFunction

Function HandleObjectUnequipped(Form akBaseObject, ObjectReference akReference)
	if !_initialized || !bEnabled
		return
	endif

	AdjustCountsForItem(akBaseObject, -1)
EndFunction

Function ForceResync()
	if !_initialized
		return
	endif

	Int n = AffixKeywordList.GetSize()
	Int[] newCounts = new Int[n]

	Actor player = GetPlayer()
	if !player
		return
	endif

	; Weapons
	Weapon wR = player.GetEquippedWeapon(false)
	if wR
		AccumulateItemKeywords(newCounts, wR)
	endif

	Weapon wL = player.GetEquippedWeapon(true)
	if wL
		AccumulateItemKeywords(newCounts, wL)
	endif

	; Armor slots (bitmask)
	Int mask = 0x00000001
	Int slotIndex = 0
	while slotIndex < 32
		Armor a = player.GetWornForm(mask) as Armor
		if a
			AccumulateItemKeywords(newCounts, a)
		endif
		mask = mask * 2
		slotIndex += 1
	endWhile

	Int i = 0
	while i < n
		_activeCounts[i] = newCounts[i]
		i += 1
	endWhile
EndFunction

Function AdjustCountsForItem(Form akItem, Int delta)
	Weapon w = akItem as Weapon
	Armor a = akItem as Armor
	if !w && !a
		return
	endif

	Int n = AffixKeywordList.GetSize()
	Int i = 0
	while i < n
		Keyword kw = AffixKeywordList.GetAt(i) as Keyword
		if kw
			Bool has = false
			if w
				has = w.HasKeyword(kw)
			else
				has = a.HasKeyword(kw)
			endif

			if has
				_activeCounts[i] = _activeCounts[i] + delta
				if _activeCounts[i] < 0
					_activeCounts[i] = 0
				endif
			endif
		endif
		i += 1
	endWhile
EndFunction

Function AccumulateItemKeywords(Int[] counts, Form akItem)
	Weapon w = akItem as Weapon
	Armor a = akItem as Armor
	if !w && !a
		return
	endif

	Int n = AffixKeywordList.GetSize()
	Int i = 0
	while i < n
		Keyword kw = AffixKeywordList.GetAt(i) as Keyword
		if kw
			Bool has = false
			if w
				has = w.HasKeyword(kw)
			else
				has = a.HasKeyword(kw)
			endif

			if has
				counts[i] = counts[i] + 1
			endif
		endif
		i += 1
	endWhile
EndFunction

; ----------------------------
; Hit processing
; ----------------------------

Function ProcessIncomingHit(ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
	if !_initialized || !bEnabled
		return
	endif

	Actor player = GetPlayer()
	Actor attacker = akAggressor as Actor
	if !player || !attacker
		return
	endif

	ProcessHit(false, attacker, player, akSource, akProjectile, abPowerAttack, abSneakAttack, abBashAttack, abHitBlocked)
EndFunction

Function ProcessOutgoingHit(Actor akTarget)
	if !_initialized || !bEnabled
		return
	endif

	Actor player = GetPlayer()
	if !player || !akTarget
		return
	endif

	; Outgoing hook often can’t provide full hit params; keep them defaulted.
	ProcessHit(true, player, akTarget, None, None, false, false, false, false)
EndFunction

Function ProcessHit(bool isOutgoing, Actor akAggressor, Actor akVictim, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
	if _dispatchingHit
		return
	endif

	Float now = Utility.GetCurrentRealTime()
	if ShouldSuppressDuplicateHit(isOutgoing, akAggressor, akVictim, akSource, now)
		return
	endif

	_dispatchingHit = true

	Int n = AffixKeywordList.GetSize()
	Int i = 0
	while i < n
		if _activeCounts[i] > 0
			CalamityAffixes_AffixEffectBase eff = None
			if AffixEffects && i < AffixEffects.Length
				eff = AffixEffects[i]
			endif
			if eff && eff.CanTrigger(isOutgoing)
				Float nextAllowed = _nextAllowedRealTime[i]
				if now >= nextAllowed
					Float chance = eff.ProcChancePct * fProcChanceMult
					if chance > 100.0
						chance = 100.0
					endif

					Int roll = Utility.RandomInt(0, 99)
					if roll < chance
						_nextAllowedRealTime[i] = now + eff.ICDSeconds
						if isOutgoing
							eff.OnProcOutgoing(GetPlayer(), akVictim, akSource, akProjectile, abPowerAttack, abSneakAttack, abBashAttack, abHitBlocked)
						else
							eff.OnProcIncoming(GetPlayer(), akAggressor, akSource, akProjectile, abPowerAttack, abSneakAttack, abBashAttack, abHitBlocked)
						endif
					endif
				endif
			endif
		endif

		i += 1
	endWhile

	_dispatchingHit = false
EndFunction

Bool Function ShouldSuppressDuplicateHit(bool isOutgoing, Actor akAggressor, Actor akVictim, Form akSource, Float now)
	if (now - _lastHitRealTime) <= DuplicateSuppressWindow
		if (isOutgoing == _lastHitOutgoing) && (akAggressor == _lastHitAggressor) && (akVictim == _lastHitVictim) && (akSource == _lastHitSource)
			return true
		endif
	endif

	_lastHitRealTime = now
	_lastHitOutgoing = isOutgoing
	_lastHitAggressor = akAggressor
	_lastHitVictim = akVictim
	_lastHitSource = akSource
	return false
EndFunction

; ----------------------------
; Helpers
; ----------------------------

Actor Function GetPlayer()
	if PlayerRef
		return PlayerRef
	endif
	return Game.GetPlayer()
EndFunction

Function Log(string msg)
	if !bDebugNotifications
		return
	endif

	Debug.Trace("[CalamityAffixes] " + msg)
EndFunction
