using System.Globalization;
using System.Text.Json;
using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;

namespace CalamityAffixes.Generator.Tests;

public sealed class RepoSpecRegressionTests
{
    [Fact]
    public void RepoSpec_GeneratorBuildsWithoutForwardMagicEffectReferences()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");
        var spec = AffixSpecLoader.Load(specPath);

        AffixSpecLoader.ValidateRunewordContractReferences(spec);
        var mod = KeywordPluginBuilder.Build(spec);

        Assert.NotNull(mod);
        foreach (var editorId in new[]
                 {
                     "CAFF_MGEF_RW_COH_PASSIVE_FIRE",
                     "CAFF_MGEF_RW_COH_PASSIVE_FROST",
                     "CAFF_MGEF_RW_COH_PASSIVE_SHOCK",
                 })
        {
            var effect = Assert.Single(mod.MagicEffects, record => record.EditorID == editorId);
            Assert.Equal(CastType.ConstantEffect, effect.CastType);
            Assert.Equal(TargetType.Self, effect.TargetType);
        }

        foreach (var editorId in new[]
                 {
                     "CAFF_MGEF_RW_DRAGON_RESIST_FIRE",
                     "CAFF_MGEF_RW_DRAGON_RESIST_FROST",
                     "CAFF_MGEF_RW_DRAGON_RESIST_SHOCK",
                 })
        {
            var effect = Assert.Single(mod.MagicEffects, record => record.EditorID == editorId);
            Assert.Equal(CastType.FireAndForget, effect.CastType);
            Assert.Equal(TargetType.Self, effect.TargetType);
        }
    }

    [Fact]
    public void RepoSpec_GeneratedEffectRecordsMatchApprovedSemantics()
    {
        var repoRoot = FindRepoRoot();
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));
        var mod = KeywordPluginBuilder.Build(spec);

        MagicEffect MagicEffectById(string editorId) =>
            Assert.Single(mod.MagicEffects, record => record.EditorID == editorId);
        Spell SpellById(string editorId) =>
            Assert.Single(mod.Spells, record => record.EditorID == editorId);
        string EffectEditorId(Effect effect) =>
            mod.MagicEffects.Single(record => record.FormKey == effect.BaseEffect.FormKey).EditorID!;
        void AssertActorValue(MagicEffect magicEffect, ActorValue actorValue)
        {
            var archetype = Assert.IsType<MagicEffectArchetype>(magicEffect.Archetype);
            Assert.Equal(MagicEffectArchetype.TypeEnum.ValueModifier, archetype.Type);
            Assert.Equal(actorValue, archetype.ActorValue);
        }
        void AssertSpellEffect(Effect effect, string magicEffectEditorId, float magnitude, int duration)
        {
            Assert.Equal(magicEffectEditorId, EffectEditorId(effect));
            Assert.NotNull(effect.Data);
            Assert.Equal(magnitude, effect.Data!.Magnitude);
            Assert.Equal(duration, effect.Data.Duration);
            Assert.Equal(0, effect.Data.Area);
        }

        var voiceSpell = SpellById("CAFF_SPEL_INCOMING_VOICE_POWER");
        Assert.Equal(SpellType.Spell, voiceSpell.Type);
        Assert.Equal(CastType.FireAndForget, voiceSpell.CastType);
        Assert.Equal(TargetType.Self, voiceSpell.TargetType);
        Assert.Equal(2, voiceSpell.Effects.Count);
        AssertSpellEffect(voiceSpell.Effects[0], "CAFF_MGEF_INCOMING_VOICE_POWER_STAMINA", 30.0f, 0);
        AssertSpellEffect(voiceSpell.Effects[1], "CAFF_MGEF_INCOMING_VOICE_POWER_ATTACK_DAMAGE", 0.15f, 5);

        var spiritOldEffect = MagicEffectById("CAFF_MGEF_RW_SPIRIT_MEDITATION");
        AssertActorValue(spiritOldEffect, ActorValue.MagickaRateMult);
        var spiritOldSpell = SpellById("CAFF_SPEL_RW_SPIRIT_MEDITATION");
        AssertSpellEffect(Assert.Single(spiritOldSpell.Effects), "CAFF_MGEF_RW_SPIRIT_MEDITATION", 120.0f, 10);
        var spiritPassiveEffect = MagicEffectById("CAFF_MGEF_RW_SPIRIT_PASSIVE_MPREGEN");
        AssertActorValue(spiritPassiveEffect, ActorValue.Magicka);
        var spiritPassiveSpell = SpellById("CAFF_SPEL_RW_SPIRIT_PASSIVE");
        Assert.Equal(SpellType.Ability, spiritPassiveSpell.Type);
        Assert.Equal(CastType.ConstantEffect, spiritPassiveSpell.CastType);
        AssertSpellEffect(Assert.Single(spiritPassiveSpell.Effects), "CAFF_MGEF_RW_SPIRIT_PASSIVE_MPREGEN", 30.0f, 0);
        var spiritAbsorbEffect = MagicEffectById("CAFF_MGEF_RW_SPIRIT_ABSORB_CHANCE");
        AssertActorValue(spiritAbsorbEffect, ActorValue.AbsorbChance);
        Assert.True(spiritAbsorbEffect.Flags.HasFlag(MagicEffect.Flag.Recover));
        Assert.False(spiritAbsorbEffect.Flags.HasFlag(MagicEffect.Flag.Hostile));
        var spiritAbsorbSpell = SpellById("CAFF_SPEL_RW_SPIRIT_ABSORPTION");
        AssertSpellEffect(Assert.Single(spiritAbsorbSpell.Effects), "CAFF_MGEF_RW_SPIRIT_ABSORB_CHANCE", 10.0f, 5);

        var smokeEffect = MagicEffectById("CAFF_MGEF_RW_SMOKE_SCREEN");
        AssertActorValue(smokeEffect, ActorValue.SpeedMult);
        Assert.True(smokeEffect.Flags.HasFlag(MagicEffect.Flag.Hostile));
        Assert.True(smokeEffect.Flags.HasFlag(MagicEffect.Flag.Detrimental));
        Assert.True(smokeEffect.Flags.HasFlag(MagicEffect.Flag.Recover));
        var smokeSpell = SpellById("CAFF_SPEL_RW_SMOKE_SCREEN");
        Assert.Equal(TargetType.TargetActor, smokeSpell.TargetType);
        AssertSpellEffect(Assert.Single(smokeSpell.Effects), "CAFF_MGEF_RW_SMOKE_SCREEN", 30.0f, 5);

        var furyEffect = MagicEffectById("CAFF_MGEF_RW_FURY_LIFESTEAL");
        AssertActorValue(furyEffect, ActorValue.WeaponSpeedMult);
        var furySpell = SpellById("CAFF_SPEL_RW_FURY_LIFESTEAL");
        Assert.Equal(2, furySpell.Effects.Count);
        AssertSpellEffect(furySpell.Effects[0], "CAFF_MGEF_RW_FURY_LIFESTEAL", 0.25f, 6);
        AssertSpellEffect(furySpell.Effects[1], "CAFF_MGEF_RW_FURY_RESTORE_STAMINA", 30.0f, 0);

        var wealthOldEffect = MagicEffectById("CAFF_MGEF_RW_WEALTH_VIGOR");
        AssertActorValue(wealthOldEffect, ActorValue.CarryWeight);
        var wealthOldSpell = SpellById("CAFF_SPEL_RW_WEALTH_VIGOR");
        AssertSpellEffect(Assert.Single(wealthOldSpell.Effects), "CAFF_MGEF_RW_WEALTH_VIGOR", 100.0f, 15);
        var wealthPassiveSpell = SpellById("CAFF_SPEL_RW_WEALTH_PASSIVE");
        Assert.Equal(SpellType.Ability, wealthPassiveSpell.Type);
        Assert.Equal(CastType.ConstantEffect, wealthPassiveSpell.CastType);
        Assert.Equal(TargetType.Self, wealthPassiveSpell.TargetType);
        Assert.Equal(2, wealthPassiveSpell.Effects.Count);
        AssertSpellEffect(wealthPassiveSpell.Effects[0], "CAFF_MGEF_RW_WEALTH_PASSIVE_CARRY_WEIGHT", 75.0f, 0);
        AssertSpellEffect(wealthPassiveSpell.Effects[1], "CAFF_MGEF_RW_WEALTH_PASSIVE_SPEECHCRAFT", 15.0f, 0);
    }

    [Fact]
    public void RepoSpec_DisablesGenericSpidCurrencyDistribution()
    {
        var repoRoot = FindRepoRoot();
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));

        Assert.Empty(spec.Keywords.SpidRules);

        var distributionPath = Path.Combine(repoRoot, "Data", "CalamityAffixes_DISTR.ini");
        var distribution = File.ReadAllText(distributionPath);
        Assert.DoesNotContain("Item =", distribution, StringComparison.OrdinalIgnoreCase);
        Assert.DoesNotContain("DeathItem =", distribution, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void FrozenV140AllocationFixture_IsSealedAndSelfConsistent()
    {
        var repoRoot = FindRepoRoot();
        var fixture = ReadV140AllocationFixture(repoRoot);

        Assert.Equal(1, fixture.FixtureVersion);
        Assert.Equal("Data/CalamityAffixes.esp", fixture.SourcePlugin);
        Assert.Equal("v1.4.0", fixture.ReleaseTag);
        Assert.Equal("c5845765e7af5d13804476785db0204c08e2397e", fixture.Commit);
        Assert.Equal("Mutagen.Bethesda.Skyrim", fixture.GeneratorPackage);
        Assert.Equal("0.52.0", fixture.GeneratorVersion);
        Assert.Equal("85e62c696c2635a6b0adaf4402df44e5593569bd217da706a8588b4a48b4a655", fixture.SourceEspSha256);
        Assert.Equal(733, fixture.MajorRecordCount);
        Assert.Equal(733, fixture.Records.Length);
        Assert.Equal(0x000ADDu, fixture.NextObjectId);
        Assert.Equal(new AllocationRecord(0x000800u, "QUST", "CalamityAffixes_MCM"), fixture.Records[0]);
        Assert.Equal(
            new AllocationRecord(0x000ADCu, "MGEF", "CAFF_MGEF_RW_DRAGON_RESIST_SHOCK"),
            fixture.Records[^1]);
        Assert.Equal(
            Enumerable.Range(0x000800, fixture.Records.Length).Select(value => (uint)value),
            fixture.Records.Select(record => record.FormId));
        Assert.Equal(fixture.Records.Length, fixture.Records.Select(record => record.FormId).Distinct().Count());
        Assert.Equal(
            fixture.Records.Length,
            fixture.Records.Select(record => record.EditorId).Distinct(StringComparer.OrdinalIgnoreCase).Count());
    }

    [Fact]
    public void RepoSpec_PreservesFrozenV140PrefixAndAppendsThirtyFiveTypedRecords()
    {
        var repoRoot = FindRepoRoot();
        var fixture = ReadV140AllocationFixture(repoRoot);
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));
        var mod = KeywordPluginBuilder.Build(spec);
        var actual = AllocationSignature(mod);

        Assert.Equal(768, actual.Length);
        Assert.Equal(fixture.Records, actual.Take(fixture.Records.Length));
        Assert.Equal(
            new[]
            {
                new AllocationRecord(0x000ADDu, "MGEF", "CAFF_MGEF_INCOMING_VOICE_POWER_STAMINA"),
                new AllocationRecord(0x000ADEu, "MGEF", "CAFF_MGEF_INCOMING_VOICE_POWER_ATTACK_DAMAGE"),
                new AllocationRecord(0x000ADFu, "SPEL", "CAFF_SPEL_INCOMING_VOICE_POWER"),
                new AllocationRecord(0x000AE0u, "MGEF", "CAFF_MGEF_RW_SPIRIT_ABSORB_CHANCE"),
                new AllocationRecord(0x000AE1u, "SPEL", "CAFF_SPEL_RW_SPIRIT_ABSORPTION"),
                new AllocationRecord(0x000AE2u, "MGEF", "CAFF_MGEF_RW_FURY_RESTORE_STAMINA"),
                new AllocationRecord(0x000AE3u, "MGEF", "CAFF_MGEF_RW_WEALTH_PASSIVE_CARRY_WEIGHT"),
                new AllocationRecord(0x000AE4u, "MGEF", "CAFF_MGEF_RW_WEALTH_PASSIVE_SPEECHCRAFT"),
                new AllocationRecord(0x000AE5u, "SPEL", "CAFF_SPEL_RW_WEALTH_PASSIVE"),
                new AllocationRecord(0x000AE6u, "ARTO", "CAFF_ARTO_VFX_VOICE_OF_POWER"),
                new AllocationRecord(0x000AE7u, "ARTO", "CAFF_ARTO_VFX_SPIRIT_ABSORB"),
                new AllocationRecord(0x000AE8u, "ARTO", "CAFF_ARTO_VFX_SMOKE_SLOW"),
                new AllocationRecord(0x000AE9u, "ARTO", "CAFF_ARTO_VFX_FURY_SURGE"),
                new AllocationRecord(0x000AEAu, "ARTO", "CAFF_ARTO_VFX_WEALTH_PASSIVE"),
            },
            actual.Skip(fixture.Records.Length).Take(14));
        Assert.True(mod.ModHeader.Flags.HasFlag(SkyrimModHeader.HeaderFlag.Small));
        Assert.Equal(0x000B00u, ((IModGetter)mod).NextFormID);
        Assert.Equal(actual.Length, actual.Select(record => record.FormId).Distinct().Count());
        Assert.Equal(
            actual.Length,
            actual.Select(record => record.EditorId).Distinct(StringComparer.OrdinalIgnoreCase).Count());
    }

    [Fact]
    public void RepoSpec_ExportReimportPreservesFrozenPrefixAndTypedTail()
    {
        var repoRoot = FindRepoRoot();
        var fixture = ReadV140AllocationFixture(repoRoot);
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));
        var generated = KeywordPluginBuilder.Build(spec);
        var generatedAllocation = AllocationSignature(generated);
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        var pluginPath = Path.Combine(tempRoot, spec.ModKey);
        Directory.CreateDirectory(tempRoot);

        try
        {
            ((IModGetter)generated).WriteToBinary(new FilePath(pluginPath));
            using var reimported = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
            var reimportedAllocation = AllocationSignature(reimported);

            Assert.Equal(768, generatedAllocation.Length);
            Assert.Equal(generatedAllocation, reimportedAllocation);
            Assert.Equal(fixture.Records, reimportedAllocation.Take(fixture.Records.Length));
            Assert.Equal(generatedAllocation.TakeLast(35), reimportedAllocation.TakeLast(35));
            Assert.True(reimported.ModHeader.Flags.HasFlag(SkyrimModHeader.HeaderFlag.Small));
            Assert.Equal(0x000B00u, reimported.NextFormID);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void GeneratedDataEsp_PreservesFrozenPrefixAndTypedTail()
    {
        var repoRoot = FindRepoRoot();
        var fixture = ReadV140AllocationFixture(repoRoot);
        var pluginPath = Path.Combine(repoRoot, "Data", "CalamityAffixes.esp");
        using var mod = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
        var actual = AllocationSignature(mod);

        Assert.Equal(768, actual.Length);
        Assert.Equal(fixture.Records, actual.Take(fixture.Records.Length));
        Assert.Equal(
            new[]
            {
                new AllocationRecord(0x000ADDu, "MGEF", "CAFF_MGEF_INCOMING_VOICE_POWER_STAMINA"),
                new AllocationRecord(0x000ADEu, "MGEF", "CAFF_MGEF_INCOMING_VOICE_POWER_ATTACK_DAMAGE"),
                new AllocationRecord(0x000ADFu, "SPEL", "CAFF_SPEL_INCOMING_VOICE_POWER"),
                new AllocationRecord(0x000AE0u, "MGEF", "CAFF_MGEF_RW_SPIRIT_ABSORB_CHANCE"),
                new AllocationRecord(0x000AE1u, "SPEL", "CAFF_SPEL_RW_SPIRIT_ABSORPTION"),
                new AllocationRecord(0x000AE2u, "MGEF", "CAFF_MGEF_RW_FURY_RESTORE_STAMINA"),
                new AllocationRecord(0x000AE3u, "MGEF", "CAFF_MGEF_RW_WEALTH_PASSIVE_CARRY_WEIGHT"),
                new AllocationRecord(0x000AE4u, "MGEF", "CAFF_MGEF_RW_WEALTH_PASSIVE_SPEECHCRAFT"),
                new AllocationRecord(0x000AE5u, "SPEL", "CAFF_SPEL_RW_WEALTH_PASSIVE"),
                new AllocationRecord(0x000AE6u, "ARTO", "CAFF_ARTO_VFX_VOICE_OF_POWER"),
                new AllocationRecord(0x000AE7u, "ARTO", "CAFF_ARTO_VFX_SPIRIT_ABSORB"),
                new AllocationRecord(0x000AE8u, "ARTO", "CAFF_ARTO_VFX_SMOKE_SLOW"),
                new AllocationRecord(0x000AE9u, "ARTO", "CAFF_ARTO_VFX_FURY_SURGE"),
                new AllocationRecord(0x000AEAu, "ARTO", "CAFF_ARTO_VFX_WEALTH_PASSIVE"),
            },
            actual.Skip(fixture.Records.Length).Take(14));
        Assert.True(mod.ModHeader.Flags.HasFlag(SkyrimModHeader.HeaderFlag.Small));
        Assert.Equal(0x000B00u, mod.NextFormID);
        Assert.Equal(actual.Length, actual.Select(record => record.FormId).Distinct().Count());
        Assert.Equal(
            actual.Length,
            actual.Select(record => record.EditorId).Distinct(StringComparer.OrdinalIgnoreCase).Count());
    }

    [Fact]
    public void RepoSpec_LegacyDragonAppendArrayIsSealedToThreeRecords()
    {
        var repoRoot = FindRepoRoot();
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));

        Assert.Equal(3, spec.Keywords.AppendedMagicEffects.Count);
        Assert.Equal(
            new[]
            {
                "CAFF_MGEF_RW_DRAGON_RESIST_FIRE",
                "CAFF_MGEF_RW_DRAGON_RESIST_FROST",
                "CAFF_MGEF_RW_DRAGON_RESIST_SHOCK",
            },
            spec.Keywords.AppendedMagicEffects.Select(effect => effect.EditorId));
    }

    [Fact]
    public void RepoSpec_IncludesCorpseExplosionKillAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasCorpseExplosionKill = spec.Keywords.Affixes.Any(affix =>
        {
            if (!TryGetRuntimeString(affix.Runtime, "trigger", out var trigger) || trigger != "Kill")
            {
                return false;
            }

            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("type", out var typeEl) || typeEl.ValueKind != JsonValueKind.String)
            {
                return false;
            }

            return typeEl.GetString() == "CorpseExplosion";
        });

        Assert.True(
            hasCorpseExplosionKill,
            "Expected at least one affix with runtime.trigger == 'Kill' and runtime.action.type == 'CorpseExplosion' in affixes/affixes.json");
    }

    [Fact]
    public void RepoSpec_IncludesCastOnCritAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var castOnCritCount = spec.Keywords.Affixes.Count(affix =>
        {
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("type", out var typeEl) || typeEl.ValueKind != JsonValueKind.String)
            {
                return false;
            }

            return typeEl.GetString() == "CastOnCrit";
        });

        Assert.True(
            castOnCritCount >= 7,
            $"Expected at least 7 affixes with runtime.action.type == 'CastOnCrit' in affixes/affixes.json; got {castOnCritCount}.");
    }

    [Fact]
    public void RepoSpec_CastOnCritVanillaSpells_SuppressHitEffectArt()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var offenders = spec.Keywords.Affixes
            .Where(affix =>
            {
                if (!TryGetActionType(affix.Runtime, out var actionType) || actionType != "CastOnCrit")
                {
                    return false;
                }

                var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

                if (actionEl.ValueKind != JsonValueKind.Object)
                {
                    return false;
                }

                if (!actionEl.TryGetProperty("spellForm", out var spellFormEl) || spellFormEl.ValueKind != JsonValueKind.String)
                {
                    return false;
                }

                var spellForm = spellFormEl.GetString();
                if (spellForm is null || !spellForm.StartsWith("Skyrim.esm|", StringComparison.Ordinal))
                {
                    return false;
                }

                if (!actionEl.TryGetProperty("noHitEffectArt", out var noHitEffectArtEl))
                {
                    return true;
                }

                return noHitEffectArtEl.ValueKind != JsonValueKind.True;
            })
            .Select(affix => affix.Id)
            .ToList();

        Assert.True(
            offenders.Count == 0,
            "Expected CastOnCrit vanilla spell procs to set runtime.action.noHitEffectArt == true " +
            $"to avoid lingering hit effect art; offenders: {string.Join(", ", offenders)}");
    }

    [Fact]
    public void RuntimeHook_CastOnCrit_DoesNotInjectFeedbackHitShader()
    {
        var hooksSource = ReadHooksRuntimeSource();
        Assert.DoesNotContain(
            "PlayCastOnCritProcFeedbackVfx(safeTarget, coc.spell);",
            hooksSource,
            StringComparison.Ordinal);
    }

    [Fact]
    public void RuntimeHook_CastOnCrit_InjectsFeedbackSfxWhenHitEffectSuppressed()
    {
        var hooksSource = ReadHooksRuntimeSource();
        Assert.Contains(
            "PlayCastOnCritProcFeedbackSfx(coc.spell);",
            hooksSource,
            StringComparison.Ordinal);
    }

    [Fact]
    public void RuntimeHook_CastOnCrit_InjectsSafeShortFeedbackVfx()
    {
        var hooksSource = ReadHooksRuntimeSource();
        Assert.Contains(
            "PlayCastOnCritProcFeedbackVfxSafe(a_target, coc.spell, a_now);",
            hooksSource,
            StringComparison.Ordinal);
        Assert.Contains(
            "InstantiateHitArt(",
            hooksSource,
            StringComparison.Ordinal);
        Assert.DoesNotContain(
            "InstantiateHitShader(",
            hooksSource,
            StringComparison.Ordinal);
    }

    [Fact]
    public void RuntimeHook_CastOnCrit_UsesUnifiedSafeFeedbackVfxArt()
    {
        var hooksSource = ReadHooksRuntimeSource();
        Assert.Contains(
            "ResolveSafeCastOnCritProcFeedbackArt",
            hooksSource,
            StringComparison.Ordinal);
        Assert.Contains(
            "0x00012FD0",
            hooksSource,
            StringComparison.Ordinal);
        Assert.Contains(
            "return ResolveSafeCastOnCritProcFeedbackArt();",
            hooksSource,
            StringComparison.Ordinal);
    }

    [Fact]
    public void RuntimeHook_OnHealthDamage_ForwardsOriginalDamage()
    {
        var hooksSource = ReadHooksRuntimeSource();
        // OnHealthDamage receives the pre-conversion original damage so the stale
        // guard's expectedDealt comparison is like-for-like.
        Assert.Contains(
            "bridge->OnHealthDamage(a_target, a_attacker, hitData, a_originalDamage);",
            hooksSource,
            StringComparison.Ordinal);
        Assert.DoesNotContain(
            "bridge->OnHealthDamage(safeTarget, safeAttacker, hitData, a_damage);",
            hooksSource,
            StringComparison.Ordinal);
    }

    [Fact]
    public void RepoSpec_IncludesSummonCorpseExplosionAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasSummonCorpseExplosion = spec.Keywords.Affixes.Any(affix =>
        {
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("type", out var typeEl) || typeEl.ValueKind != JsonValueKind.String)
            {
                return false;
            }

            return typeEl.GetString() == "SummonCorpseExplosion";
        });

        Assert.True(
            hasSummonCorpseExplosion,
            "Expected at least one affix with runtime.action.type == 'SummonCorpseExplosion' in affixes/affixes.json");
    }

    [Fact]
    public void RepoSpec_IncludesEvolutionAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasEvolution = spec.Keywords.Affixes.Any(affix =>
        {
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("evolution", out var evolutionEl))
            {
                return false;
            }

            return evolutionEl.ValueKind == JsonValueKind.Object;
        });

        Assert.True(
            hasEvolution,
            "Expected at least one affix with runtime.action.evolution in affixes/affixes.json");
    }

    [Fact]
    public void RepoSpec_IncludesModeCycleAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasModeCycle = spec.Keywords.Affixes.Any(affix =>
        {
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("modeCycle", out var modeCycleEl))
            {
                return false;
            }

            return modeCycleEl.ValueKind == JsonValueKind.Object;
        });

        Assert.True(
            hasModeCycle,
            "Expected at least one affix with runtime.action.modeCycle in affixes/affixes.json");
    }

    [Fact]
    public void RepoSpec_IncludesManualModeCycleAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasManualModeCycle = spec.Keywords.Affixes.Any(affix =>
        {
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("modeCycle", out var modeCycleEl) || modeCycleEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!modeCycleEl.TryGetProperty("manualOnly", out var manualOnlyEl) || manualOnlyEl.ValueKind != JsonValueKind.True)
            {
                return false;
            }

            return true;
        });

        Assert.True(
            hasManualModeCycle,
            "Expected at least one affix with runtime.action.modeCycle.manualOnly == true in affixes/affixes.json");
    }

    [Fact]
    public void RepoSpec_IncludesGrowthAndManualModeCycleCombinedAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasCombinedAffix = spec.Keywords.Affixes.Any(affix =>
        {
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);

            if (actionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("evolution", out var evolutionEl) || evolutionEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!actionEl.TryGetProperty("modeCycle", out var modeCycleEl) || modeCycleEl.ValueKind != JsonValueKind.Object)
            {
                return false;
            }

            if (!modeCycleEl.TryGetProperty("manualOnly", out var manualOnlyEl) || manualOnlyEl.ValueKind != JsonValueKind.True)
            {
                return false;
            }

            return true;
        });

        Assert.True(
            hasCombinedAffix,
            "Expected at least one affix with both runtime.action.evolution and runtime.action.modeCycle.manualOnly == true in affixes/affixes.json");
    }

    [Fact]
    public void RepoSpec_RunewordIdentityWave_MatchesSemanticContract()
    {
        var repoRoot = FindRepoRoot();
        var modulePath = Path.Combine(repoRoot, "affixes", "modules", "keywords.affixes.runewords.json");
        Assert.True(File.Exists(modulePath), $"Expected runeword module at: {modulePath}");

        using var document = JsonDocument.Parse(File.ReadAllText(modulePath));
        Assert.True(document.RootElement.ValueKind == JsonValueKind.Array, "Expected the runeword module to contain a top-level array.");

        var runewords = document.RootElement
            .EnumerateArray()
            .ToDictionary(
                element => element.GetProperty("id").GetString()!,
                element => element.Clone(),
                StringComparer.Ordinal);

        JsonElement Affix(string id)
        {
            Assert.True(runewords.TryGetValue(id, out var affix), $"Expected runeword '{id}' in the source module.");
            return affix;
        }

        JsonElement Object(JsonElement parent, string property, string context)
        {
            Assert.True(
                parent.TryGetProperty(property, out var value) && value.ValueKind == JsonValueKind.Object,
                $"{context}: expected '{property}' to be an object.");
            return value;
        }

        void StringValue(JsonElement parent, string property, string expected, string context)
        {
            Assert.True(
                parent.TryGetProperty(property, out var value) &&
                value.ValueKind == JsonValueKind.String &&
                value.GetString() == expected,
                $"{context}: expected '{property}' == '{expected}'.");
        }

        void NumberValue(JsonElement parent, string property, double expected, string context)
        {
            Assert.True(
                parent.TryGetProperty(property, out var value) &&
                value.ValueKind == JsonValueKind.Number &&
                value.TryGetDouble(out var actual) &&
                Math.Abs(actual - expected) < 0.000001,
                $"{context}: expected '{property}' == {expected}.");
        }

        JsonElement Action(JsonElement affix, string context) =>
            Object(Object(affix, "runtime", context), "action", $"{context} runtime");

        JsonElement Record(JsonElement affix, string singular, string plural, string editorId, string context)
        {
            var records = Object(affix, "records", context);
            var matches = new List<JsonElement>();

            if (records.TryGetProperty(singular, out var one) &&
                one.ValueKind == JsonValueKind.Object &&
                one.TryGetProperty("editorId", out var oneId) &&
                oneId.GetString() == editorId)
            {
                matches.Add(one.Clone());
            }

            if (records.TryGetProperty(plural, out var many) && many.ValueKind == JsonValueKind.Array)
            {
                matches.AddRange(
                    many.EnumerateArray()
                        .Where(record =>
                            record.TryGetProperty("editorId", out var recordId) &&
                            recordId.GetString() == editorId)
                        .Select(record => record.Clone()));
            }

            Assert.True(matches.Count == 1, $"{context}: expected exactly one record '{editorId}'; got {matches.Count}.");
            return matches[0];
        }

        List<JsonElement> Effects(JsonElement spell, string context)
        {
            if (spell.TryGetProperty("effects", out var many) && many.ValueKind == JsonValueKind.Array)
            {
                return many.EnumerateArray().Select(effect => effect.Clone()).ToList();
            }

            Assert.True(
                spell.TryGetProperty("effect", out var one) && one.ValueKind == JsonValueKind.Object,
                $"{context}: expected an effect object or effects array.");
            return new List<JsonElement> { one.Clone() };
        }

        void SpellMagnitude(IReadOnlyCollection<JsonElement> effects, string magicEffectEditorId, double magnitude, string context)
        {
            var matches = effects
                .Where(effect =>
                    effect.TryGetProperty("magicEffectEditorId", out var effectId) &&
                    effectId.GetString() == magicEffectEditorId)
                .ToList();
            Assert.True(matches.Count == 1, $"{context}: expected exactly one effect '{magicEffectEditorId}'; got {matches.Count}.");
            NumberValue(matches[0], "magnitude", magnitude, context);
        }

        void ActorValueEffect(
            string affixId,
            string spellEditorId,
            string magicEffectEditorId,
            string actorValue,
            double magnitude,
            string context)
        {
            var affix = Affix(affixId);
            var spell = Record(affix, "spell", "spells", spellEditorId, $"{context} spell");
            SpellMagnitude(Effects(spell, $"{context} spell"), magicEffectEditorId, magnitude, context);
            var magicEffect = Record(affix, "magicEffect", "magicEffects", magicEffectEditorId, $"{context} magic effect");
            StringValue(magicEffect, "actorValue", actorValue, $"{context} magic effect");
        }

        var chaosAction = Action(Affix("runeword_chaos_final"), "Chaos");
        StringValue(chaosAction, "type", "CastSpellAdaptiveElement", "Chaos action");
        StringValue(chaosAction, "mode", "WeakestResist", "Chaos action");
        NumberValue(chaosAction, "magnitudeOverride", 45.0, "Chaos action");
        var chaosSpells = Object(chaosAction, "spells", "Chaos action");
        Assert.True(chaosSpells.EnumerateObject().Count() == 3, $"Chaos action: expected exactly 3 shared elemental spells; got {chaosSpells.EnumerateObject().Count()}.");
        StringValue(chaosSpells, "Fire", "CAFF_SPEL_DMG_FIRE_DYNAMIC", "Chaos spells");
        StringValue(chaosSpells, "Frost", "CAFF_SPEL_DMG_FROST_DYNAMIC", "Chaos spells");
        StringValue(chaosSpells, "Shock", "CAFF_SPEL_DMG_SHOCK_DYNAMIC", "Chaos spells");

        var mosaic = Affix("runeword_mosaic_final");
        var mosaicSpell = Record(mosaic, "spell", "spells", "CAFF_SPEL_RW_MOSAIC_PRISM", "Mosaic");
        var mosaicEffects = Effects(mosaicSpell, "Mosaic spell");
        Assert.True(mosaicEffects.Count == 3, $"Mosaic spell: expected exactly 3 elemental effects; got {mosaicEffects.Count}.");
        SpellMagnitude(mosaicEffects, "CAFF_MGEF_DMG_FIRE_DYNAMIC", 8.0, "Mosaic fire");
        SpellMagnitude(mosaicEffects, "CAFF_MGEF_DMG_FROST_DYNAMIC", 8.0, "Mosaic frost");
        SpellMagnitude(mosaicEffects, "CAFF_MGEF_DMG_SHOCK_DYNAMIC", 8.0, "Mosaic shock");

        var obsessionAction = Action(Affix("runeword_obsession_final"), "Obsession");
        StringValue(obsessionAction, "type", "CastSpell", "Obsession action");
        StringValue(obsessionAction, "spellEditorId", "CAFF_SPEL_RW_CHAOS_BLEED", "Obsession action");
        StringValue(obsessionAction, "applyTo", "Target", "Obsession action");
        NumberValue(obsessionAction, "magnitudeOverride", 75.0, "Obsession action");
        StringValue(obsessionAction, "passiveSpellEditorId", "CAFF_SPEL_RW_OBSESSION_PASSIVE", "Obsession action");

        var botd = Affix("runeword_breath_of_the_dying_final");
        var botdAction = Action(botd, "Breath of the Dying");
        StringValue(botdAction, "type", "CorpseExplosion", "Breath of the Dying action");
        NumberValue(botdAction, "flatDamage", 24.0, "Breath of the Dying action");
        NumberValue(botdAction, "pctOfCorpseMaxHealth", 6.0, "Breath of the Dying action");
        NumberValue(botdAction, "radius", 600.0, "Breath of the Dying action");
        NumberValue(botdAction, "maxTargets", 8.0, "Breath of the Dying action");
        NumberValue(botdAction, "selectionPriority", 10.0, "Breath of the Dying action");
        var botdEffect = Record(botd, "magicEffect", "magicEffects", "CAFF_MGEF_RW_BOTD_POISON_BURST", "Breath of the Dying");
        StringValue(botdEffect, "resistValue", "PoisonResist", "Breath of the Dying magic effect");
        var botdSpell = Record(botd, "spell", "spells", "CAFF_SPEL_RW_BOTD_POISON_BURST", "Breath of the Dying");
        var botdSpellEffects = Effects(botdSpell, "Breath of the Dying spell");
        Assert.True(botdSpellEffects.Count == 1, $"Breath of the Dying spell: expected exactly one effect; got {botdSpellEffects.Count}.");
        StringValue(
            botdSpellEffects[0],
            "magicEffectEditorId",
            "CAFF_MGEF_RW_BOTD_POISON_BURST",
            "Breath of the Dying spell effect");
        NumberValue(botdSpellEffects[0], "area", 0.0, "Breath of the Dying spell effect");

        var obedienceAction = Action(Affix("runeword_obedience_final"), "Obedience");
        Assert.False(
            obedienceAction.TryGetProperty("magnitudeScaling", out _),
            "Obedience action: fixed SpeedMult -50% must not scale with hit damage.");

        var brambleAction = Action(Affix("runeword_bramble_final"), "Bramble");
        Assert.False(
            brambleAction.TryGetProperty("magnitudeScaling", out _),
            "Bramble action: fixed ReflectDamage +25% must not reinterpret incoming damage as a percent magnitude.");

        var effectContracts = new[]
        {
            ("runeword_obedience_final", "CAFF_SPEL_RW_OBEDIENCE_CRUSH", "CAFF_MGEF_RW_OBEDIENCE_CRUSH", "SpeedMult", 50.0, "Obedience fixed slow"),
            ("runeword_bramble_final", "CAFF_SPEL_RW_BRAMBLE_REFLECT", "CAFF_MGEF_RW_BRAMBLE_REFLECT", "ReflectDamage", 25.0, "Bramble fixed reflect"),
            ("runeword_obsession_final", "CAFF_SPEL_RW_OBSESSION_PASSIVE", "CAFF_MGEF_RW_OBSESSION_PASSIVE_MPREGEN", "MagickaRateMult", 25.0, "Obsession passive"),
            ("runeword_rhyme_final", "CAFF_SPEL_RW_RHYME_SHIELD", "CAFF_MGEF_RW_RHYME_SHIELD", "DamageResist", 60.0, "Rhyme active"),
            ("runeword_rhyme_final", "CAFF_SPEL_RW_RHYME_PASSIVE", "CAFF_MGEF_RW_RHYME_PASSIVE_FROST", "ResistFrost", 40.0, "Rhyme passive"),
            ("runeword_faith_final", "CAFF_SPEL_RW_FAITH_FANATIC", "CAFF_MGEF_RW_FAITH_FANATIC_SPEED", "WeaponSpeedMult", 0.15, "Faith active attack speed"),
            ("runeword_faith_final", "CAFF_SPEL_RW_FAITH_FANATIC", "CAFF_MGEF_RW_FAITH_FANATIC_WARD", "AttackDamageMult", 0.20, "Faith active attack damage"),
            ("runeword_faith_final", "CAFF_SPEL_RW_FAITH_PASSIVE", "CAFF_MGEF_RW_FAITH_PASSIVE_IAS", "WeaponSpeedMult", 0.15, "Faith passive"),
            ("runeword_call_to_arms_final", "CAFF_SPEL_RW_CTA_WARCRY", "CAFF_MGEF_RW_CTA_WARCRY", "AttackDamageMult", 0.20, "Call to Arms active"),
            ("runeword_honor_final", "CAFF_SPEL_RW_HONOR_VIGOR", "CAFF_MGEF_RW_HONOR_VIGOR", "HealRateMult", 80.0, "Honor active"),
            ("runeword_honor_final", "CAFF_SPEL_RW_HONOR_PASSIVE", "CAFF_MGEF_RW_HONOR_PASSIVE_HPREGEN", "HealRateMult", 20.0, "Honor passive"),
            ("runeword_hustle_a_final", "CAFF_SPEL_RW_HUSTLE_A_RUSH", "CAFF_MGEF_RW_HUSTLE_A_RUSH", "DamageResist", 60.0, "Hustle-A active"),
            ("runeword_hustle_a_final", "CAFF_SPEL_RW_HUSTLEA_PASSIVE", "CAFF_MGEF_RW_HUSTLEA_PASSIVE_SPEED", "SpeedMult", 8.0, "Hustle-A passive")
        };

        foreach (var contract in effectContracts)
        {
            ActorValueEffect(
                contract.Item1,
                contract.Item2,
                contract.Item3,
                contract.Item4,
                contract.Item5,
                contract.Item6);
        }
    }

    [Fact]
    public void RepoSpec_RunewordIdentityWave2_MatchesSemanticContract()
    {
        var repoRoot = FindRepoRoot();
        var modulePath = Path.Combine(repoRoot, "affixes", "modules", "keywords.affixes.runewords.json");
        using var moduleDocument = JsonDocument.Parse(File.ReadAllText(modulePath));
        var runewords = moduleDocument.RootElement.EnumerateArray()
            .ToDictionary(x => x.GetProperty("id").GetString()!, x => x.Clone(), StringComparer.Ordinal);

        JsonElement Affix(string id)
        {
            Assert.True(runewords.TryGetValue(id, out var value), $"Expected runeword '{id}'.");
            return value;
        }

        JsonElement Action(string id) => Affix(id).GetProperty("runtime").GetProperty("action");

        void StringValue(JsonElement parent, string property, string expected, string context)
        {
            Assert.True(
                parent.TryGetProperty(property, out var value) &&
                value.ValueKind == JsonValueKind.String &&
                value.GetString() == expected,
                $"{context}: expected {property} == '{expected}'.");
        }

        void NumberValue(JsonElement parent, string property, double expected, string context)
        {
            Assert.True(
                parent.TryGetProperty(property, out var value) &&
                value.ValueKind == JsonValueKind.Number &&
                value.TryGetDouble(out var actual) &&
                Math.Abs(actual - expected) < 0.000001,
                $"{context}: expected {property} == {expected}.");
        }

        void BoolValue(JsonElement parent, string property, bool expected, string context)
        {
            Assert.True(
                parent.TryGetProperty(property, out var value) &&
                (value.ValueKind is JsonValueKind.True or JsonValueKind.False) &&
                value.GetBoolean() == expected,
                $"{context}: expected {property} == {expected}.");
        }

        var spells = new Dictionary<string, JsonElement>(StringComparer.Ordinal);
        foreach (var affix in runewords.Values)
        {
            var records = affix.GetProperty("records");
            if (records.TryGetProperty("spell", out var one) && one.ValueKind == JsonValueKind.Object)
            {
                spells.Add(one.GetProperty("editorId").GetString()!, one.Clone());
            }
            if (records.TryGetProperty("spells", out var many) && many.ValueKind == JsonValueKind.Array)
            {
                foreach (var spell in many.EnumerateArray())
                {
                    spells.Add(spell.GetProperty("editorId").GetString()!, spell.Clone());
                }
            }
        }

        var magicEffects = new Dictionary<string, JsonElement>(StringComparer.Ordinal);
        void CollectMagicEffects(JsonElement node)
        {
            if (node.ValueKind == JsonValueKind.Object)
            {
                if (node.TryGetProperty("magicEffect", out var one) &&
                    one.ValueKind == JsonValueKind.Object &&
                    one.TryGetProperty("editorId", out var oneId))
                {
                    magicEffects.TryAdd(oneId.GetString()!, one.Clone());
                }
                if (node.TryGetProperty("magicEffects", out var many) && many.ValueKind == JsonValueKind.Array)
                {
                    foreach (var effect in many.EnumerateArray())
                    {
                        magicEffects.TryAdd(effect.GetProperty("editorId").GetString()!, effect.Clone());
                    }
                }
                if (node.TryGetProperty("appendedMagicEffects", out var appended) &&
                    appended.ValueKind == JsonValueKind.Array)
                {
                    foreach (var effect in appended.EnumerateArray())
                    {
                        magicEffects.TryAdd(effect.GetProperty("editorId").GetString()!, effect.Clone());
                    }
                }
                foreach (var property in node.EnumerateObject())
                {
                    CollectMagicEffects(property.Value);
                }
            }
            else if (node.ValueKind == JsonValueKind.Array)
            {
                foreach (var item in node.EnumerateArray())
                {
                    CollectMagicEffects(item);
                }
            }
        }

        foreach (var sourcePath in Directory.EnumerateFiles(
                     Path.Combine(repoRoot, "affixes", "modules"),
                     "*.json",
                     SearchOption.AllDirectories))
        {
            using var sourceDocument = JsonDocument.Parse(File.ReadAllText(sourcePath));
            CollectMagicEffects(sourceDocument.RootElement);
        }

        void AssertRuntime(
            string id, string trigger, double chance, double icd,
            string actionType, string spellEditorId, string? applyTo, bool noHitEffectArt)
        {
            var affix = Affix(id);
            var runtime = affix.GetProperty("runtime");
            StringValue(runtime, "trigger", trigger, id);
            NumberValue(runtime, "procChancePercent", chance, id);
            NumberValue(runtime, "icdSeconds", icd, id);
            var action = runtime.GetProperty("action");
            StringValue(action, "type", actionType, id);
            StringValue(action, "spellEditorId", spellEditorId, id);
            if (applyTo is null)
            {
                Assert.False(action.TryGetProperty("applyTo", out _), $"{id}: specialized action must not retain applyTo.");
            }
            else
            {
                StringValue(action, "applyTo", applyTo, id);
            }
            BoolValue(action, "noHitEffectArt", noHitEffectArt, id);
        }

        void AssertSpell(
            string context,
            string spellEditorId,
            params (string EffectId, double Magnitude, double Duration, double Area)[] expected)
        {
            Assert.True(spells.TryGetValue(spellEditorId, out var spell), $"{context}: missing spell {spellEditorId}.");
            var effects = spell.TryGetProperty("effects", out var many)
                ? many.EnumerateArray().Select(x => x.Clone()).ToList()
                : new List<JsonElement> { spell.GetProperty("effect").Clone() };
            Assert.Equal(expected.Length, effects.Count);
            for (var i = 0; i < expected.Length; i++)
            {
                StringValue(effects[i], "magicEffectEditorId", expected[i].EffectId, context);
                NumberValue(effects[i], "magnitude", expected[i].Magnitude, context);
                NumberValue(effects[i], "duration", expected[i].Duration, context);
                NumberValue(effects[i], "area", expected[i].Area, context);
                Assert.True(magicEffects.ContainsKey(expected[i].EffectId), $"{context}: unresolved MGEF {expected[i].EffectId}.");
            }
        }

        void AssertScaling(
            string id, string source, double mult, double min, double max, bool spellBaseAsMin)
        {
            var scaling = Action(id).GetProperty("magnitudeScaling");
            StringValue(scaling, "source", source, id);
            NumberValue(scaling, "mult", mult, id);
            NumberValue(scaling, "add", 0.0, id);
            NumberValue(scaling, "min", min, id);
            NumberValue(scaling, "max", max, id);
            BoolValue(scaling, "spellBaseAsMin", spellBaseAsMin, id);
        }

        void AssertNoScaling(string id) =>
            Assert.False(Action(id).TryGetProperty("magnitudeScaling", out _), $"{id}: unexpected magnitudeScaling.");

        void AssertUiText(string id, string expectedKo, string expectedEn)
        {
            var affix = Affix(id);
            StringValue(affix, "name", expectedKo, id);
            StringValue(affix, "nameKo", expectedKo, id);
            StringValue(affix, "nameEn", expectedEn, id);
        }

        void AssertMagicEffect(
            string id, string actorValue, bool hostile, bool? recover,
            string? resistValue = null, string? archetype = null)
        {
            Assert.True(magicEffects.TryGetValue(id, out var effect), $"Missing MGEF {id}.");
            StringValue(effect, "actorValue", actorValue, id);
            BoolValue(effect, "hostile", hostile, id);
            if (recover.HasValue)
            {
                BoolValue(effect, "recover", recover.Value, id);
            }
            else
            {
                Assert.False(effect.TryGetProperty("recover", out _), $"{id}: recover must be absent.");
            }
            if (resistValue is not null) StringValue(effect, "resistValue", resistValue, id);
            if (archetype is not null) StringValue(effect, "archetype", archetype, id);
        }

        AssertRuntime("runeword_destruction_final", "Hit", 30, 5, "CastSpell", "CAFF_SPEL_RW_DESTRUCTION_BLEED", "Target", false);
        AssertRuntime("runeword_hand_of_justice_final", "Hit", 28, 4, "CastSpell", "CAFF_SPEL_RW_HOJ_LIFESTEAL", "Target", false);
        AssertRuntime("runeword_dragon_final", "IncomingHit", 30, 10, "CastSpell", "CAFF_SPEL_RW_DRAGON_SCALE", "Self", true);
        AssertRuntime("runeword_mist_final", "Hit", 30, 6, "CastSpell", "CAFF_SPEL_RW_MIST_FOG", "Target", false);
        AssertRuntime("runeword_famine_final", "Hit", 30, 7, "CastSpell", "CAFF_SPEL_RW_FAMINE_DRAIN", "Target", false);
        AssertRuntime("runeword_beast_final", "Hit", 30, 18, "CastSpell", "CAFF_SPEL_RW_BEAST_RAGE", "Self", true);
        AssertRuntime("runeword_eternity_final", "IncomingHit", 25, 15, "CastSpell", "CAFF_SPEL_RW_ETERNITY_BULWARK", "Self", true);
        AssertRuntime("runeword_enigma_final", "IncomingHit", 100, 30, "CastSpell", "CAFF_SPEL_RW_ENIGMA_PHASE", "Self", true);
        AssertRuntime("runeword_last_wish_final", "LowHealth", 100, 45, "CastSpell", "CAFF_SPEL_RW_LASTWISH_FADE", "Self", true);
        AssertRuntime("runeword_plague_final", "Kill", 40, 4, "CorpseExplosion", "CAFF_SPEL_RW_PLAGUE_CLOUD", null, false);
        NumberValue(Action("runeword_plague_final"), "selectionPriority", 20.0, "Plague action");
        AssertRuntime("runeword_pride_final", "Hit", 30, 4, "CastSpell", "CAFF_SPEL_RW_PRIDE_CHILL", "Target", false);
        AssertRuntime("runeword_mosaic_final", "Hit", 26, 5, "CastSpell", "CAFF_SPEL_RW_MOSAIC_PRISM", "Target", false);
        AssertRuntime("runeword_obsession_final", "Hit", 34, 5, "CastSpell", "CAFF_SPEL_RW_CHAOS_BLEED", "Target", false);

        AssertSpell("Destruction", "CAFF_SPEL_RW_DESTRUCTION_BLEED",
            ("CAFF_MGEF_RW_DESTRUCTION_BLEED", 10, 5, 350));
        AssertSpell("Hand of Justice", "CAFF_SPEL_RW_HOJ_LIFESTEAL",
            ("CAFF_MGEF_RW_HOJ_LIFESTEAL", 40, 0, 0));
        AssertSpell("Dragon", "CAFF_SPEL_RW_DRAGON_SCALE",
            ("CAFF_MGEF_RW_DRAGON_SCALE", 120, 8, 0),
            ("CAFF_MGEF_RW_DRAGON_RESIST_FIRE", 25, 8, 0),
            ("CAFF_MGEF_RW_DRAGON_RESIST_FROST", 25, 8, 0),
            ("CAFF_MGEF_RW_DRAGON_RESIST_SHOCK", 25, 8, 0));
        AssertSpell("Mist", "CAFF_SPEL_RW_MIST_FOG",
            ("CAFF_MGEF_RW_MIST_FOG", 100, 6, 0),
            ("CAFF_MGEF_HIT_SHOCK_OVERLOAD", 75, 0, 0),
            ("CAFF_MGEF_RW_RIFT_SPARK", 30, 6, 0));
        AssertSpell("Famine", "CAFF_SPEL_RW_FAMINE_DRAIN",
            ("CAFF_MGEF_TRAP_DRAGONTEETH_DRAIN_STAMINA", 30, 5, 0),
            ("CAFF_MGEF_RW_FAMINE_DRAIN", 0.20, 6, 0),
            ("CAFF_MGEF_RW_WIND_PUSH", 0.15, 6, 0));
        AssertSpell("Beast", "CAFF_SPEL_RW_BEAST_RAGE",
            ("CAFF_MGEF_RW_BEAST_RAGE_POWER", 0.30, 10, 0),
            ("CAFF_MGEF_RW_BEAST_RAGE_ARMOR", 150, 10, 0),
            ("CAFF_MGEF_RW_FAITH_FANATIC_SPEED", 0.15, 10, 0));
        AssertSpell("Eternity", "CAFF_SPEL_RW_ETERNITY_BULWARK",
            ("CAFF_MGEF_INCOMING_WARDEN_SHELL", 300, 6, 0),
            ("CAFF_MGEF_RW_ETERNITY_BULWARK", 25, 6, 0));
        AssertSpell("Enigma", "CAFF_SPEL_RW_ENIGMA_PHASE",
            ("CAFF_MGEF_RW_ENIGMA_PHASE", 1, 4, 0),
            ("CAFF_MGEF_HIT_SHADOWSTEP_BURST", 45, 4, 0));
        AssertSpell("Last Wish", "CAFF_SPEL_RW_LASTWISH_FADE",
            ("CAFF_MGEF_RW_LASTWISH_FADE_ARMOR", 250, 12, 0),
            ("CAFF_MGEF_RW_LASTWISH_FADE_RESIST", 50, 12, 0),
            ("CAFF_MGEF_HIT_BLOOD_SIPHON", 250, 0, 0));
        AssertSpell("Plague", "CAFF_SPEL_RW_PLAGUE_CLOUD",
            ("CAFF_MGEF_RW_PLAGUE_CLOUD", 1, 1, 0));
        AssertSpell("Pride", "CAFF_SPEL_RW_PRIDE_CHILL",
            ("CAFF_MGEF_RW_PRIDE_CHILL", 30, 0, 250));
        AssertSpell("Mosaic", "CAFF_SPEL_RW_MOSAIC_PRISM",
            ("CAFF_MGEF_DMG_FIRE_DYNAMIC", 8, 0, 0),
            ("CAFF_MGEF_DMG_FROST_DYNAMIC", 8, 0, 0),
            ("CAFF_MGEF_DMG_SHOCK_DYNAMIC", 8, 0, 0));
        AssertSpell("Obsession", "CAFF_SPEL_RW_CHAOS_BLEED",
            ("CAFF_MGEF_RW_CHAOS_BLEED", 1, 0, 0));

        AssertScaling("runeword_destruction_final", "HitPhysicalDealt", 0.04, 0, 40, true);
        AssertScaling("runeword_hand_of_justice_final", "HitTotalDealt", 0.18, 40, 250, true);
        AssertScaling("runeword_pride_final", "HitPhysicalDealt", 0.16, 0, 200, true);
        AssertScaling("runeword_mosaic_final", "HitTotalDealt", 0.05, 0, 80, true);
        AssertScaling("runeword_obsession_final", "HitTotalDealt", 0.18, 75, 300, false);
        AssertUiText(
            "runeword_destruction_final",
            "룬워드 파괴 [Vex-Lo-Ber-Jah-Ko]: 적중 시 30% 확률로 전격 폭풍(물리 적중 피해의 4%, 전격 피해 10~40/초, 5초, 반경 350). 5초마다 발동.",
            "Runeword Destruction (Vex-Lo-Ber-Jah-Ko): 30% on hit / ICD 5s - Shock Storm (4% of Physical Hit Damage, 10-40 Shock Damage/s, 5s, Radius 350)");
        AssertUiText(
            "runeword_last_wish_final",
            "룬워드 최후의 소원 [Jah-Mal-Jah-Sur-Jah-Ber]: 체력 35% 이하일 때 체력 250 즉시 회복, 방어도 +250·마법저항 +50(12초). 45초마다 발동.",
            "Runeword Last Wish (Jah-Mal-Jah-Sur-Jah-Ber): HP<35% / ICD 45s - Restore 250 Health + Armor 250 and Magic Resist 50 (12s)");
        AssertUiText(
            "runeword_plague_final",
            "룬워드 역병 [Cham-Shael-Um]: 처치 시 40% 확률로 역병 시체 연쇄 폭발(독 피해 12 + 시체 최대 체력 3%, 반경 450, 최대 12명, 최대 연쇄 깊이 2). 4초마다 발동.",
            "Runeword Plague (Cham-Shael-Um): 40% on Kill / ICD 4s - Plague Corpse Chain Explosion (12 + 3% Corpse Max Health, Radius 450, up to 12 targets, max chain depth 2)");
        AssertUiText(
            "runeword_pride_final",
            "룬워드 프라이드 [Cham-Sur-Io-Lo]: 적중 시 30% 확률로 냉기 충격파(물리 적중 피해의 16%, 냉기 피해 30~200, 반경 250). 4초마다 발동.",
            "Runeword Pride (Cham-Sur-Io-Lo): 30% on hit / ICD 4s - Frost Impact (16% of Physical Hit Damage, 30-200 Frost Damage, Radius 250)");
        foreach (var id in new[]
                 {
                     "runeword_dragon_final", "runeword_mist_final", "runeword_famine_final",
                     "runeword_beast_final", "runeword_eternity_final", "runeword_enigma_final",
                     "runeword_last_wish_final", "runeword_plague_final"
                 })
        {
            AssertNoScaling(id);
        }

        AssertMagicEffect("CAFF_MGEF_RW_HOJ_LIFESTEAL", "Health", true, false, "ResistFire", "Absorb");
        AssertMagicEffect("CAFF_MGEF_INCOMING_WARDEN_SHELL", "DamageResist", false, true);
        AssertMagicEffect("CAFF_MGEF_RW_ETERNITY_BULWARK", "ReflectDamage", false, true);
        AssertMagicEffect("CAFF_MGEF_RW_DRAGON_SCALE", "DamageResist", false, true);
        AssertMagicEffect("CAFF_MGEF_TRAP_DRAGONTEETH_DRAIN_STAMINA", "Stamina", true, null);
        AssertMagicEffect("CAFF_MGEF_RW_FAMINE_DRAIN", "AttackDamageMult", true, true);
        AssertMagicEffect("CAFF_MGEF_RW_ENIGMA_PHASE", "Invisibility", false, null, archetype: "Invisibility");

        var lastWishRuntime = Affix("runeword_last_wish_final").GetProperty("runtime");
        NumberValue(lastWishRuntime, "lowHealthThresholdPercent", 35, "Last Wish");
        var obsessionAction = Action("runeword_obsession_final");
        NumberValue(obsessionAction, "magnitudeOverride", 75, "Obsession");
        StringValue(obsessionAction, "passiveSpellEditorId", "CAFF_SPEL_RW_OBSESSION_PASSIVE", "Obsession");

        var plagueAction = Action("runeword_plague_final");
        NumberValue(plagueAction, "flatDamage", 12, "Plague");
        NumberValue(plagueAction, "pctOfCorpseMaxHealth", 3, "Plague");
        NumberValue(plagueAction, "radius", 450, "Plague");
        NumberValue(plagueAction, "maxTargets", 12, "Plague");
        NumberValue(plagueAction, "maxChainDepth", 2, "Plague");
        NumberValue(plagueAction, "chainFalloff", 0.70, "Plague");
        NumberValue(plagueAction, "chainWindowSeconds", 0.6, "Plague");
        NumberValue(plagueAction, "rateLimitWindowSeconds", 1.0, "Plague");
        NumberValue(plagueAction, "maxExplosionsPerWindow", 2, "Plague");

        var summaryPath = Path.Combine(repoRoot, "skse", "CalamityAffixes", "src", "EventBridge.Loot.Runeword.SummaryText.h");
        var summary = File.ReadAllText(summaryPath);
        (string Id, string Key, string Text, string? OldKey)[] uiContracts =
        {
            ("rw_destruction", "signature_destruction", "적중 시 반경 350 전격 폭풍: 물리 피해의 4%, 초당 10~40(5초)", "shock_dot"),
            ("rw_hand_of_justice", "signature_hand_of_justice", "화염 심판으로 체력 40~250 흡수(적중 피해 18%)", "self_judgment"),
            ("rw_dragon", "signature_dragon", "피격 시 방어도 +120·화염/냉기/전격 저항 +25(8초)", "self_light_armor"),
            ("rw_mist", "signature_mist", "마나 재생 -100%·마법 저항 -30(6초), 마나 75 즉시 소진", "debuff_magicka_suppress"),
            ("rw_famine", "signature_famine", "기력 -30/초(5초), 공격력 -20%·공격 속도 -15%(6초)", "debuff_stamina_drain"),
            ("rw_beast", "signature_beast", "공격력 +30%·방어도 +150·공격 속도 +15%(10초)", "long_cd_beast_rage"),
            ("rw_eternity", "signature_eternity", "피격 시 방어도 +300·피해 반사 +25%(6초)", "long_cd_bulwark"),
            ("rw_enigma", "signature_enigma", "피격 시 투명화·이동 속도 +45%(4초), 상시 이동 속도 +10%", null),
            ("rw_last_wish", "signature_last_wish", "체력 35% 이하에서 체력 250 회복·방어도 +250·마법 저항 +50(12초)", "lowhealth_fade"),
            ("rw_plague", "signature_plague", "시체 연쇄 폭발: 독 피해 12 + 시체 최대 체력 3%, 반경 450, 최대 연쇄 깊이 2", "kill_poison_cloud"),
            ("rw_pride", "signature_pride", "적중 시 반경 250 냉기 충격파: 물리 피해의 16%, 피해 30~200", "frost_strike"),
            ("rw_mosaic", "signature_mosaic", "화염·냉기·전격 피해를 각각 8~80(적중 피해 5%) 동시에 가함", null),
            ("rw_obsession", "signature_obsession", "적중 시 마법 피해 75~300(적중 피해 18%), 상시 마나 재생률 +25%", null)
        };
        foreach (var contract in uiContracts)
        {
            Assert.Contains($"{{ \"{contract.Id}\", \"{contract.Key}\" }}", summary);
            Assert.Contains(contract.Text, summary);
            if (contract.OldKey is not null)
            {
                Assert.DoesNotContain($"{{ \"{contract.Id}\", \"{contract.OldKey}\" }}", summary);
            }
        }
    }

    [Fact]
    public void RepoSpec_AllAffixesHaveBilingualNames()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        foreach (var affix in spec.Keywords.Affixes)
        {
            Assert.False(
                string.IsNullOrWhiteSpace(affix.NameEn),
                $"Affix {affix.Id} missing nameEn");
            Assert.False(
                string.IsNullOrWhiteSpace(affix.NameKo),
                $"Affix {affix.Id} missing nameKo");
        }
    }

    [Fact]
    public void RepoSpec_LuckyHitSettingsUseSupportedContextsOnly()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        foreach (var affix in spec.Keywords.Affixes)
        {
            var hasLuckyChance = TryGetRuntimeNumber(affix.Runtime, "luckyHitChancePercent", out var luckyChancePct);
            var hasLuckyCoeff = TryGetRuntimeNumber(affix.Runtime, "luckyHitProcCoefficient", out var luckyCoeff);
            if ((!hasLuckyChance || luckyChancePct <= 0.0) && !hasLuckyCoeff)
            {
                continue;
            }

            _ = TryGetRuntimeString(affix.Runtime, "trigger", out var trigger);
            _ = TryGetActionType(affix.Runtime, out var actionType);

            var supported =
                actionType is "CastOnCrit" or "ConvertDamage" or "MindOverMatter" or "Archmage" ||
                trigger is "Hit" or "IncomingHit";

            Assert.True(
                supported,
                $"Affix {affix.Id} uses luckyHit settings in unsupported context (trigger={trigger ?? "<none>"}, action.type={actionType ?? "<none>"}).");
        }
    }

    [Fact]
    public void RepoSpec_IncludesMindOverMatterAffixWithValidFields()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);
        var found = false;

        foreach (var affix in spec.Keywords.Affixes)
        {
            if (!TryGetActionType(affix.Runtime, out var actionType) || actionType != "MindOverMatter")
            {
                continue;
            }

            found = true;

            Assert.True(
                TryGetRuntimeString(affix.Runtime, "trigger", out var trigger) && trigger == "IncomingHit",
                $"MindOverMatter affix {affix.Id} must use runtime.trigger == IncomingHit.");

            Assert.True(affix.Runtime.Action is not null,
                $"MindOverMatter affix {affix.Id} must have runtime.action.");
            var actionEl = JsonSerializer.SerializeToElement(affix.Runtime.Action);
            Assert.True(actionEl.ValueKind == JsonValueKind.Object,
                $"MindOverMatter affix {affix.Id} runtime.action must be object.");

            Assert.True(
                actionEl.TryGetProperty("damageToMagickaPct", out var redirectPctEl) &&
                redirectPctEl.ValueKind == JsonValueKind.Number &&
                redirectPctEl.TryGetDouble(out var redirectPct) &&
                redirectPct > 0.0 &&
                redirectPct <= 100.0,
                $"MindOverMatter affix {affix.Id} must define action.damageToMagickaPct in (0, 100].");

            if (actionEl.TryGetProperty("maxRedirectPerHit", out var maxRedirectEl))
            {
                Assert.True(
                    maxRedirectEl.ValueKind == JsonValueKind.Number &&
                    maxRedirectEl.TryGetDouble(out var maxRedirect) &&
                    maxRedirect >= 0.0,
                    $"MindOverMatter affix {affix.Id} has invalid action.maxRedirectPerHit (expected >= 0).");
            }
        }

        Assert.True(found, "Expected at least one MindOverMatter affix in affixes/affixes.json.");
    }

    [Fact]
    public void RepoSpec_SpecialActions_UseExplicitPositiveProcChance()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);
        var offenders = spec.Keywords.Affixes
            .Where(affix =>
            {
                if (!TryGetActionType(affix.Runtime, out var actionType) ||
                    actionType is not ("CastOnCrit" or "ConvertDamage" or "MindOverMatter" or "Archmage" or "CorpseExplosion" or "SummonCorpseExplosion"))
                {
                    return false;
                }

                return !TryGetRuntimeNumber(affix.Runtime, "procChancePercent", out var procChance) || procChance <= 0.0;
            })
            .Select(affix => affix.Id)
            .ToList();

        Assert.True(
            offenders.Count == 0,
            "Expected special actions to use explicit runtime.procChancePercent > 0; offenders: " +
            string.Join(", ", offenders));
    }

    private static AllocationRecord[] AllocationSignature(IModGetter mod)
    {
        return mod
            .EnumerateMajorRecords()
            .OrderBy(record => record.FormKey.ID)
            .Select(record => new AllocationRecord(
                record.FormKey.ID,
                RecordSignature(record),
                RecordEditorId(record)))
            .ToArray();
    }

    private static string RecordSignature(IMajorRecordGetter record) => record switch
    {
        IQuestGetter => "QUST",
        IMiscItemGetter => "MISC",
        IKeywordGetter => "KYWD",
        IMagicEffectGetter => "MGEF",
        ISpellGetter => "SPEL",
        IArtObjectGetter => "ARTO",
        ILeveledItemGetter => "LVLI",
        _ => throw new InvalidDataException($"Unsupported allocation fixture record type: {record.GetType().Name}"),
    };

    private static string RecordEditorId(IMajorRecordGetter record) => record switch
    {
        IQuestGetter value => value.EditorID,
        IMiscItemGetter value => value.EditorID,
        IKeywordGetter value => value.EditorID,
        IMagicEffectGetter value => value.EditorID,
        ISpellGetter value => value.EditorID,
        IArtObjectGetter value => value.EditorID,
        ILeveledItemGetter value => value.EditorID,
        _ => throw new InvalidDataException($"Unsupported allocation fixture record type: {record.GetType().Name}"),
    } ?? throw new InvalidDataException($"Record {record.FormKey} is missing EditorID.");

    private static AllocationFixture ReadV140AllocationFixture(string repoRoot)
    {
        var fixturePath = Path.Combine(
            repoRoot,
            "tools",
            "CalamityAffixes.Generator.Tests",
            "Fixtures",
            "v1.4.0-c584576-allocation.json");
        using var document = JsonDocument.Parse(File.ReadAllText(fixturePath));
        var fixtureVersion = document.RootElement.GetProperty("fixtureVersion").GetInt32();
        var source = document.RootElement.GetProperty("source");
        var records = document.RootElement.GetProperty("records")
            .EnumerateArray()
            .Select(record => new AllocationRecord(
                ParseLocalFormId(record.GetProperty("localFormId").GetString()),
                record.GetProperty("signature").GetString()!,
                record.GetProperty("editorId").GetString()!))
            .ToArray();

        return new AllocationFixture(
            fixtureVersion,
            source.GetProperty("plugin").GetString()!,
            source.GetProperty("releaseTag").GetString()!,
            source.GetProperty("commit").GetString()!,
            source.GetProperty("generatorPackage").GetString()!,
            source.GetProperty("generatorVersion").GetString()!,
            source.GetProperty("sourceEspSha256").GetString()!,
            source.GetProperty("majorRecordCount").GetInt32(),
            ParseLocalFormId(source.GetProperty("nextObjectId").GetString()),
            records);
    }

    private static uint ParseLocalFormId(string? value)
    {
        if (value is null || !value.StartsWith("0x", StringComparison.Ordinal) ||
            !uint.TryParse(value.AsSpan(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var parsed))
        {
            throw new InvalidDataException($"Invalid local FormID in allocation fixture: {value ?? "<null>"}");
        }

        return parsed;
    }

    private sealed record AllocationFixture(
        int FixtureVersion,
        string SourcePlugin,
        string ReleaseTag,
        string Commit,
        string GeneratorPackage,
        string GeneratorVersion,
        string SourceEspSha256,
        int MajorRecordCount,
        uint NextObjectId,
        AllocationRecord[] Records);

    private sealed record AllocationRecord(uint FormId, string RecordType, string EditorId);

    private static string FindRepoRoot()
    {
        var dir = new DirectoryInfo(AppContext.BaseDirectory);
        while (dir is not null)
        {
            if (File.Exists(Path.Combine(dir.FullName, "affixes", "affixes.json")))
            {
                return dir.FullName;
            }

            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Failed to locate repo root (affixes/affixes.json not found above test output directory).");
    }

    private static string ReadHooksRuntimeSource()
    {
        var repoRoot = FindRepoRoot();
        var hooksPath = Path.Combine(repoRoot, "skse", "CalamityAffixes", "src", "Hooks.cpp");
        var hooksDispatchPath = Path.Combine(repoRoot, "skse", "CalamityAffixes", "src", "Hooks.Dispatch.cpp");

        Assert.True(File.Exists(hooksPath), $"Expected Hooks.cpp to exist at path: {hooksPath}");
        Assert.True(File.Exists(hooksDispatchPath), $"Expected Hooks.Dispatch.cpp to exist at path: {hooksDispatchPath}");

        return File.ReadAllText(hooksPath) + "\n" + File.ReadAllText(hooksDispatchPath);
    }

    private static bool TryGetRuntimeString(RuntimeSpec? runtime, string key, out string? value)
    {
        value = null;

        if (runtime is null)
        {
            return false;
        }

        // Known typed fields
        if (key == "trigger")
        {
            value = runtime.Trigger;
            return value is not null;
        }

        // Fall back to extension data for unknown fields
        if (runtime.ExtensionData is null || !runtime.ExtensionData.TryGetValue(key, out var obj))
        {
            return false;
        }

        if (obj is not JsonElement el || el.ValueKind != JsonValueKind.String)
        {
            return false;
        }

        value = el.GetString();
        return value is not null;
    }

    private static bool TryGetRuntimeNumber(RuntimeSpec? runtime, string key, out double value)
    {
        value = 0.0;

        if (runtime is null)
        {
            return false;
        }

        // Known typed fields
        switch (key)
        {
            case "procChancePercent":
                if (runtime.ProcChancePercent.HasValue) { value = runtime.ProcChancePercent.Value; return true; }
                return false;
            case "icdSeconds":
                if (runtime.IcdSeconds.HasValue) { value = runtime.IcdSeconds.Value; return true; }
                return false;
            case "perTargetICDSeconds":
                if (runtime.PerTargetICDSeconds.HasValue) { value = runtime.PerTargetICDSeconds.Value; return true; }
                return false;
            case "lootWeight":
                if (runtime.LootWeight.HasValue) { value = runtime.LootWeight.Value; return true; }
                return false;
        }

        // Fall back to extension data for unknown fields
        if (runtime.ExtensionData is null || !runtime.ExtensionData.TryGetValue(key, out var obj))
        {
            return false;
        }

        if (obj is not JsonElement el || el.ValueKind is not (JsonValueKind.Number))
        {
            return false;
        }

        return el.TryGetDouble(out value);
    }

    private static bool TryGetActionType(RuntimeSpec? runtime, out string? value)
    {
        value = null;

        if (runtime?.Action is null)
        {
            return false;
        }

        var actionEl = JsonSerializer.SerializeToElement(runtime.Action);
        if (actionEl.ValueKind != JsonValueKind.Object)
        {
            return false;
        }

        if (!actionEl.TryGetProperty("type", out var typeEl) || typeEl.ValueKind != JsonValueKind.String)
        {
            return false;
        }

        value = typeEl.GetString();
        return value is not null;
    }
}
