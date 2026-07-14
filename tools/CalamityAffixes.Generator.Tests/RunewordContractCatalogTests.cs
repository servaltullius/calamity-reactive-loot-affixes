using System.Text.Json.Nodes;
using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Tests;

public sealed class RunewordContractCatalogTests
{
    [Fact]
    public void LoadFromPath_RepoContract_IsExactUniquePositiveAndFullyReferenced()
    {
        var contractPath = Path.Combine(FindRepoRoot(), "affixes", "runeword.contract.json");
        var snapshot = RunewordContractCatalog.LoadFromPath(contractPath);

        Assert.Equal(94, snapshot.Recipes.Count);
        Assert.Equal(33, snapshot.RuneWeights.Count);
        Assert.Equal(
            snapshot.Recipes.Count,
            snapshot.Recipes.Select(recipe => recipe.Id).Distinct(StringComparer.OrdinalIgnoreCase).Count());
        Assert.Equal(
            snapshot.Recipes.Count,
            snapshot.Recipes.Select(recipe => recipe.ResultAffixId).Distinct(StringComparer.OrdinalIgnoreCase).Count());
        Assert.Equal(
            snapshot.RuneWeights.Count,
            snapshot.RuneWeights.Select(weight => weight.Rune).Distinct(StringComparer.OrdinalIgnoreCase).Count());
        Assert.All(snapshot.RuneWeights, weight => Assert.True(double.IsFinite(weight.Weight) && weight.Weight > 0.0));

        var weightedRunes = snapshot.RuneWeights
            .Select(weight => weight.Rune)
            .ToHashSet(StringComparer.OrdinalIgnoreCase);
        Assert.All(
            snapshot.Recipes.SelectMany(recipe => recipe.Runes),
            rune => Assert.Contains(rune, weightedRunes));
    }

    [Fact]
    public void LoadFromPath_WhenRecipeIdIsDuplicated_Throws()
    {
        var root = LoadMutableRepoContract();
        var catalog = root["runewordCatalog"]!.AsArray();
        catalog[1]!["id"] = catalog[0]!["id"]!.GetValue<string>();

        WithTemporaryContract(root, path =>
        {
            var ex = Assert.Throws<InvalidDataException>(() => RunewordContractCatalog.LoadFromPath(path));
            Assert.Contains("Duplicate runeword recipe id", ex.Message, StringComparison.OrdinalIgnoreCase);
        });
    }

    [Fact]
    public void LoadFromPath_WhenRuneWeightIsNotPositive_Throws()
    {
        var root = LoadMutableRepoContract();
        var weights = root["runewordRuneWeights"]!.AsArray();
        weights[0]!["weight"] = 0.0;

        WithTemporaryContract(root, path =>
        {
            var ex = Assert.Throws<InvalidDataException>(() => RunewordContractCatalog.LoadFromPath(path));
            Assert.Contains("finite positive weight", ex.Message, StringComparison.OrdinalIgnoreCase);
        });
    }

    private static JsonObject LoadMutableRepoContract()
    {
        var contractPath = Path.Combine(FindRepoRoot(), "affixes", "runeword.contract.json");
        return JsonNode.Parse(File.ReadAllText(contractPath))!.AsObject();
    }

    private static void WithTemporaryContract(JsonObject root, Action<string> assertion)
    {
        var tempRoot = Path.Combine(
            Path.GetTempPath(),
            "CalamityAffixes.Generator.Tests",
            Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        var path = Path.Combine(tempRoot, "runeword.contract.json");
        File.WriteAllText(path, root.ToJsonString());

        try
        {
            assertion(path);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    private static string FindRepoRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            if (File.Exists(Path.Combine(current.FullName, "affixes", "runeword.contract.json")))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate repository root.");
    }
}
