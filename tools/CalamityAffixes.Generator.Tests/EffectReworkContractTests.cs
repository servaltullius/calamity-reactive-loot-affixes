using System.Text.Json;

namespace CalamityAffixes.Generator.Tests;

public sealed class EffectReworkContractTests
{
    [Fact]
    public void VoiceOfPower_UsesDedicatedCompositeSpellAndKeepsSharedLegacyRecords()
    {
        var affix = ReadAffix("keywords.affixes.core.json", "voice_of_power");
        var records = affix.GetProperty("records");
        var runtime = affix.GetProperty("runtime");

        Assert.Equal("CAFF_MGEF_DMG_FIRE_DYNAMIC", records.GetProperty("magicEffect").GetProperty("editorId").GetString());
        Assert.Equal("CAFF_SPEL_DMG_FIRE_DYNAMIC", records.GetProperty("spell").GetProperty("editorId").GetString());
        Assert.Equal("IncomingHit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(25.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.Equal(8.0, runtime.GetProperty("icdSeconds").GetDouble());

        var action = runtime.GetProperty("action");
        Assert.Equal("CastSpell", action.GetProperty("type").GetString());
        Assert.Equal("CAFF_SPEL_INCOMING_VOICE_POWER", action.GetProperty("spellEditorId").GetString());
        Assert.Equal("Self", action.GetProperty("applyTo").GetString());
    }

    [Fact]
    public void Spirit_KeepsOldActivePairAsTombstoneAndUsesAbsorptionTail()
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_spirit_final");
        var magicEffects = affix.GetProperty("records").GetProperty("magicEffects").EnumerateArray().ToArray();
        var spells = affix.GetProperty("records").GetProperty("spells").EnumerateArray().ToArray();

        Assert.Equal(2, magicEffects.Length);
        Assert.Equal("CAFF_MGEF_RW_SPIRIT_MEDITATION", magicEffects[0].GetProperty("editorId").GetString());
        Assert.Equal("MagickaRateMult", magicEffects[0].GetProperty("actorValue").GetString());
        Assert.False(magicEffects[0].GetProperty("hostile").GetBoolean());
        Assert.True(magicEffects[0].GetProperty("recover").GetBoolean());

        Assert.Equal("CAFF_SPEL_RW_SPIRIT_MEDITATION", spells[0].GetProperty("editorId").GetString());
        Assert.Equal("Self", spells[0].GetProperty("delivery").GetString());
        AssertEffect(spells[0].GetProperty("effect"), "CAFF_MGEF_RW_SPIRIT_MEDITATION", 120.0, 10);

        Assert.Equal("CAFF_MGEF_RW_SPIRIT_PASSIVE_MPREGEN", magicEffects[1].GetProperty("editorId").GetString());
        Assert.Equal("Magicka", magicEffects[1].GetProperty("actorValue").GetString());
        Assert.Equal("CAFF_SPEL_RW_SPIRIT_PASSIVE", spells[1].GetProperty("editorId").GetString());
        Assert.Equal("Ability", spells[1].GetProperty("spellType").GetString());
        Assert.Equal("ConstantEffect", spells[1].GetProperty("castType").GetString());
        AssertEffect(spells[1].GetProperty("effect"), "CAFF_MGEF_RW_SPIRIT_PASSIVE_MPREGEN", 30.0, 0);

        var runtime = affix.GetProperty("runtime");
        Assert.Equal("Hit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(28.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.Equal(10.0, runtime.GetProperty("icdSeconds").GetDouble());
        var action = runtime.GetProperty("action");
        Assert.Equal("CAFF_SPEL_RW_SPIRIT_ABSORPTION", action.GetProperty("spellEditorId").GetString());
        Assert.Equal("CAFF_SPEL_RW_SPIRIT_PASSIVE", action.GetProperty("passiveSpellEditorId").GetString());
        Assert.True(action.GetProperty("refreshPassiveSpellOnPostLoad").GetBoolean());
    }

    [Fact]
    public void Smoke_SlowsTheIncomingAttacker()
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_smoke_final");
        var magicEffect = affix.GetProperty("records").GetProperty("magicEffect");
        var spell = affix.GetProperty("records").GetProperty("spell");
        var runtime = affix.GetProperty("runtime");

        Assert.Equal("SpeedMult", magicEffect.GetProperty("actorValue").GetString());
        Assert.True(magicEffect.GetProperty("hostile").GetBoolean());
        Assert.True(magicEffect.GetProperty("recover").GetBoolean());
        Assert.Equal("TargetActor", spell.GetProperty("delivery").GetString());
        AssertEffect(spell.GetProperty("effect"), "CAFF_MGEF_RW_SMOKE_SCREEN", 30.0, 5);
        Assert.Equal("IncomingHit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(22.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.Equal(12.0, runtime.GetProperty("icdSeconds").GetDouble());
        Assert.Equal("Target", runtime.GetProperty("action").GetProperty("applyTo").GetString());
    }

    [Fact]
    public void Fury_StrikesAndFillsBleedWithoutCriticalChance()
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_fury_final");
        var records = affix.GetProperty("records");
        var runtime = affix.GetProperty("runtime");
        var action = runtime.GetProperty("action");

        // v2.3.0 stage 4: the attack damage buff overlapped Faith, Beast and Call
        // to Arms (and buff replacement made them exclusive), so Fury now strikes
        // the target and fills its Bleed meter.
        var magicEffect = records.GetProperty("magicEffect");
        Assert.Equal("CAFF_MGEF_RW_FURY_LIFESTEAL", magicEffect.GetProperty("editorId").GetString());
        Assert.Equal("Health", magicEffect.GetProperty("actorValue").GetString());
        Assert.True(magicEffect.GetProperty("hostile").GetBoolean());
        var spell = records.GetProperty("spell");
        Assert.Equal("TargetActor", spell.GetProperty("delivery").GetString());
        AssertEffect(spell.GetProperty("effect"), "CAFF_MGEF_RW_FURY_LIFESTEAL", 15.0, 0);
        Assert.Equal("Target", action.GetProperty("applyTo").GetString());
        Assert.Equal("Bleed", action.GetProperty("statusTag").GetString());
        Assert.Equal(100.0, action.GetProperty("statusAmount").GetDouble());
        Assert.Equal("Hit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(24.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.Equal(6.0, runtime.GetProperty("icdSeconds").GetDouble());
        Assert.DoesNotContain("CriticalChance", affix.GetRawText(), StringComparison.Ordinal);
    }

    [Fact]
    public void Wealth_KeepsOldProcPairAsTombstoneAndUsesPassiveOnlySentinel()
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_wealth_final");
        var records = affix.GetProperty("records");

        var oldMagicEffect = records.GetProperty("magicEffect");
        Assert.Equal("CAFF_MGEF_RW_WEALTH_VIGOR", oldMagicEffect.GetProperty("editorId").GetString());
        Assert.Equal("CarryWeight", oldMagicEffect.GetProperty("actorValue").GetString());
        Assert.False(oldMagicEffect.GetProperty("hostile").GetBoolean());
        Assert.True(oldMagicEffect.GetProperty("recover").GetBoolean());

        var oldSpell = records.GetProperty("spell");
        Assert.Equal("CAFF_SPEL_RW_WEALTH_VIGOR", oldSpell.GetProperty("editorId").GetString());
        Assert.Equal("Self", oldSpell.GetProperty("delivery").GetString());
        AssertEffect(oldSpell.GetProperty("effect"), "CAFF_MGEF_RW_WEALTH_VIGOR", 100.0, 15);

        var runtime = affix.GetProperty("runtime");
        Assert.Equal("Hit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(0.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.False(runtime.TryGetProperty("icdSeconds", out _));
        var action = runtime.GetProperty("action");
        Assert.Equal("DebugNotify", action.GetProperty("type").GetString());
        Assert.Equal("CAFF_SPEL_RW_WEALTH_PASSIVE", action.GetProperty("passiveSpellEditorId").GetString());
        Assert.False(action.TryGetProperty("spellEditorId", out _));
    }

    [Fact]
    public void TypedTail_HasApprovedEffectSemantics()
    {
        var root = ReadJson(Path.Combine("affixes", "modules", "spec.root.json"));
        var records = root.GetProperty("keywords").GetProperty("appendedRecords").EnumerateArray().ToArray();

        Assert.Equal(113, records.Length);
        AssertMagicEffect(records[0], "CAFF_MGEF_INCOMING_VOICE_POWER_STAMINA", "Stamina", hostile: false, recover: false);
        AssertMagicEffect(records[1], "CAFF_MGEF_INCOMING_VOICE_POWER_ATTACK_DAMAGE", "AttackDamageMult", hostile: false, recover: true);
        AssertSpellEffects(records[2], "CAFF_SPEL_INCOMING_VOICE_POWER",
            ("CAFF_MGEF_INCOMING_VOICE_POWER_STAMINA", 30.0, 0),
            ("CAFF_MGEF_INCOMING_VOICE_POWER_ATTACK_DAMAGE", 0.15, 5));
        AssertMagicEffect(records[3], "CAFF_MGEF_RW_SPIRIT_ABSORB_CHANCE", "AbsorbChance", hostile: false, recover: true);
        AssertSpellEffects(records[4], "CAFF_SPEL_RW_SPIRIT_ABSORPTION",
            ("CAFF_MGEF_RW_SPIRIT_ABSORB_CHANCE", 10.0, 5));
        AssertMagicEffect(records[5], "CAFF_MGEF_RW_FURY_RESTORE_STAMINA", "Stamina", hostile: false, recover: false);
        AssertMagicEffect(records[6], "CAFF_MGEF_RW_WEALTH_PASSIVE_CARRY_WEIGHT", "CarryWeight", hostile: false, recover: true);
        AssertMagicEffect(records[7], "CAFF_MGEF_RW_WEALTH_PASSIVE_SPEECHCRAFT", "SpeechcraftModifier", hostile: false, recover: true);
        var wealthSpell = records[8].GetProperty("spell");
        Assert.Equal("Ability", wealthSpell.GetProperty("spellType").GetString());
        Assert.Equal("ConstantEffect", wealthSpell.GetProperty("castType").GetString());
        AssertSpellEffects(records[8], "CAFF_SPEL_RW_WEALTH_PASSIVE",
            ("CAFF_MGEF_RW_WEALTH_PASSIVE_CARRY_WEIGHT", 75.0, 0),
            ("CAFF_MGEF_RW_WEALTH_PASSIVE_SPEECHCRAFT", 15.0, 0));

        // v2.3.0 review tail (positional FormIDs: these stay last, in this order).
        AssertMagicEffect(records[96], "CAFF_MGEF_DMG_FIRE_DYNAMIC_HITFX", "None", hostile: true, recover: false);
        AssertMagicEffect(records[97], "CAFF_MGEF_DMG_FROST_DYNAMIC_HITFX", "None", hostile: true, recover: false);
        AssertMagicEffect(records[98], "CAFF_MGEF_DMG_SHOCK_DYNAMIC_HITFX", "None", hostile: true, recover: false);
        AssertMagicEffect(records[99], "CAFF_MGEF_RW_MIST_MANA_DRAIN", "Magicka", hostile: true, recover: false);
        Assert.False(records[99].GetProperty("magicEffect").TryGetProperty("resistValue", out _));
        AssertMagicEffect(records[100], "CAFF_MGEF_CHAOS_RUNE_TRAP_SUNDER", "DamageResist", hostile: true, recover: true);
        Assert.Equal("CAFF_SPEL_CHAOS_RUNE_TRAP_SUNDER", records[101].GetProperty("spell").GetProperty("editorId").GetString());
        AssertMagicEffect(records[102], "CAFF_MGEF_CHAOS_RUNE_TRAP_FRAGILE", "ResistMagic", hostile: true, recover: true);
        Assert.Equal("CAFF_SPEL_CHAOS_RUNE_TRAP_FRAGILE", records[103].GetProperty("spell").GetProperty("editorId").GetString());
        AssertMagicEffect(records[104], "CAFF_MGEF_RW_OBSESSION_ARCANE", "Health", hostile: true, recover: false);
        Assert.Equal("CAFF_SPEL_RW_OBSESSION_ARCANE", records[105].GetProperty("spell").GetProperty("editorId").GetString());
        string[] riders = ["FIREBOLT", "ICE_SPIKE", "LIGHTNING_BOLT", "THUNDERBOLT", "ICY_SPEAR", "CHAIN_LIGHTNING", "ICE_STORM"];
        for (var index = 0; index < riders.Length; index++)
        {
            Assert.Equal($"CAFF_SPEL_COC_{riders[index]}_RIDER", records[106 + index].GetProperty("spell").GetProperty("editorId").GetString());
        }
    }

    [Fact]
    public void DynamicDamageSpells_DealDamageInstantlyAndCarryTheShaderSeparately()
    {
        // A 1s damage-over-time is replaced by the next cast of the same spell, losing
        // the rest of the earlier hit; the damage is instant and a separate 1s effect
        // keeps the elemental hit shader visible.
        var module = ReadJson("affixes", "modules", "keywords.affixes.core.json");
        foreach (var element in new[] { "FIRE", "FROST", "SHOCK" })
        {
            var spell = module.EnumerateArray()
                .Select(affix => affix.TryGetProperty("records", out var records) && records.TryGetProperty("spell", out var s) ? s : default)
                .Single(s => s.ValueKind == JsonValueKind.Object && s.GetProperty("editorId").GetString() == $"CAFF_SPEL_DMG_{element}_DYNAMIC");
            var effects = spell.GetProperty("effects").EnumerateArray().ToArray();
            Assert.Equal(2, effects.Length);
            AssertEffect(effects[0], $"CAFF_MGEF_DMG_{element}_DYNAMIC", 1.0, 0);
            AssertEffect(effects[1], $"CAFF_MGEF_DMG_{element}_DYNAMIC_HITFX", 0.0, 1);
        }
    }

    [Fact]
    public void SpawnTrapSpells_UseRuntimeHostileSelectionWithoutEngineAreaSplash()
    {
        var module = ReadJson(Path.Combine("affixes", "modules", "keywords.affixes.core.json"));
        var affixes = module.EnumerateArray().Select(affix => affix.Clone()).ToArray();
        var spells = affixes
            .Where(affix => affix.TryGetProperty("records", out var records) &&
                (records.TryGetProperty("spell", out _) || records.TryGetProperty("spells", out _)))
            .SelectMany(affix =>
            {
                var records = affix.GetProperty("records");
                if (records.TryGetProperty("spells", out var spellArray))
                {
                    return spellArray.EnumerateArray().Select(spell => spell.Clone());
                }
                return new[] { records.GetProperty("spell").Clone() };
            })
            .ToDictionary(spell => spell.GetProperty("editorId").GetString()!);

        var root = ReadJson(Path.Combine("affixes", "modules", "spec.root.json"));
        foreach (var record in root.GetProperty("keywords").GetProperty("appendedRecords").EnumerateArray())
        {
            if (record.GetProperty("type").GetString() == "Spell")
            {
                var spell = record.GetProperty("spell").Clone();
                spells[spell.GetProperty("editorId").GetString()!] = spell;
            }
        }

        var expectedTraps = new (string AffixId, double Radius)[]
        {
            ("rune_trap", 150.0),
            ("plague_spore", 150.0),
            ("tar_blight", 170.0),
            ("siphon_spore", 170.0),
            ("chaos_rune", 150.0),
        };

        foreach (var expected in expectedTraps)
        {
            var affix = affixes.Single(candidate => candidate.GetProperty("id").GetString() == expected.AffixId);
            var action = affix.GetProperty("runtime").GetProperty("action");
            Assert.Equal("SpawnTrap", action.GetProperty("type").GetString());
            Assert.Equal(expected.Radius, action.GetProperty("radius").GetDouble());
            Assert.Equal(2, action.GetProperty("maxTargetsPerTrigger").GetInt32());

            var spellEditorIds = new List<string>
            {
                action.GetProperty("spellEditorId").GetString()!,
            };
            if (action.TryGetProperty("extraSpells", out var extraSpells))
            {
                spellEditorIds.AddRange(extraSpells.EnumerateArray()
                    .Select(extra => extra.GetProperty("spellEditorId").GetString()!));
            }

            foreach (var spellEditorId in spellEditorIds)
            {
                Assert.True(spells.TryGetValue(spellEditorId, out var spell), $"Missing trap spell: {spellEditorId}");
                var effects = spell.TryGetProperty("effects", out var effectArray)
                    ? effectArray.EnumerateArray().ToArray()
                    : new[] { spell.GetProperty("effect") };
                Assert.All(effects, effect => Assert.Equal(0, effect.GetProperty("area").GetInt32()));
            }
        }
    }

    [Fact]
    public void Wisdom_RemainsAtTheDeferredBaseline()
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_wisdom_final");
        var records = affix.GetProperty("records");
        var runtime = affix.GetProperty("runtime");

        Assert.Equal("MagickaRate", records.GetProperty("magicEffect").GetProperty("actorValue").GetString());
        AssertEffect(records.GetProperty("spell").GetProperty("effect"), "CAFF_MGEF_RW_WISDOM_INSIGHT", 5.0, 7);
        Assert.Equal("IncomingHit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(18.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.Equal(11.0, runtime.GetProperty("icdSeconds").GetDouble());
        Assert.Equal("Self", runtime.GetProperty("action").GetProperty("applyTo").GetString());
    }

    [Fact]
    public void Ground_ShortensShoutRecoveryLikeVanillaTalos()
    {
        // ShoutRecoveryMult is a multiplier on 1: the old +50 made cooldowns tens
        // of times longer. Vanilla Talos uses a Peak Value Modifier of 0.2.
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_ground_final");
        var records = affix.GetProperty("records");
        var magicEffect = records.GetProperty("magicEffect");
        Assert.Equal("ShoutRecoveryMult", magicEffect.GetProperty("actorValue").GetString());
        Assert.Equal("PeakValueModifier", magicEffect.GetProperty("archetype").GetString());
        Assert.False(magicEffect.GetProperty("hostile").GetBoolean());
        AssertEffect(records.GetProperty("spell").GetProperty("effect"), "CAFF_MGEF_RW_GROUND_ANCHOR", 0.5, 8);
    }

    [Theory]
    [InlineData("keywords.affixes.suffixes.json", "suffix_eagle_eye_t1", 5)]
    [InlineData("keywords.affixes.suffixes.json", "suffix_eagle_eye_t2", 10)]
    [InlineData("keywords.affixes.suffixes.json", "suffix_eagle_eye_t3", 15)]
    [InlineData("keywords.affixes.runewords.json", "runeword_zephyr_final", 30)]
    public void BowSpeed_RidesOnANoOpCarrierForTheRuntime(string moduleFile, string id, double percent)
    {
        // BowSpeedBonus is Steady Hand's zoom slow-time. The bonus now rides on a
        // no-op effect that the SKSE runtime turns into WeaponSpeedMult while a
        // bow is drawn (BowDrawSpeed.h); its editor ID is how the runtime finds it.
        var affix = ReadAffix(moduleFile, id);
        var records = affix.GetProperty("records");
        var magicEffect = records.GetProperty("magicEffect");
        Assert.Equal("None", magicEffect.GetProperty("actorValue").GetString());
        Assert.Equal("Script", magicEffect.GetProperty("archetype").GetString());
        var effect = records.GetProperty("spell").GetProperty("effect");
        Assert.Equal(percent, effect.GetProperty("magnitude").GetDouble());
        var carrierSource = File.ReadAllText(Path.Combine(
            FindRepoRoot(), "skse", "CalamityAffixes", "src", "EventBridge.BowDrawSpeed.cpp"));
        Assert.Contains($"\"{magicEffect.GetProperty("editorId").GetString()}\"", carrierSource);
    }

    private static JsonElement ReadAffix(string moduleFile, string id)
    {
        var module = ReadJson(Path.Combine("affixes", "modules", moduleFile));
        return module.EnumerateArray().Single(candidate => candidate.GetProperty("id").GetString() == id).Clone();
    }

    // v2.3.0 stage 4: the "+skill for a few seconds" runewords now apply a shared
    // status (or pay off against two) and keep a smaller skill bonus always on.
    public static TheoryData<string, string?, string, double> SkillRunewordRedesign => new()
    {
        { "runeword_leaf_final", "Burning", "DestructionModifier", 10 },
        { "runeword_strength_final", "Exposed", "TwoHandedModifier", 10 },
        { "runeword_edge_final", "Doom", "MarksmanModifier", 10 },
        { "runeword_oath_final", "Bleed", "OneHandedModifier", 10 },
        { "runeword_passion_final", "Shock", "IllusionModifier", 10 },
        { "runeword_unbending_will_final", null, "BlockModifier", 15 },
        { "runeword_cure_final", "Burning", "RestorationModifier", 10 },
        { "runeword_hearth_final", "Freeze", "ResistFrost", 20 },
        { "runeword_enlightenment_final", "Exposed", "AlterationModifier", 10 },
        { "runeword_myth_final", "Bleed", "HeavyArmorModifier", 10 },
    };

    [Theory]
    [MemberData(nameof(SkillRunewordRedesign))]
    public void SkillRunewords_ApplyAStatusAndKeepAPassiveSkillBonus(string affixId, string? status, string passiveValue, double passiveMagnitude)
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", affixId);
        var action = affix.GetProperty("runtime").GetProperty("action");
        if (status == "Doom")
        {
            Assert.Equal("DoomMark", action.GetProperty("type").GetString());
        }
        else
        {
            Assert.Equal("CastSpell", action.GetProperty("type").GetString());
            Assert.Equal("Target", action.GetProperty("applyTo").GetString());
            if (status is null)
            {
                Assert.Equal(2, affix.GetProperty("runtime").GetProperty("requireTargetStatusCount").GetInt32());
            }
            else
            {
                Assert.Equal(status, action.GetProperty("statusTag").GetString());
            }
            var magicEffect = affix.GetProperty("records").GetProperty("magicEffect");
            Assert.True(magicEffect.GetProperty("hostile").GetBoolean(), affixId);
        }

        var passiveId = action.GetProperty("passiveSpellEditorId").GetString();
        var root = ReadJson(Path.Combine("affixes", "modules", "spec.root.json"));
        var tail = root.GetProperty("keywords").GetProperty("appendedRecords").EnumerateArray().ToArray();
        var records = affix.GetProperty("records");
        var passiveSpell = tail
            .Where(record => record.GetProperty("type").GetString() == "Spell")
            .Select(record => record.GetProperty("spell"))
            .Append(records.GetProperty("spell"))
            .Single(spell => spell.GetProperty("editorId").GetString() == passiveId);
        Assert.Equal("Ability", passiveSpell.GetProperty("spellType").GetString());
        var effect = passiveSpell.GetProperty("effect");
        Assert.Equal(passiveMagnitude, effect.GetProperty("magnitude").GetDouble());
        var effectId = effect.GetProperty("magicEffectEditorId").GetString();
        var passiveEffect = tail
            .Where(record => record.GetProperty("type").GetString() == "MagicEffect")
            .Select(record => record.GetProperty("magicEffect"))
            .Append(records.GetProperty("magicEffect"))
            .Single(magicEffect => magicEffect.GetProperty("editorId").GetString() == effectId);
        Assert.Equal(passiveValue, passiveEffect.GetProperty("actorValue").GetString());
        Assert.False(passiveEffect.GetProperty("hostile").GetBoolean());
    }

