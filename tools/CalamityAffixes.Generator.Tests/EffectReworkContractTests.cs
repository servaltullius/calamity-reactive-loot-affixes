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
    public void Fury_UsesAttackSpeedAndInstantStaminaWithoutCriticalChance()
    {
        var affix = ReadAffix("keywords.affixes.runewords.json", "runeword_fury_final");
        var records = affix.GetProperty("records");
        var runtime = affix.GetProperty("runtime");

        Assert.Equal("CAFF_MGEF_RW_FURY_LIFESTEAL", records.GetProperty("magicEffect").GetProperty("editorId").GetString());
        Assert.Equal("WeaponSpeedMult", records.GetProperty("magicEffect").GetProperty("actorValue").GetString());
        var effects = records.GetProperty("spell").GetProperty("effects").EnumerateArray().ToArray();
        Assert.Equal(2, effects.Length);
        AssertEffect(effects[0], "CAFF_MGEF_RW_FURY_LIFESTEAL", 0.25, 6);
        AssertEffect(effects[1], "CAFF_MGEF_RW_FURY_RESTORE_STAMINA", 30.0, 0);
        Assert.Equal("Hit", runtime.GetProperty("trigger").GetString());
        Assert.Equal(24.0, runtime.GetProperty("procChancePercent").GetDouble());
        Assert.Equal(12.0, runtime.GetProperty("icdSeconds").GetDouble());
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

        Assert.Equal(35, records.Length);
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

    private static JsonElement ReadAffix(string moduleFile, string id)
    {
        var module = ReadJson(Path.Combine("affixes", "modules", moduleFile));
        return module.EnumerateArray().Single(candidate => candidate.GetProperty("id").GetString() == id).Clone();
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
