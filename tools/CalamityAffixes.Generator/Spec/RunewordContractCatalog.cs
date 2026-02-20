using System.Globalization;
using System.Text.RegularExpressions;
using System.Text.Json;

namespace CalamityAffixes.Generator.Spec;

internal static class RunewordContractCatalog
{
    internal sealed record RecipeEntry(
        string Id,
        string DisplayName,
        IReadOnlyList<string> Runes,
        string ResultAffixId,
        string? RecommendedBase);

    internal sealed record RuneWeightEntry(
        string Rune,
        double Weight);

    internal sealed record Snapshot(
        IReadOnlyList<RecipeEntry> Recipes,
        IReadOnlyList<RuneWeightEntry> RuneWeights);

    private const string RunewordContractRelativePath = "affixes/runeword.contract.json";
    private const string RunewordCatalogRowsRelativePath = "skse/CalamityAffixes/src/RunewordCatalogRows.inl";
    private const string RunewordWeightsRelativePath = "skse/CalamityAffixes/src/EventBridge.Loot.Runeword.Detail.cpp";

    private static readonly Regex RunewordCatalogRowRegex = new(
        "RunewordCatalogRow\\{\\s*\"(?<id>[^\"]+)\"\\s*,\\s*\"(?<name>[^\"]+)\"\\s*,\\s*\"(?<runes>[^\"]*)\"\\s*,\\s*\"(?<result>[^\"]+)\"\\s*,\\s*(?<base>[^}\\r\\n]+)\\s*\\},?",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly Regex RuneWeightRegex = new(
        "\\{\\s*\"(?<rune>[^\"]+)\"\\s*,\\s*(?<weight>[0-9]+(?:\\.[0-9]+)?)\\s*\\},?",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly IReadOnlyList<RuneWeightEntry> FallbackRuneWeights =
    [
        new RuneWeightEntry("El", 1200.0),
        new RuneWeightEntry("Eld", 1100.0),
        new RuneWeightEntry("Tir", 1000.0),
        new RuneWeightEntry("Nef", 900.0),
        new RuneWeightEntry("Eth", 800.0),
        new RuneWeightEntry("Ith", 700.0),
        new RuneWeightEntry("Tal", 620.0),
        new RuneWeightEntry("Ral", 560.0),
        new RuneWeightEntry("Ort", 500.0),
        new RuneWeightEntry("Thul", 450.0),
        new RuneWeightEntry("Amn", 400.0),
        new RuneWeightEntry("Sol", 340.0),
        new RuneWeightEntry("Shael", 280.0),
        new RuneWeightEntry("Dol", 230.0),
        new RuneWeightEntry("Hel", 190.0),
        new RuneWeightEntry("Io", 155.0),
        new RuneWeightEntry("Lum", 125.0),
        new RuneWeightEntry("Ko", 100.0),
        new RuneWeightEntry("Fal", 80.0),
        new RuneWeightEntry("Lem", 64.0),
        new RuneWeightEntry("Pul", 50.0),
        new RuneWeightEntry("Um", 39.0),
        new RuneWeightEntry("Mal", 30.0),
        new RuneWeightEntry("Ist", 23.0),
        new RuneWeightEntry("Gul", 17.0),
        new RuneWeightEntry("Vex", 14.0),
        new RuneWeightEntry("Ohm", 11.0),
        new RuneWeightEntry("Lo", 8.0),
        new RuneWeightEntry("Sur", 6.0),
        new RuneWeightEntry("Ber", 5.0),
        new RuneWeightEntry("Jah", 4.0),
        new RuneWeightEntry("Cham", 3.0),
        new RuneWeightEntry("Zod", 2.0),
    ];

    internal static Snapshot Load()
    {
        var contractSnapshot = ParseContractSnapshot(FindByWalkingParents(RunewordContractRelativePath));
        var recipes = contractSnapshot.Recipes.Count > 0
            ? contractSnapshot.Recipes.ToList()
            : ParseCatalogRows(FindByWalkingParents(RunewordCatalogRowsRelativePath));
        var runeWeights = contractSnapshot.RuneWeights.Count > 0
            ? contractSnapshot.RuneWeights.ToList()
            : ParseRuneWeights(FindByWalkingParents(RunewordWeightsRelativePath));

        if (runeWeights.Count == 0)
        {
            runeWeights = FallbackRuneWeights.ToList();
        }

        if (recipes.Count > 0)
        {
            var existingRunes = new HashSet<string>(
                runeWeights.Select(weight => weight.Rune),
                StringComparer.Ordinal);

            foreach (var rune in recipes.SelectMany(recipe => recipe.Runes))
            {
                if (!existingRunes.Add(rune))
                {
                    continue;
                }

                runeWeights.Add(new RuneWeightEntry(rune, 25.0));
            }
        }

        return new Snapshot(recipes, runeWeights);
    }

    private static Snapshot ParseContractSnapshot(string? sourcePath)
    {
        if (string.IsNullOrWhiteSpace(sourcePath) || !File.Exists(sourcePath))
        {
            return new Snapshot([], []);
        }

        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(sourcePath));
            var root = doc.RootElement;
            if (root.ValueKind != JsonValueKind.Object)
            {
                return new Snapshot([], []);
            }

            var recipes = ParseContractRecipes(root);
            var runeWeights = ParseContractRuneWeights(root);
            return new Snapshot(recipes, runeWeights);
        }
        catch
        {
            return new Snapshot([], []);
        }
    }

