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
}
