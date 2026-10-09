# Calamity Effect List

> Version `v2.3.0` (2026-10-09). Generated from the mod data; every line is the in-game English text.
> Korean lists: [prefixes](PREFIX_EFFECTS.md), [suffixes](SUFFIX_EFFECTS.md), [runewords](RUNEWORD_EFFECTS.md). Project: [GitHub](https://github.com/servaltullius/calamity-reactive-loot-affixes)

Each item has **3 affix slots**: a head slot that holds **a runeword or a prefix**, then up to **2 suffixes**.

## Reading an entry

- **18% on hit**: the chance each time you hit. **on hit taken**: when you are hit. **on kill**: when you kill.
- **Lucky Hit 35%**: the hit has a 35% chance to trigger the effect.
- **ICD 8s**: internal cooldown; the effect cannot fire again for 8 seconds.
- Listed chances are base values. Your gear and the rules below change how often an effect really fires.

## Stacking rules

- **A runeword and a prefix that both trigger on the same item** each fire at 80% of their listed chance; one triggered affix alone fires at 100%. Passive suffixes never lower it. (Chaos at 16% next to a Crit Cast prefix shows 12.8%.)
- **The same triggered prefix on different items** (spells, adaptive spells, and traps) combines up to 3 copies: `1 - (1-p1)(1-p2)(1-p3)`, so 40% copies fire 40% → 64% → 78.4% of the time with 1, 2, or 3 equipped. It fires more often; it does not hit harder or fire twice.
- **Crit Cast** spells: melee critical and power attacks can cast up to 2 different spells, normal melee hits and bow or crossbow hits up to 1, and all share a 0.15s cooldown.
- **Suffixes of the same family** add their tiers and cap at tier 3: tier 1 + tier 1 = tier 2, tier 1 + tier 2 = tier 3. Only the resulting tier applies (Guardian 1 + 2 gives +80 Armor, not +75).

## Prefixes (73)

Prefixes are triggered effects. An item holds at most one prefix.

### Elemental Strikes

- **Storm Call** (Lucky Hit 38% / ICD 1.5s): 10 Lightning Damage + 10% of Hit Damage
- **Flame Strike** (100% on hit): 6 Fire Damage + 10% of Hit Damage
- **Frost Strike** (100% on hit): 6 Frost Damage + 10% of Hit Damage
- **Spark Strike** (100% on hit): 6 Shock Damage + 10% of Hit Damage

### Elemental Weakness

- **Flame Weakness** (Lucky Hit 36% / ICD 2.8s): Fire Resist -15 (4s)
- **Frost Weakness** (24% on hit while chaining hits in the last 2s / ICD 2.4s): Frost Resist -15 (4s)
- **Shock Weakness** (Lucky Hit 35% / ICD 4s): Shock Resist -25 (4s)

### Summons

- **Wolf Spirit** (30% on hit / ICD 24s): Summon Familiar (60s)
- **Flame Sprite** (Lucky Hit 36% / ICD 14s): Summon Exploding Flame Familiar
- **Flame Atronach** (Lucky Hit 30% / ICD 32s): Summon Flame Atronach (60s)
- **Frost Atronach** (Lucky Hit 30% / ICD 32s): Summon Frost Atronach (60s)
- **Storm Atronach** (Lucky Hit 26% / ICD 40s): Summon Storm Atronach (60s)
- **Dremora Pact** (Lucky Hit 22% / ICD 52s): Summon Dremora Lord (60s)
- **Soul Harvest** (Lucky Hit 30% / ICD 1s): Restore Magicka & Stamina 8 + 3% of Physical Damage (max 24)

### Defense When Hit

- **Shadow Counter** (on hit taken / ICD 30s): Invisibility 3s + Attack Damage +25% 6s
- **Stone Ward** (10% on hit taken / ICD 6s): Damage Resist +120 (3s)
- **Arcane Ward** (18% on hit taken / ICD 8s): Magic Resist +25% (4s)
- **Healing Surge** (20% on hit taken / ICD 9s): Health Regen +140% (5s)
- **Mage Armor I**: Redirect 10% of physical hit damage to Magicka
- **Mage Armor II**: Redirect 15% of physical hit damage to Magicka

### Scroll Mastery

- **Scroll Mastery I**: +20% chance to not consume scrolls
- **Scroll Mastery II**: +25% chance to not consume scrolls
- **Scroll Mastery III**: +30% chance to not consume scrolls
- **Scroll Mastery IV**: +35% chance to not consume scrolls

### Burning

- **Ember Brand** (40% on hit / ICD 0.8s): Ignite 4 + 4% of Hit Damage (4s)
- **Ember Pyre** (18% on kill / ICD 1s): Corpse explosion (320 radius, 8% max health + 14 fire)

### Control and Debuffs

- **Ice Shackle** (24% on hit / ICD 2.5s): Frost snare -35% (2s)
- **Mana Burn** (Lucky Hit 35% / ICD 1.2s): Drain 60 Magicka
- **Life Drain** (Lucky Hit 38% / ICD 0.7s): Heal 4 + 4.5% of physical hit damage (max 24)

### Utility

- **Soul Siphon** (30% on kill / ICD 2s): Magicka Regen +35% (8s)
- **Shadow Stride** (on kill / ICD 6s): Weapon Attack Speed +15% (4s), evolves to +18%/+21% after 10/30 procs
- **Stealth Assault** (on kill / ICD 8s): Invisibility 3s + Move Speed +30% 6s
- **Battle Frenzy** (20% on hit / ICD 0.6s / per-target ICD 12s): Weapon Attack Speed +15% (4s)

### Traps and Runes

- **Bear Trap** (20% crit/power attack; 40% normal weapon hit / ICD 6s): Deploy trap at enemy feet (2.5s reload) -> Snare -100% Move Speed (2s) + Random bonus effect
- **Rune Trap** (12% on hit / ICD 1.5s): Deploy mine at target (0.6s delay, 8s, 1 charge) -> Slow up to 2 enemies within radius 150 (-40%, 2s)
- **Chaos Rune** (12% on hit / ICD 2s / per-target ICD 8s): Deploy mine at target (0.8s delay, 10s, 1 charge) -> Slow and curse up to 2 enemies within radius 150

### Elemental Infusion (Damage Conversion)

- **Fire Infusion**: Convert 50% Physical to Fire
- **Fire Infusion**: Convert 100% Physical to Fire
- **Frost Infusion**: Convert 50% Physical to Frost
- **Frost Infusion**: Convert 100% Physical to Frost
- **Shock Infusion**: Convert 50% Physical to Shock
- **Shock Infusion**: Convert 100% Physical to Shock

### Archmage

- **Archmage** (on spell hit): 10% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
- **Archmage** (on spell hit): 12% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
- **Archmage** (on spell hit): 14% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
- **Archmage** (on spell hit): 16% Max Magicka as Lightning Damage / 5% Max Magicka extra cost

### Crit Cast

- **Crit Cast**: Firebolt (100% Melee Crit/Power Attack; 45% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Firebolt; damage is the greater of the spell's base damage and 30% of physical hit damage.
- **Crit Cast**: Ice Spike (100% Melee Crit/Power Attack; 45% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Ice Spike; damage is the greater of the spell's base damage and 30% of physical hit damage.
- **Crit Cast**: Lightning Bolt (100% Melee Crit/Power Attack; 45% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Lightning Bolt; damage is the greater of the spell's base damage and 30% of physical hit damage.
- **Crit Cast**: Thunderbolt (100% Melee Crit/Power Attack; 35% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Thunderbolt; damage is the greater of the spell's base damage and 30% of physical hit damage.
- **Crit Cast**: Icy Spear (100% Melee Crit/Power Attack; 35% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Icy Spear; damage is the greater of the spell's base damage and 30% of physical hit damage.
- **Crit Cast**: Chain Lightning (100% Melee Crit/Power Attack; 35% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Chain Lightning; damage is the greater of the spell's base damage and 30% of physical hit damage.
- **Crit Cast**: Ice Storm (100% Melee Crit/Power Attack; 35% Normal Melee Hit; any Bow/Crossbow Hit (100%); ICD 0.15s): Cast Ice Storm; damage is the greater of the spell's base damage and 30% of physical hit damage.

### Death Pyre (Corpse Explosions)

- **Death Pyre T1** (on kill): Radius 600, (12 + 6% corpse max HP) Fire Damage. Chain(0.7x) max 5, 4/sec limit
- **Death Pyre T2** (on kill): Radius 600, (14 + 7% corpse max HP) Fire Damage. Chain(0.7x) max 5, 4/sec limit
- **Death Pyre T3** (on kill): Radius 600, (16 + 8% corpse max HP) Fire Damage. Chain(0.7x) max 5, 4/sec limit

### Conjured Pyre

- **Conjured Pyre T1** (on summon death): Radius 600, (14 + 7% summon max HP) Fire Damage. Chain(0.8x) max 3, 3/sec limit
- **Conjured Pyre T2** (on summon death): Radius 600, (18 + 8.5% summon max HP) Fire Damage. Chain(0.8x) max 3, 3/sec limit

### Plague and Adaptive

- **Elemental Bane** (20% on hit / ICD 1s / per-target ICD 12s): Highest elemental resist -15 (4s)
- **Plague Spore** (20% on hit / ICD 1.5s): Spore burst after 0.6s (Up to 2 enemies within radius 150, Poison 3/s, 4s)
- **Tar Blight** (18% on hit / ICD 2s): Tar burst after 0.6s (Up to 2 enemies within radius 170, Slow -35% + Armor -150, 4s)
- **Siphon Spore** (18% on hit / ICD 2s): Siphon burst after 0.6s (Up to 2 enemies within radius 170, Magicka/Stamina Regen -60%, 6s)

### Evolving

- **Thunder Mastery** (100% on hit): Lightning Damage Growth (x1.0→1.25→1.6→2.1, stages: 15/45/90 hits)
- **Elemental Attunement** (100% on hit): Fire/Frost/Shock manual switch + Growth (x1.0→1.3→1.7→2.2, stages: 20/60/120 hits)

### Thu'um (Shouts)

- **Voice of Power** (25% on hit taken / ICD 8s): Restore 30 Stamina and gain +15% Attack Damage for 5s
- **Death's Mark** (Lucky Hit 25% / ICD 1s / per-target ICD 10s): Armor -200 (6s)
- **Ice Form** (Lucky Hit 15% / ICD 10s): Move Speed -100% (2s)
- **Disarming Shout** (Lucky Hit 10% / ICD 10s): Attack Speed Reduction
- **Whirlwind Sprint** (100% on kill / ICD 3s): Move Speed Boost (Growth: x1.0→1.25→1.55→2.0)

### Schools of Magic

- **Spell Breach** (Lucky Hit 12% / ICD 10s): Magic Resist Reduction
- **Stamina Drain** (Lucky Hit 20% / ICD 4s): Stamina Drain
- **Nourishing Flame** (on hit after a kill in the last 5s / ICD 2s): Heal 6 + 10% of Physical Hit Damage (max 50)
- **Mana Knot** (Lucky Hit 35% / ICD 5s): Magicka Regen -100% (6s)

## Suffixes (66 in 22 families)

Suffixes are passive bonuses. An item holds up to two suffixes, each from a different family.
Tier names read *of Minor X* (tier 1), *of X* or *of the X* (tier 2), and *of Grand X* (tier 3).

| Family | Tier 1 | Tier 2 | Tier 3 | Good for |
| --- | --- | --- | --- | --- |
| **Assassin** | Critical Chance +10%, Critical Damage +10% | Critical Chance +20%, Critical Damage +20% | Critical Chance +30%, Critical Damage +30% | Critical hits |
| **Brilliance** | Max Magicka +20 | Max Magicka +40 | Max Magicka +60 | Mages |
| **Bulwark** | Block +5% | Block +10% | Block +15% | Shield tanks |
| **Champion** | Two-Handed +5% | Two-Handed +10% | Two-Handed +15% | Two-handed fighters |
| **Conjurer** | Conjuration +5% | Conjuration +10% | Conjuration +15% | Conjurers |
| **Eagle Eye** | Bow Speed +5% | Bow Speed +10% | Bow Speed +15% | Archers |
| **Enchanter** | Enchanting +5% | Enchanting +10% | Enchanting +15% | Enchanters |
| **Endurance** | Max Stamina +20 | Max Stamina +40 | Max Stamina +60 | Melee and archers |
| **Evasion** | Light Armor +5% | Light Armor +10% | Light Armor +15% | Light armor |
| **Fortitude** | Heavy Armor +5% | Heavy Armor +10% | Heavy Armor +15% | Heavy armor |
| **Gladiator** | Attack Speed +5% | Attack Speed +10% | Attack Speed +15% | Attack speed |
| **Guardian** | Armor +25 | Armor +50 | Armor +80 | Physical defense |
| **Marksman** | Archery +5% | Archery +10% | Archery +15% | Archers |
| **Meditation** | Magicka Regen +25% | Magicka Regen +50% | Magicka Regen +75% | Mages |
| **Regeneration** | Health Regen +25% | Health Regen +50% | Health Regen +75% | Anyone |
| **Shadow** | Sneak +5% | Sneak +10% | Sneak +15% | Stealth |
| **Spell Ward** | Magic Resist +3% | Magic Resist +8% | Magic Resist +12% | Magic resistance |
| **Steed** | Carry Weight +15 | Carry Weight +30 | Carry Weight +50 | Carry weight |
| **Swiftness** | Move Speed +4% | Move Speed +7% | Move Speed +10% | Movement |
| **Swordsmanship** | One-Handed +5% | One-Handed +10% | One-Handed +15% | One-handed fighters |
| **Tenacity** | Stamina Regen +25% | Stamina Regen +50% | Stamina Regen +75% | Melee |
| **Vitality** | Max Health +25 | Max Health +50 | Max Health +75 | Anyone |

## Runewords (95)

Collect the listed rune fragments, pick the recipe in the panel's Runeword tab, and transmute it onto an equipped weapon or armor.
The suggested base is only a hint: the panel names a more specific one, and no base ever blocks the transmute.

### Runes

| Rarity | Runes |
| --- | --- |
| Common | El, Eld, Tir, Nef, Eth, Ith, Tal, Ral, Ort, Thul, Amn |
| Uncommon | Sol, Shael, Dol, Hel, Io, Lum, Ko, Fal, Lem, Pul, Um |
| Rare | Mal, Ist, Gul, Vex, Ohm, Lo |
| Very rare | Sur, Ber, Jah, Cham, Zod |

### 2 runes (13)

| Runeword | Runes | Base | Effect |
| --- | --- | --- | --- |
| **Leaf** | Tir-Ral | Weapon | 22% on hit / ICD 7s - Leaf Flame (Destruction +20, 8s) |
| **Lore** | Ort-Sol | Armor | 20% on hit taken / ICD 11s - Light of Lore (Magicka Regen +80%, 7s) |
| **Nadir** | Nef-Tir | Armor | 12% on hit taken / ICD 16s - Nadir of Fear (Fear 5s) |
| **Prudence** | Mal-Tir | Armor | 17% on hit taken / ICD 11s - Shock Ward (Shock Resist +40, 7s) |
| **Rhyme** | Shael-Eth | Armor | 22% on Hit Taken / ICD 12s - Shield Ward (Armor +60, 6s) + Frost Resist +40 |
| **Smoke** | Nef-Lum | Armor | 22% on hit taken / ICD 12s - Slow the attacker by 30% for 5s |
| **Splendor** | Eth-Lum | Armor | 18% on hit taken / ICD 10s - Magic Shield (Magic Resist +30, 7s) |
| **Stealth** | Tal-Eth | Armor | 10% on hit taken / ICD 20s - Emergency Stealth (Invisibility 2s) |
| **Steel** | Tir-El | Weapon | 18% on hit / ICD 8s - Steel Bleed (Heal Suppress -80%, 5s) |
| **Strength** | Amn-Tir | Weapon | 20% on hit / ICD 9s - Heavy Strike (Two-Handed +25, 6s) |
| **White** | Dol-Io | Weapon | 18% on hit / ICD 8s - Mana Surge (Magicka +50, 10s) |
| **Wind** | Sur-El | Weapon | 22% on hit / ICD 4s - Gale Pressure (Weapon Speed -20%, 5s) |
| **Zephyr** | Ort-Eth | Weapon | 20% on hit / ICD 8s - Gale Shot (Bow Speed +30%, 6s) |

### 3 runes (42)

| Runeword | Runes | Base | Effect |
| --- | --- | --- | --- |
| **Ancient's Pledge** | Ral-Ort-Tal | Armor | 20% on hit taken / ICD 11s - Ancient Pledge (Poison Resist +60, 7s) |
| **Black** | Thul-Io-Nef | Weapon | 8% on hit / ICD 20s - Dark Shock (Paralysis 2s) |
| **Bone** | Sol-Um-Um | Armor | 16% on hit taken / ICD 11s - Frost Ward (Frost Resist +45, 7s) |
| **Bulwark** | Shael-Io-Sol | Armor | 22% on hit taken / ICD 11s - Bulwark (Armor +80, 7s) |
| **Chaos** | Fal-Ohm-Um | Weapon | 16% on Hit / ICD 5.5s - Adaptive Element (weakest resist) |
| **Crescent Moon** | Shael-Um-Tir | Weapon | 24% on hit / ICD 6s - Shock Shred (Shock Resist -35, 6s) |
| **Cure** | Shael-Io-Tal | Armor | 22% on hit taken / ICD 11s - Healing Awakening (Restoration +20, 8s) |
| **Delirium** | Lem-Ist-Io | Armor | 10% on hit / ICD 15s - Delirium (Frenzy 5s) |
| **Dragon** | Sur-Lo-Sol | Armor | 30% on hit taken / ICD 10s - Dragon Scales (Armor +120 and Fire/Frost/Shock Resist +25, 8s) |
| **Dream** | Io-Jah-Pul | Armor | 30% on Hit / ICD 0.8s - Shock Strike + Shock Resist +20 |
| **Duress** | Shael-Um-Thul | Armor | 20% on hit taken / ICD 12s - Weaken Attacker (Attack -25%, 5s) |
| **Edge** | Tir-Tal-Amn | Weapon | 20% on hit / ICD 8s - Sharp Shot (Marksman +20, 7s) |
| **Enigma** | Jah-Ith-Ber | Armor | on Hit Taken / ICD 30s - Phase Escape (Invisibility and Move Speed +45%, 4s) + Move Speed +10% |
| **Enlightenment** | Pul-Ral-Sol | Armor | 22% on hit taken / ICD 11s - Enlightenment (Alteration +25, 8s) |
| **Flickering Flame** | Nef-Pul-Vex | Armor | 100% on hit / ICD 0.5s - Flame Erosion (Target Fire Resist -25, 6s) |
| **Fury** | Jah-Gul-Eth | Weapon | 24% on Hit / ICD 12s - Attack Speed +25% for 6s and restore 30 Stamina |
| **Gloom** | Fal-Um-Pul | Armor | 12% on hit taken / ICD 18s - Dark Siphon (Absorb Magicka 30) |
| **Ground** | Shael-Io-Ort | Armor | 22% on hit taken / ICD 11s - Grounding Force (Shout Recovery +50%, 8s) |
| **Hearth** | Shael-Io-Thul | Armor | 22% on hit taken / ICD 11s - Conjuration Ember (Conjuration +20, 7s) |
| **Hustle-A** | Shael-Ko-Eld | Armor | 26% on Hit Taken / ICD 10s - Defense Boost (Armor +60, 5s) + Move Speed +8% |
| **Hustle-W** | Shael-Ko-Eld | Weapon | 26% on Hit / ICD 7s - Rush (Move+Attack Speed) + Attack Speed +12% |
| **King's Grace** | Amn-Ral-Thul | Weapon | 22% on hit / ICD 9s - Holy Healing (HealRate +8, 7s) |
| **Lawbringer** | Amn-Lem-Ko | Weapon | 40% on kill / ICD 6s - Justice (Turn Undead AoE, ICD 6s) |
| **Lionheart** | Hel-Lum-Fal | Armor | HP<30% / ICD 45s - Lionheart (Emergency HP 200 restore) |
| **Malice** | Ith-El-Eth | Weapon | 20% on hit / ICD 9s - Venom Infection (Poison DoT 8/s, 6s) |
| **Melody** | Shael-Ko-Nef | Weapon | 18% on hit / ICD 8s - Melodic Drain (Absorb Stamina 20) |
| **Metamorphosis** | Io-Cham-Fal | Armor | 5% on hit taken / ICD 30s - Metamorphosis (Ethereal Invincibility 3s) |
| **Mosaic** | Mal-Gul-Amn | Weapon | 26% on hit / ICD 5s - Fire/Frost/Shock Damage 8-80 each (5% of Hit Damage each) |
| **Myth** | Hel-Amn-Nef | Armor | 20% on hit taken / ICD 11s - Mythic Armor (Heavy Armor +25, 7s) |
| **Pattern** | Tal-Ort-Thul | Weapon | 20% on hit / ICD 8s - Combo Rhythm (Crit Chance +15%, 6s) |
| **Peace** | Shael-Thul-Amn | Armor | 10% on hit taken / ICD 16s - Wave of Peace (Calm 6s) |
| **Plague** | Cham-Shael-Um | Weapon | 40% on Kill / ICD 4s - Plague Corpse Chain Explosion (12 + 3% Corpse Max Health, Radius 450, up to 12 targets, max chain depth 2) |
| **Principle** | Ral-Gul-Eld | Armor | 28% on hit taken / ICD 10s - Principle Shield (Fire Resist +50, 7s) |
| **Radiance** | Nef-Sol-Ith | Armor | 15% on hit taken / ICD 40s - Radiance (Detect Life 200m, 30s) |
| **Rain** | Ort-Mal-Ith | Armor | 22% on hit taken / ICD 11s - Rain of Life (Heal Rate +100%, 8s) |
| **Sanctuary** | Ko-Ko-Mal | Armor | 100% on Hit Taken / ICD 0.5s - Reflect (15%) + Magic Resist +15 |
| **Shadow Boxer** | Shael-Ko-Um | Weapon | on Melee Hit / ICD 12s - Shadow Boxing (8s): your shadow repeats each melee hit 0.4s later for 40% of its physical damage |
| **Temper** | Shael-Io-Ral | Armor | 18% on hit taken / ICD 8s - Tempered Endurance (Stamina Regen +5, 8s) |
| **Treachery** | Shael-Thul-Lem | Armor | 26% on Hit Taken / ICD 10s - Haste (Move Speed +28%, 5s) + Attack Speed +10% |
| **Venom** | Tal-Dol-Mal | Weapon | 24% on hit / ICD 6s - Poison Shred (Poison Resist -40, 6s) |
| **Wealth** | Lem-Ko-Tir | Armor | Always active - Carry Weight +75 and Speechcraft +15 |
| **Wisdom** | Pul-Ith-Eld | Armor | 18% on hit taken / ICD 11s - Wisdom Depth (Magicka Regen +5, 7s) |

### 4 runes (25)

| Runeword | Runes | Base | Effect |
| --- | --- | --- | --- |
| **Bramble** | Ral-Ohm-Sur-Eth | Armor | 22% on hit taken / ICD 10s - Bramble (Reflect Damage +25%, 6s) |
| **Brand** | Jah-Lo-Mal-Gul | Weapon | 28% on hit / ICD 5s - Brand (Fire DoT 10/s, 6s) |
| **Chains of Honor** | Dol-Um-Ber-Ist | Armor | 20% on Hit Taken / ICD 16s - Phase (Move Speed +35%, 5s) + Fire/Frost/Shock Resist +15 |
| **Exile** | Vex-Ohm-Ist-Dol | Armor | HP<40% / ICD 50s - Iron Skin (Armor+250, 10s) |
| **Faith** | Ohm-Jah-Lem-Eld | Weapon | 22% on Hit / ICD 12s - Fanaticism (Attack Damage +20%, Attack Speed +15%, 6s) + Attack Speed +15% |
| **Famine** | Fal-Ohm-Ort-Jah | Weapon | 30% on hit / ICD 7s - Famine (Stamina -30/s for 5s, Attack Damage -20% and Attack Speed -15% for 6s) |
| **Fortitude** | El-Sol-Dol-Lo | Any | 25% on Hit Taken / ICD 16s - Damage Resist boost + Armor +80 |
| **Hand of Justice** | Sur-Cham-Amn-Lo | Weapon | 28% on Hit / ICD 4s - Fire Judgment Absorb (40 or 18% of Hit Damage, max 250) |
| **Harmony** | Tir-Ith-Sol-Ko | Weapon | 20% on hit / ICD 6s - Harmonic Disruption (Stamina Regen -80%, 5s) |
| **Heart of the Oak** | Ko-Vex-Pul-Thul | Weapon | 32% on hit / ICD 8s - Arcane Rift (Adaptive elemental Resist Reduction) |
| **Holy Thunder** | Eth-Ral-Ort-Tal | Weapon | 22% on hit / ICD 7s - Holy Thunder (Shock Damage 30) |
| **Ice** | Amn-Shael-Jah-Lo | Weapon | 100% on hit / ICD 0.5s - Frost Shards (Target Frost Resist -30, 6s) |
| **Infinity** | Ber-Mal-Ber-Ist | Weapon | 100% on Hit / ICD 0.5s / perTarget 6s - Element Resist Reduction |
| **Insight** | Ral-Tir-Tal-Sol | Weapon | 30% on Hit / ICD 10s - Magicka Recovery boost + Magicka Regen +25 |
| **Kingslayer** | Mal-Um-Gul-Fal | Weapon | 30% on Hit / ICD 0.6s - Bleed (3/s × 8s, 4% phys) |
| **Memory** | Lum-Io-Sol-Eth | Weapon | 20% on hit / ICD 9s - Stamina Recall (Stamina Regen +100%, 8s) |
| **Oath** | Shael-Pul-Mal-Lum | Weapon | 24% on hit / ICD 7s - Oath Power (One-Handed +30, 7s) |
| **Passion** | Dol-Ort-Eld-Lem | Weapon | 26% on hit / ICD 7s - Passionate Illusion (Illusion +20, 7s) |
| **Phoenix** | Vex-Vex-Lo-Jah | Any | 60% on Kill / ICD 2s - Restore HP+MP |
| **Pride** | Cham-Sur-Io-Lo | Weapon | 30% on hit / ICD 4s - Frost Impact (16% of Physical Hit Damage, 30-200 Frost Damage, Radius 250) |
| **Rift** | Hel-Ko-Lem-Gul | Weapon | 8% on hit / ICD 18s - Magic Shred (Magic Resist -30, 6s) |
| **Spirit** | Tal-Thul-Ort-Amn | Any | 28% on Hit / ICD 10s - Absorb Chance +10 points for 5s + Max Magicka +30 |
| **Stone** | Shael-Um-Pul-Lum | Armor | 22% on hit taken / ICD 8s - Armor Crush (Target Armor -50, 6s) |
| **Voice of Reason** | Lem-Ko-El-Eld | Weapon | 22% on hit / ICD 7s - Frost Shred (Frost Resist -35, 6s) |
| **Wrath** | Pul-Lum-Ber-Mal | Weapon | 26% on hit / ICD 5s - Divine Wrath (Fire Damage 30) |

### 5 runes (10)

| Runeword | Runes | Base | Effect |
| --- | --- | --- | --- |
| **Beast** | Ber-Tir-Um-Mal-Lum | Weapon | 30% on Hit / ICD 18s - Beast Rage (Attack Damage +30%, Armor +150, Attack Speed +15%, 10s) |
| **Call to Arms** | Amn-Ral-Mal-Ist-Ohm | Weapon | 24% on Hit / ICD 18s - Battle Cry (Attack Damage +20%, 8s) + HP +50, MP +30 |
| **Death** | Hel-El-Vex-Ort-Gul | Weapon | 24% on hit / ICD 5s - Death Touch (Absorb Health 15) |
| **Destruction** | Vex-Lo-Ber-Jah-Ko | Weapon | 30% on hit / ICD 5s - Shock Storm (4% of Physical Hit Damage, 10-40 Shock Damage/s, 5s, Radius 350) |
| **Doom** | Hel-Ohm-Um-Lo-Cham | Weapon | 100% on Hit / ICD 0.5s - Frost Slow (Speed-30%, 4s) |
| **Eternity** | Amn-Ber-Ist-Sol-Sur | Weapon | 25% on Hit Taken / ICD 15s - Eternal Bulwark (Armor +300, Reflect Damage +25%, 6s) |
| **Grief** | Eth-Tir-Lo-Mal-Ral | Weapon | 30% on Hit / ICD 0.5s - Heal 8% of Hit Damage + Attack Speed +10% |
| **Honor** | Amn-El-Ith-Tir-Sol | Weapon | 26% on Hit / ICD 8s - Vigor (Health Regen Rate +80%, 7s) + Health Regen Rate +20% |
| **Mist** | Cham-Shael-Gul-Thul-Ith | Weapon | 30% on hit / ICD 6s - Arcane Collapse (Magicka Regen -100% and Magic Resist -30 for 6s, instant Magicka drain 75) |
| **Obedience** | Hel-Ko-Thul-Eth-Fal | Weapon | 18% on hit / ICD 5s - Crush (Target Speed -50%, 3s) |

### 6 runes (5)

| Runeword | Runes | Base | Effect |
| --- | --- | --- | --- |
| **Breath of the Dying** | Vex-Hel-El-Eld-Zod-Eth | Weapon | 50% on Kill / ICD 3s - Poison Corpse Explosion (24 + 6% Corpse Max Health, Radius 600) |
| **Last Wish** | Jah-Mal-Jah-Sur-Jah-Ber | Weapon | HP<35% / ICD 45s - Restore 250 Health + Armor 250 and Magic Resist 50 (12s) |
| **Obsession** | Zod-Ist-Lem-Lum-Io-Nef | Weapon | 34% on Hit / ICD 5s - Magic Damage 75-300 (18% of Hit Damage) + Magicka Regen Rate +25% |
| **Silence** | Dol-Eld-Hel-Ist-Tir-Vex | Weapon | 26% on hit / ICD 7s - Silence (Magicka -50, 5s) |
| **Unbending Will** | Fal-Io-Ith-Eld-El-Hel | Weapon | 22% on hit / ICD 9s - Unbending Will (Block +30, 7s) |
