using System.Text.Json;
using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Tests;

public sealed class HostileOnlyExternalSpellTests
{
    private sealed record EffectExpectation(uint FormId, float Magnitude, int Duration, int Area);
    private sealed record SpellExpectation(
        string EditorId,
        TargetType TargetType,
        float ChargeTime,
        float Range,
        EffectExpectation[] Effects);

    private static readonly SpellExpectation[] ExpectedSpells =
    [
        new("CAFF_SPEL_COC_FIREBOLT_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x00012F03u, 25.0f, 0, 0), new(0x000153D3u, 0.25f, 0, 0),
            new(0x000F392Du, 99.0f, 15, 0),
        ]),
        new("CAFF_SPEL_COC_ICE_SPIKE_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x0001CEA2u, 25.0f, 0, 0), new(0x000153D3u, 0.25f, 0, 0),
            new(0x000B729Fu, 50.0f, 3, 0), new(0x000F3932u, 0.0f, 3, 0),
        ]),
        new("CAFF_SPEL_COC_LIGHTNING_BOLT_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x0001CEA8u, 25.0f, 0, 0), new(0x000153D3u, 0.25f, 0, 0),
            new(0x000F3F0Du, 200.0f, 1, 0),
        ]),
        new("CAFF_SPEL_COC_THUNDERBOLT_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x0010F7EFu, 60.0f, 0, 0), new(0x000153D3u, 0.25f, 0, 0),
            new(0x000F3F0Du, 200.0f, 1, 0),
        ]),
        new("CAFF_SPEL_COC_ICY_SPEAR_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x0010F7F0u, 60.0f, 0, 0), new(0x000153D3u, 0.25f, 0, 0),
            new(0x000B729Fu, 50.0f, 3, 0), new(0x000F3932u, 0.0f, 3, 0),
        ]),
        new("CAFF_SPEL_COC_CHAIN_LIGHTNING_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x0005DBAEu, 40.0f, 0, 20), new(0x000153D3u, 0.25f, 0, 0),
            new(0x000F3F0Fu, 200.0f, 1, 20),
        ]),
        new("CAFF_SPEL_COC_ICE_STORM_HOSTILE_ONLY", TargetType.Aimed, 0.5f, 0.0f,
        [
            new(0x0001CEA3u, 40.0f, 0, 15), new(0x000B729Fu, 50.0f, 5, 0),
            new(0x000F3935u, 0.0f, 3, 15), new(0x000153D3u, 0.25f, 0, 0),
        ]),
        new("CAFF_SPEL_TRAP_BANISH_DAEDRA_HOSTILE_ONLY", TargetType.TargetActor, 0.5f, 50.0f,
        [
            new(0x0006D22Bu, 15.0f, 0, 0), new(0x0006D22Du, 100.0f, 0, 0),
            new(0x000173DCu, 20.0f, 1, 0),
        ]),
        new("CAFF_SPEL_TRAP_TURN_UNDEAD_HOSTILE_ONLY", TargetType.Aimed, 0.0f, 0.0f,
        [
            new(0x0004B145u, 6.0f, 30, 0), new(0x0004D3F9u, 50.0f, 0, 0),
        ]),
    ];

    private static readonly Dictionary<string, string> ExpectedCastOnCritActions = new(StringComparer.Ordinal)
    {
        ["crit_cast_firebolt"] = "CAFF_SPEL_COC_FIREBOLT_HOSTILE_ONLY",
        ["crit_cast_ice_spike"] = "CAFF_SPEL_COC_ICE_SPIKE_HOSTILE_ONLY",
        ["crit_cast_lightning_bolt"] = "CAFF_SPEL_COC_LIGHTNING_BOLT_HOSTILE_ONLY",
        ["crit_cast_thunderbolt"] = "CAFF_SPEL_COC_THUNDERBOLT_HOSTILE_ONLY",
        ["crit_cast_icy_spear"] = "CAFF_SPEL_COC_ICY_SPEAR_HOSTILE_ONLY",
        ["crit_cast_chain_lightning"] = "CAFF_SPEL_COC_CHAIN_LIGHTNING_HOSTILE_ONLY",
        ["crit_cast_ice_storm"] = "CAFF_SPEL_COC_ICE_STORM_HOSTILE_ONLY",
    };

    [Fact]
    public void RepoSpec_HostileOnlyExternalSpellClonesPreserveEffectsAndGateEveryRecipient()
    {
        var spec = AffixSpecLoader.Load(Path.Combine(FindRepoRoot(), "affixes", "affixes.json"));
        var mod = KeywordPluginBuilder.Build(spec);

        AssertHostileOnlySpells(mod.Spells);
    }

    [Fact]
    public void GeneratedDataEsp_HostileOnlyExternalSpellClonesPreserveEffectsAndGateEveryRecipient()
    {
        using var mod = SkyrimMod.CreateFromBinaryOverlay(
            Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp"),
            SkyrimRelease.SkyrimSE);

        AssertHostileOnlySpells(mod.Spells);
    }

    [Fact]
    public void RepoSpec_CastOnCritAndTrapActionsUseOnlyHostileOnlyClones()
    {
        var spec = AffixSpecLoader.Load(Path.Combine(FindRepoRoot(), "affixes", "affixes.json"));

        foreach (var expected in ExpectedCastOnCritActions)
        {
            var affix = Assert.Single(spec.Keywords.Affixes, candidate => candidate.Id == expected.Key);
            var action = JsonSerializer.SerializeToElement(affix.Runtime.Action);
            Assert.Equal(expected.Value, action.GetProperty("spellEditorId").GetString());
            Assert.False(action.TryGetProperty("spellForm", out _));
        }

        var bearTrap = Assert.Single(spec.Keywords.Affixes, candidate => candidate.Id == "bear_trap");
        var bearTrapAction = JsonSerializer.SerializeToElement(bearTrap.Runtime.Action);
        var externalSpells = bearTrapAction.GetProperty("extraSpells").EnumerateArray()
            .Where(entry => entry.TryGetProperty("spellEditorId", out var editorId) &&
                            editorId.GetString()?.Contains("HOSTILE_ONLY", StringComparison.Ordinal) == true)
            .ToArray();
        Assert.Equal(2, externalSpells.Length);
        Assert.Contains(externalSpells, entry =>
            entry.GetProperty("spellEditorId").GetString() == "CAFF_SPEL_TRAP_BANISH_DAEDRA_HOSTILE_ONLY");
        Assert.Contains(externalSpells, entry =>
            entry.GetProperty("spellEditorId").GetString() == "CAFF_SPEL_TRAP_TURN_UNDEAD_HOSTILE_ONLY");
        Assert.DoesNotContain(bearTrapAction.GetProperty("extraSpells").EnumerateArray(),
            entry => entry.TryGetProperty("spellForm", out var form) &&
                     form.GetString() is "Skyrim.esm|0006D22C" or "Skyrim.esm|0004B146");
    }

    private static void AssertHostileOnlySpells(IEnumerable<ISpellGetter> spells)
    {
        var spellArray = spells.ToArray();
        var skyrim = ModKey.FromNameAndExtension("Skyrim.esm");
        var playerRef = new FormKey(skyrim, 0x000014);
        var eitherHand = new FormKey(skyrim, 0x00013F44);

        for (var spellIndex = 0; spellIndex < ExpectedSpells.Length; spellIndex++)
        {
            var expected = ExpectedSpells[spellIndex];
            var spell = Assert.Single(spellArray, candidate => candidate.EditorID == expected.EditorId);
            Assert.Equal(0x000B06u + (uint)spellIndex, spell.FormKey.ID);
            Assert.Equal(SpellType.Spell, spell.Type);
            Assert.Equal(CastType.FireAndForget, spell.CastType);
            Assert.Equal(expected.TargetType, spell.TargetType);
            Assert.Equal(expected.ChargeTime, spell.ChargeTime);
            Assert.Equal(expected.Range, spell.Range);
            Assert.Equal(eitherHand, spell.EquipmentType.FormKey);
            Assert.Equal(expected.Effects.Length, spell.Effects.Count);

            for (var index = 0; index < expected.Effects.Length; index++)
            {
                var expectedEffect = expected.Effects[index];
                var effect = spell.Effects[index];
                Assert.Equal(new FormKey(skyrim, expectedEffect.FormId), effect.BaseEffect.FormKey);
                Assert.NotNull(effect.Data);
                Assert.Equal(expectedEffect.Magnitude, effect.Data!.Magnitude);
                Assert.Equal(expectedEffect.Duration, effect.Data.Duration);
                Assert.Equal(expectedEffect.Area, effect.Data.Area);

                Assert.Equal(2, effect.Conditions.Count);
                var condition = Assert.IsAssignableFrom<IConditionFloatGetter>(effect.Conditions[0]);
                Assert.Equal(CompareOperator.EqualTo, condition.CompareOperator);
                Assert.Equal(1.0f, condition.ComparisonValue);
                var hostile = Assert.IsAssignableFrom<IIsHostileToActorConditionDataGetter>(condition.Data);
                Assert.Equal(Condition.RunOnType.Subject, hostile.RunOnType);
                Assert.Equal(playerRef, hostile.TargetNpc.Link.FormKeyNullable);
                var teammateCondition = Assert.IsAssignableFrom<IConditionFloatGetter>(effect.Conditions[1]);
                Assert.Equal(CompareOperator.EqualTo, teammateCondition.CompareOperator);
                Assert.Equal(0.0f, teammateCondition.ComparisonValue);
                var teammate = Assert.IsAssignableFrom<IGetPlayerTeammateConditionDataGetter>(teammateCondition.Data);
                Assert.Equal(Condition.RunOnType.Subject, teammate.RunOnType);
            }
        }
    }

    private static string FindRepoRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            if (File.Exists(Path.Combine(current.FullName, "affixes", "affixes.json")))
            {
                return current.FullName;
            }
            current = current.Parent;
        }
        throw new DirectoryNotFoundException("Repository root not found.");
    }
}
