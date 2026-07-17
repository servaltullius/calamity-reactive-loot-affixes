using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Tests;

public sealed class LootDefaultsTests
{
    [Fact]
    public void LootSpec_DefaultsMatchPublishedCurrencyAndSourceContract()
    {
        var defaults = new LootSpec();

        Assert.Equal(12.0, defaults.RunewordFragmentChancePercent);
        Assert.Equal(7.0, defaults.ReforgeOrbChancePercent);
        Assert.Equal(1.0, defaults.LootSourceChanceMultCorpse);
        Assert.Equal(1.0, defaults.LootSourceChanceMultContainer);
        Assert.Equal(1.15, defaults.LootSourceChanceMultBossContainer);
        Assert.Equal(1.0, defaults.LootSourceChanceMultWorld);
    }

    [Fact]
    public void RepoDefaults_MatchGeneratorAndPapyrusFallbacks()
    {
        var repoRoot = FindRepoRoot();
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));
        var loot = Assert.IsType<LootSpec>(spec.Loot);

        Assert.Equal(12.0, loot.RunewordFragmentChancePercent);
        Assert.Equal(7.0, loot.ReforgeOrbChancePercent);
        Assert.Equal(1.0, loot.LootSourceChanceMultCorpse);
        Assert.Equal(1.15, loot.LootSourceChanceMultBossContainer);

        var papyrusSource = File.ReadAllText(
            Path.Combine(repoRoot, "Data", "Scripts", "Source", "CalamityAffixes_MCMConfig.psc"));
        Assert.Contains(
            "RunewordFragmentChanceDefault = 12.0",
            papyrusSource,
            StringComparison.Ordinal);
        Assert.Contains(
            "ReforgeOrbChanceDefault = 7.0",
            papyrusSource,
            StringComparison.Ordinal);
    }

    private static string FindRepoRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            if (File.Exists(Path.Combine(current.FullName, "affixes", "affixes.modules.json")))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate repository root.");
    }
}
