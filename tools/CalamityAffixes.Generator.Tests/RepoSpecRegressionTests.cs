using System.Text.Json;
using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Tests;

public sealed class RepoSpecRegressionTests
{
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