    [Fact]
    public void BuildUpTail_PayoffsAreHostileAndNamedAsTheRuntimeLooksThemUp()
    {
        var root = ReadJson(Path.Combine("affixes", "modules", "spec.root.json"));
        var records = root.GetProperty("keywords").GetProperty("appendedRecords").EnumerateArray().ToArray();
        var start = Array.FindIndex(records, record =>
            record.TryGetProperty("magicEffect", out var effect) &&
            effect.GetProperty("editorId").GetString() == "CAFF_MGEF_STATUS_FREEZE_SLOW");
        var tail = records[start..(start + 9)];

        var magicEffects = tail.Take(5).Select(record => record.GetProperty("magicEffect")).ToArray();
        Assert.Equal(
            new[] { "CAFF_MGEF_STATUS_FREEZE_SLOW", "CAFF_MGEF_STATUS_FREEZE_WEAKEN", "CAFF_MGEF_STATUS_BLEED_BURST", "CAFF_MGEF_STATUS_SHOCK_DISCHARGE", "CAFF_MGEF_STATUS_BURNING" },
            magicEffects.Select(effect => effect.GetProperty("editorId").GetString()));
        Assert.All(magicEffects, effect => Assert.True(effect.GetProperty("hostile").GetBoolean()));

        var spells = tail.Skip(5).Select(record => record.GetProperty("spell")).ToArray();
        Assert.All(spells, spell => Assert.Equal("TargetActor", spell.GetProperty("delivery").GetString()));
        var freeze = spells[0].GetProperty("effects").EnumerateArray().ToArray();
        Assert.Equal(80.0, freeze[0].GetProperty("magnitude").GetDouble());
        Assert.Equal(0.3, freeze[1].GetProperty("magnitude").GetDouble());
        Assert.All(freeze, effect => Assert.Equal(3, effect.GetProperty("duration").GetInt32()));
        // Burning lasts as long as the runtime keeps its stacks (kBurningDurationMs).
        Assert.Equal(4, spells[3].GetProperty("effect").GetProperty("duration").GetInt32());

        var runtime = File.ReadAllText(Path.Combine(FindRepoRoot(), "skse", "CalamityAffixes", "src", "EventBridge.Actions.Status.cpp"));
        foreach (var spell in spells)
        {
            Assert.Contains($"\"{spell.GetProperty("editorId").GetString()}\"", runtime);
        }
        var meters = File.ReadAllText(Path.Combine(FindRepoRoot(), "skse", "CalamityAffixes", "include", "CalamityAffixes", "StatusMeters.h"));
        Assert.Contains("kBurningDurationMs = 4000", meters);
    }

