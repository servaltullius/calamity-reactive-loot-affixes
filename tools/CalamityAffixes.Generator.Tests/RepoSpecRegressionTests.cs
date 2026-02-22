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

            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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
            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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

                if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
                {
                    return false;
                }

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
    public void RepoSpec_IncludesSummonCorpseExplosionAffix()
    {
        var repoRoot = FindRepoRoot();
        var specPath = Path.Combine(repoRoot, "affixes", "affixes.json");

        var spec = AffixSpecLoader.Load(specPath);

        var hasSummonCorpseExplosion = spec.Keywords.Affixes.Any(affix =>
        {
            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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
            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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
            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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
            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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
            if (!affix.Runtime.TryGetValue("action", out var actionObj) || actionObj is not JsonElement actionEl)
            {
                return false;
            }

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

            Assert.True(affix.Runtime.TryGetValue("action", out var actionObj),
                $"MindOverMatter affix {affix.Id} must have runtime.action.");
            Assert.True(actionObj is JsonElement, $"MindOverMatter affix {affix.Id} runtime.action must be JSON.");
            var actionEl = (JsonElement)actionObj!;
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

    private static bool TryGetRuntimeString(Dictionary<string, object?> runtime, string key, out string? value)
    {
        value = null;

        if (!runtime.TryGetValue(key, out var obj))
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

    private static bool TryGetRuntimeNumber(Dictionary<string, object?> runtime, string key, out double value)
    {
        value = 0.0;

        if (!runtime.TryGetValue(key, out var obj))
        {
            return false;
        }

        if (obj is not JsonElement el || el.ValueKind is not (JsonValueKind.Number))
        {
            return false;
        }

        return el.TryGetDouble(out value);
    }

    private static bool TryGetActionType(Dictionary<string, object?> runtime, out string? value)
    {
        value = null;

        if (!runtime.TryGetValue("action", out var obj) || obj is not JsonElement actionEl || actionEl.ValueKind != JsonValueKind.Object)
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
