using System.Text.Json;

namespace CalamityAffixes.Generator.Tests;

public sealed class WeaponSubtypeContractTests
{
    [Fact]
    public void RepoSuffixModule_UsesExactWeaponSubtypeFamilies()
    {
        var repoRoot = FindRepoRoot();
        var modulePath = Path.Combine(repoRoot, "affixes", "modules", "keywords.affixes.suffixes.json");
        using var document = JsonDocument.Parse(File.ReadAllText(modulePath));

        var expected = new Dictionary<string, string[]>(StringComparer.Ordinal)
        {
            ["swordsman"] = ["OneHandedMelee"],
            ["champion"] = ["TwoHandedMelee"],
            ["marksman"] = ["Bow", "Crossbow"],
            ["eagle_eye"] = ["Bow", "Crossbow"]
        };
        var counts = expected.Keys.ToDictionary(key => key, _ => 0, StringComparer.Ordinal);

        foreach (var affix in document.RootElement.EnumerateArray())
        {
            var id = affix.GetProperty("id").GetString() ?? "<missing>";
            var family = affix.GetProperty("family").GetString() ?? "<missing>";
            if (!expected.TryGetValue(family, out var expectedSubtypes))
            {
                Assert.False(
                    affix.TryGetProperty("weaponSubtypes", out _),
                    $"Unrestricted suffix '{id}' must not define weaponSubtypes.");
                continue;
            }

            counts[family] += 1;
            Assert.True(
                affix.TryGetProperty("weaponSubtypes", out var subtypes) &&
                subtypes.ValueKind == JsonValueKind.Array,
                $"Restricted suffix '{id}' must define weaponSubtypes as an array.");
            Assert.Equal(
                expectedSubtypes,
                subtypes.EnumerateArray().Select(value => value.GetString()).ToArray());
        }

        foreach (var family in expected.Keys)
        {
            Assert.Equal(3, counts[family]);
        }
    }

    [Fact]
    public void RepoSchema_RestrictsWeaponSubtypeContractKeys()
    {
        var repoRoot = FindRepoRoot();
        var schemaPath = Path.Combine(repoRoot, "affixes", "affixes.schema.json");
        using var document = JsonDocument.Parse(File.ReadAllText(schemaPath));

        var values = document.RootElement
            .GetProperty("$defs")
            .GetProperty("affix")
            .GetProperty("properties")
            .GetProperty("weaponSubtypes")
            .GetProperty("items")
            .GetProperty("enum")
            .EnumerateArray()
            .Select(value => value.GetString())
            .ToArray();

        Assert.Equal(
            new[] { "OneHandedMelee", "TwoHandedMelee", "Bow", "Crossbow" },
            values);
    }

    private static string FindRepoRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "affixes", "affixes.json")))
            {
                return directory.FullName;
            }
            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException("Failed to locate repository root.");
    }
}