    private static List<RecipeEntry> ParseContractRecipes(JsonElement root)
    {
        if (!root.TryGetProperty("runewordCatalog", out var catalogElement) ||
            catalogElement.ValueKind != JsonValueKind.Array)
        {
            return [];
        }

        var output = new List<RecipeEntry>(catalogElement.GetArrayLength());
        foreach (var recipeElement in catalogElement.EnumerateArray())
        {
            if (recipeElement.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            if (!TryGetRequiredString(recipeElement, "id", out var recipeId) ||
                !TryGetRequiredString(recipeElement, "name", out var displayName) ||
                !TryGetRequiredString(recipeElement, "resultAffixId", out var resultAffixId))
            {
                continue;
            }

            if (!recipeElement.TryGetProperty("runes", out var runesElement) ||
                runesElement.ValueKind != JsonValueKind.Array)
            {
                continue;
            }

            var runes = new List<string>(runesElement.GetArrayLength());
            foreach (var runeElement in runesElement.EnumerateArray())
            {
                if (runeElement.ValueKind != JsonValueKind.String)
                {
                    continue;
                }

                var rune = runeElement.GetString()?.Trim();
                if (string.IsNullOrWhiteSpace(rune))
                {
                    continue;
                }

                runes.Add(rune);
            }

            if (runes.Count == 0)
            {
                continue;
            }

            string? recommendedBase = null;
            if (recipeElement.TryGetProperty("recommendedBase", out var baseElement) &&
                baseElement.ValueKind == JsonValueKind.String)
            {
                var baseToken = baseElement.GetString()?.Trim();
                if (string.Equals(baseToken, "Weapon", StringComparison.OrdinalIgnoreCase))
                {
                    recommendedBase = "Weapon";
                }
                else if (string.Equals(baseToken, "Armor", StringComparison.OrdinalIgnoreCase))
                {
                    recommendedBase = "Armor";
                }
            }

            output.Add(new RecipeEntry(
                recipeId,
                displayName,
                runes,
                resultAffixId,
                recommendedBase));
        }

        return output;
    }

    private static List<RuneWeightEntry> ParseContractRuneWeights(JsonElement root)
    {
        if (!root.TryGetProperty("runewordRuneWeights", out var weightsElement) ||
            weightsElement.ValueKind != JsonValueKind.Array)
        {
            return [];
        }

        var output = new List<RuneWeightEntry>(weightsElement.GetArrayLength());
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var entryElement in weightsElement.EnumerateArray())
        {
            if (entryElement.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            if (!TryGetRequiredString(entryElement, "rune", out var rune) ||
                !entryElement.TryGetProperty("weight", out var weightElement) ||
                weightElement.ValueKind != JsonValueKind.Number ||
                !weightElement.TryGetDouble(out var weight) ||
                weight <= 0.0 ||
                !seen.Add(rune))
            {
                continue;
            }

            output.Add(new RuneWeightEntry(rune, weight));
        }

        return output;
    }

    private static bool TryGetRequiredString(JsonElement parent, string key, out string value)
    {
        value = string.Empty;
        if (!parent.TryGetProperty(key, out var property) || property.ValueKind != JsonValueKind.String)
        {
            return false;
        }

        var raw = property.GetString()?.Trim();
        if (string.IsNullOrWhiteSpace(raw))
        {
            return false;
        }

        value = raw;
        return true;
    }

    private static List<RecipeEntry> ParseCatalogRows(string? sourcePath)
    {
        var output = new List<RecipeEntry>();
        if (string.IsNullOrWhiteSpace(sourcePath) || !File.Exists(sourcePath))
        {
            return output;
        }

        foreach (var line in File.ReadLines(sourcePath))
        {
            var match = RunewordCatalogRowRegex.Match(line);
            if (!match.Success)
            {
                continue;
            }

            var recipeId = match.Groups["id"].Value.Trim();
            var displayName = match.Groups["name"].Value.Trim();
            var resultAffixId = match.Groups["result"].Value.Trim();
            if (string.IsNullOrWhiteSpace(recipeId) ||
                string.IsNullOrWhiteSpace(displayName) ||
                string.IsNullOrWhiteSpace(resultAffixId))
            {
                continue;
            }

            var runes = match.Groups["runes"]
                .Value
                .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                .Where(rune => !string.IsNullOrWhiteSpace(rune))
                .ToArray();
            if (runes.Length == 0)
            {
                continue;
            }

            var baseToken = match.Groups["base"].Value.Trim();
            var recommendedBase = baseToken switch
            {
                "LootItemType::kWeapon" => "Weapon",
                "LootItemType::kArmor" => "Armor",
                "std::nullopt" => null,
                _ => null,
            };

            output.Add(new RecipeEntry(
                recipeId,
                displayName,
                runes,
                resultAffixId,
                recommendedBase));
        }

        return output;
    }

    private static List<RuneWeightEntry> ParseRuneWeights(string? sourcePath)
    {
        var output = new List<RuneWeightEntry>();
        if (string.IsNullOrWhiteSpace(sourcePath) || !File.Exists(sourcePath))
        {
            return output;
        }

        var seen = new HashSet<string>(StringComparer.Ordinal);
        foreach (var line in File.ReadLines(sourcePath))
        {
            var match = RuneWeightRegex.Match(line);
            if (!match.Success)
            {
                continue;
            }

            var rune = match.Groups["rune"].Value.Trim();
            if (string.IsNullOrWhiteSpace(rune) || !seen.Add(rune))
            {
                continue;
            }

            if (!double.TryParse(
                    match.Groups["weight"].Value,
                    NumberStyles.Float | NumberStyles.AllowThousands,
                    CultureInfo.InvariantCulture,
                    out var weight) || weight <= 0.0)
            {
                continue;
            }

            output.Add(new RuneWeightEntry(rune, weight));
        }

        return output;
    }

    private static string? FindByWalkingParents(string relativePath)
    {
        var starts = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        void AddStart(string? candidate)
        {
            if (string.IsNullOrWhiteSpace(candidate))
            {
                return;
            }

            starts.Add(Path.GetFullPath(candidate));
        }

        AddStart(Directory.GetCurrentDirectory());
        AddStart(AppContext.BaseDirectory);

        foreach (var start in starts)
        {
            var current = start;
            while (!string.IsNullOrWhiteSpace(current))
            {
                var candidate = Path.Combine(current, relativePath);
                if (File.Exists(candidate))
                {
                    return candidate;
                }

                var parent = Directory.GetParent(current);
                if (parent is null)
                {
                    break;
                }

                current = parent.FullName;
            }
        }

        return null;
    }
}
