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
    private const string AllowLegacyFallbackEnvironmentVariable = "CAFF_ALLOW_LEGACY_RUNEWORD_CONTRACT_FALLBACK";
    private const int ExpectedRecipeCount = 94;
    private const int ExpectedRuneWeightCount = 33;

    private static readonly Regex RunewordCatalogRowRegex = new(
        "RunewordCatalogRow\\{\\s*\"(?<id>[^\"]+)\"\\s*,\\s*\"(?<name>[^\"]+)\"\\s*,\\s*\"(?<runes>[^\"]*)\"\\s*,\\s*\"(?<result>[^\"]+)\"\\s*,\\s*(?<base>[^}\\r\\n]+)\\s*\\},?",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly Regex RuneWeightRegex = new(
        "\\{\\s*\"(?<rune>[^\"]+)\"\\s*,\\s*(?<weight>[0-9]+(?:\\.[0-9]+)?)\\s*\\},?",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly IReadOnlyList<RuneWeightEntry> FallbackRuneWeights =
    [
        new RuneWeightEntry("El", 4.0),
        new RuneWeightEntry("Eld", 4.0),
        new RuneWeightEntry("Tir", 4.0),
        new RuneWeightEntry("Nef", 4.0),
        new RuneWeightEntry("Eth", 4.0),
        new RuneWeightEntry("Ith", 4.0),
        new RuneWeightEntry("Tal", 4.0),
        new RuneWeightEntry("Ral", 4.0),
        new RuneWeightEntry("Ort", 4.0),
        new RuneWeightEntry("Thul", 4.0),
        new RuneWeightEntry("Amn", 4.0),
        new RuneWeightEntry("Sol", 3.0),
        new RuneWeightEntry("Shael", 3.0),
        new RuneWeightEntry("Dol", 3.0),
        new RuneWeightEntry("Hel", 3.0),
        new RuneWeightEntry("Io", 3.0),
        new RuneWeightEntry("Lum", 3.0),
        new RuneWeightEntry("Ko", 3.0),
        new RuneWeightEntry("Fal", 3.0),
        new RuneWeightEntry("Lem", 3.0),
        new RuneWeightEntry("Pul", 3.0),
        new RuneWeightEntry("Um", 3.0),
        new RuneWeightEntry("Mal", 2.0),
        new RuneWeightEntry("Ist", 2.0),
        new RuneWeightEntry("Gul", 2.0),
        new RuneWeightEntry("Vex", 2.0),
        new RuneWeightEntry("Ohm", 2.0),
        new RuneWeightEntry("Lo", 2.0),
        new RuneWeightEntry("Sur", 1.0),
        new RuneWeightEntry("Ber", 1.0),
        new RuneWeightEntry("Jah", 1.0),
        new RuneWeightEntry("Cham", 1.0),
        new RuneWeightEntry("Zod", 1.0),
    ];

    internal static Snapshot Load()
    {
        var contractPath = FindByWalkingParents(RunewordContractRelativePath);
        var allowLegacyFallback = string.Equals(
            Environment.GetEnvironmentVariable(AllowLegacyFallbackEnvironmentVariable),
            "1",
            StringComparison.Ordinal);

        if (!string.IsNullOrWhiteSpace(contractPath) && File.Exists(contractPath))
        {
            try
            {
                return LoadFromPath(contractPath);
            }
            catch (Exception ex) when (allowLegacyFallback)
            {
                Console.Error.WriteLine(
                    $"WARNING: Strict runeword contract validation failed ({ex.Message}). " +
                    $"Using explicitly enabled legacy fallback from {AllowLegacyFallbackEnvironmentVariable}=1.");
            }
        }
        else if (!allowLegacyFallback)
        {
            throw new FileNotFoundException(
                $"Required runeword contract was not found: {RunewordContractRelativePath}. " +
                $"Legacy fallback is disabled by default; set {AllowLegacyFallbackEnvironmentVariable}=1 only for recovery tooling.");
        }

        if (!allowLegacyFallback)
        {
            throw new InvalidDataException(
                $"Runeword contract validation failed and legacy fallback is disabled. " +
                $"Set {AllowLegacyFallbackEnvironmentVariable}=1 only for recovery tooling.");
        }

        return LoadLegacyFallback();
    }

    internal static Snapshot LoadFromPath(string sourcePath)
    {
        return ValidateSnapshot(ParseContractSnapshot(sourcePath), sourcePath);
    }

    private static Snapshot LoadLegacyFallback()
    {
        var recipes = ParseCatalogRows(FindByWalkingParents(RunewordCatalogRowsRelativePath));
        var runeWeights = ParseRuneWeights(FindByWalkingParents(RunewordWeightsRelativePath));

        if (runeWeights.Count == 0)
        {
            Console.Error.WriteLine(
                "WARNING: No rune weights loaded from source. Using explicitly enabled hardcoded legacy fallback.");
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

                runeWeights.Add(new RuneWeightEntry(rune, 1.0));
            }
        }

        return ValidateSnapshot(new Snapshot(recipes, runeWeights), "legacy fallback sources");
    }

    private static Snapshot ParseContractSnapshot(string? sourcePath)
    {
        if (string.IsNullOrWhiteSpace(sourcePath) || !File.Exists(sourcePath))
        {
            throw new FileNotFoundException("Runeword contract file does not exist.", sourcePath);
        }

        using var doc = JsonDocument.Parse(File.ReadAllText(sourcePath));
        var root = doc.RootElement;
        if (root.ValueKind != JsonValueKind.Object)
        {
            throw new InvalidDataException("Runeword contract root must be a JSON object.");
        }

        var recipes = ParseContractRecipes(root);
        var runeWeights = ParseContractRuneWeights(root);
        return new Snapshot(recipes, runeWeights);
    }

    private static List<RecipeEntry> ParseContractRecipes(JsonElement root)
    {
        if (!root.TryGetProperty("runewordCatalog", out var catalogElement) ||
            catalogElement.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidDataException("Runeword contract requires a runewordCatalog array.");
        }

        var output = new List<RecipeEntry>(catalogElement.GetArrayLength());
        var recipeIndex = 0;
        foreach (var recipeElement in catalogElement.EnumerateArray())
        {
            if (recipeElement.ValueKind != JsonValueKind.Object)
            {
                throw new InvalidDataException($"runewordCatalog[{recipeIndex}] must be an object.");
            }

            if (!TryGetRequiredString(recipeElement, "id", out var recipeId) ||
                !TryGetRequiredString(recipeElement, "name", out var displayName) ||
                !TryGetRequiredString(recipeElement, "resultAffixId", out var resultAffixId))
            {
                throw new InvalidDataException(
                    $"runewordCatalog[{recipeIndex}] requires non-empty id, name, and resultAffixId strings.");
            }

            if (!recipeElement.TryGetProperty("runes", out var runesElement) ||
                runesElement.ValueKind != JsonValueKind.Array)
            {
                throw new InvalidDataException($"Runeword recipe '{recipeId}' requires a runes array.");
            }

            var runes = new List<string>(runesElement.GetArrayLength());
            var runeIndex = 0;
            foreach (var runeElement in runesElement.EnumerateArray())
            {
                if (runeElement.ValueKind != JsonValueKind.String)
                {
                    throw new InvalidDataException(
                        $"Runeword recipe '{recipeId}' rune at index {runeIndex} must be a string.");
                }

                var rune = runeElement.GetString()?.Trim();
                if (string.IsNullOrWhiteSpace(rune))
                {
                    throw new InvalidDataException(
                        $"Runeword recipe '{recipeId}' rune at index {runeIndex} must not be empty.");
                }

                runes.Add(rune);
                runeIndex += 1;
            }

            if (runes.Count == 0)
            {
                throw new InvalidDataException($"Runeword recipe '{recipeId}' must contain at least one rune.");
            }

            string? recommendedBase = null;
            if (recipeElement.TryGetProperty("recommendedBase", out var baseElement))
            {
                if (baseElement.ValueKind == JsonValueKind.String)
                {
                    var baseToken = baseElement.GetString()?.Trim();
                    recommendedBase = baseToken?.ToLowerInvariant() switch
                    {
                        "weapon" => "Weapon",
                        "armor" => "Armor",
                        _ => throw new InvalidDataException(
                            $"Runeword recipe '{recipeId}' recommendedBase must be Weapon, Armor, or null."),
                    };
                }
                else if (baseElement.ValueKind != JsonValueKind.Null)
                {
                    throw new InvalidDataException(
                        $"Runeword recipe '{recipeId}' recommendedBase must be Weapon, Armor, or null.");
                }
            }

            output.Add(new RecipeEntry(
                recipeId,
                displayName,
                runes,
                resultAffixId,
                recommendedBase));
            recipeIndex += 1;
        }

        return output;
    }

    private static List<RuneWeightEntry> ParseContractRuneWeights(JsonElement root)
    {
        if (!root.TryGetProperty("runewordRuneWeights", out var weightsElement) ||
            weightsElement.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidDataException("Runeword contract requires a runewordRuneWeights array.");
        }

        var output = new List<RuneWeightEntry>(weightsElement.GetArrayLength());
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var weightIndex = 0;
        foreach (var entryElement in weightsElement.EnumerateArray())
        {
            if (entryElement.ValueKind != JsonValueKind.Object)
            {
                throw new InvalidDataException($"runewordRuneWeights[{weightIndex}] must be an object.");
            }

            if (!TryGetRequiredString(entryElement, "rune", out var rune) ||
                !entryElement.TryGetProperty("weight", out var weightElement) ||
                weightElement.ValueKind != JsonValueKind.Number ||
                !weightElement.TryGetDouble(out var weight) ||
                !double.IsFinite(weight) ||
                weight <= 0.0)
            {
                throw new InvalidDataException(
                    $"runewordRuneWeights[{weightIndex}] requires a non-empty rune and finite positive weight.");
            }

            if (!seen.Add(rune))
            {
                throw new InvalidDataException($"Duplicate runeword rune weight: {rune}");
            }

            output.Add(new RuneWeightEntry(rune, weight));
            weightIndex += 1;
        }

        return output;
    }

    private static Snapshot ValidateSnapshot(Snapshot snapshot, string sourceLabel)
    {
        if (snapshot.Recipes.Count != ExpectedRecipeCount)
        {
            throw new InvalidDataException(
                $"Runeword contract '{sourceLabel}' must contain exactly {ExpectedRecipeCount} recipes " +
                $"(found {snapshot.Recipes.Count}).");
        }

        if (snapshot.RuneWeights.Count != ExpectedRuneWeightCount)
        {
            throw new InvalidDataException(
                $"Runeword contract '{sourceLabel}' must contain exactly {ExpectedRuneWeightCount} rune weights " +
                $"(found {snapshot.RuneWeights.Count}).");
        }

        var recipeIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var resultAffixIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var recipe in snapshot.Recipes)
        {
            if (!recipeIds.Add(recipe.Id))
            {
                throw new InvalidDataException($"Duplicate runeword recipe id: {recipe.Id}");
            }

            if (!resultAffixIds.Add(recipe.ResultAffixId))
            {
                throw new InvalidDataException($"Duplicate runeword resultAffixId: {recipe.ResultAffixId}");
            }
        }

        var runeNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var runeWeight in snapshot.RuneWeights)
        {
            if (string.IsNullOrWhiteSpace(runeWeight.Rune) ||
                !double.IsFinite(runeWeight.Weight) ||
                runeWeight.Weight <= 0.0)
            {
                throw new InvalidDataException(
                    $"Rune weights require non-empty rune names and finite positive values: {runeWeight.Rune}");
            }

            if (!runeNames.Add(runeWeight.Rune))
            {
                throw new InvalidDataException($"Duplicate runeword rune weight: {runeWeight.Rune}");
            }
        }

        var referencedRunes = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var recipe in snapshot.Recipes)
        {
            foreach (var rune in recipe.Runes)
            {
                if (!runeNames.Contains(rune))
                {
                    throw new InvalidDataException(
                        $"Runeword recipe '{recipe.Id}' references rune '{rune}' without a weight entry.");
                }

                referencedRunes.Add(rune);
            }
        }

        var unusedRuneWeights = runeNames
            .Where(rune => !referencedRunes.Contains(rune))
            .OrderBy(rune => rune, StringComparer.OrdinalIgnoreCase)
            .ToArray();
        if (unusedRuneWeights.Length > 0)
        {
            throw new InvalidDataException(
                "Runeword contract contains unreferenced rune weights: " + string.Join(", ", unusedRuneWeights));
        }

        return snapshot;
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