    private static JsonElement ReadJson(params string[] relativePath)
    {
        var path = Path.Combine(new[] { FindRepoRoot() }.Concat(relativePath).ToArray());
        using var document = JsonDocument.Parse(File.ReadAllText(path));
        return document.RootElement.Clone();
    }

    private static void AssertMagicEffect(
        JsonElement taggedRecord,
        string editorId,
        string actorValue,
        bool hostile,
        bool recover)
    {
        Assert.Equal("MagicEffect", taggedRecord.GetProperty("type").GetString());
        var magicEffect = taggedRecord.GetProperty("magicEffect");
        Assert.Equal(editorId, magicEffect.GetProperty("editorId").GetString());
        Assert.Equal(actorValue, magicEffect.GetProperty("actorValue").GetString());
        Assert.Equal(hostile, magicEffect.GetProperty("hostile").GetBoolean());
        Assert.Equal(recover, magicEffect.GetProperty("recover").GetBoolean());
    }

    private static void AssertSpellEffects(
        JsonElement taggedRecord,
        string editorId,
        params (string MagicEffectEditorId, double Magnitude, int Duration)[] expected)
    {
        Assert.Equal("Spell", taggedRecord.GetProperty("type").GetString());
        var spell = taggedRecord.GetProperty("spell");
        Assert.Equal(editorId, spell.GetProperty("editorId").GetString());
        Assert.Equal("Self", spell.GetProperty("delivery").GetString());
        var effects = spell.TryGetProperty("effects", out var effectArray)
            ? effectArray.EnumerateArray().ToArray()
            : new[] { spell.GetProperty("effect") };
        Assert.Equal(expected.Length, effects.Length);
        for (var index = 0; index < expected.Length; index++)
        {
            AssertEffect(effects[index], expected[index].MagicEffectEditorId, expected[index].Magnitude, expected[index].Duration);
        }
    }

    private static void AssertEffect(JsonElement effect, string magicEffectEditorId, double magnitude, int duration)
    {
        Assert.Equal(magicEffectEditorId, effect.GetProperty("magicEffectEditorId").GetString());
        Assert.Equal(magnitude, effect.GetProperty("magnitude").GetDouble(), precision: 6);
        Assert.Equal(duration, effect.GetProperty("duration").GetInt32());
        Assert.Equal(0, effect.GetProperty("area").GetInt32());
    }

    private static string FindRepoRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "affixes", "affixes.modules.json")))
            {
                return directory.FullName;
            }
            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate repository root.");
    }
}
