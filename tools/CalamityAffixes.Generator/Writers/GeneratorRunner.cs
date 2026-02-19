using CalamityAffixes.Generator.Spec;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Cache;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;
using System.Text.Json;

namespace CalamityAffixes.Generator.Writers;

public static class GeneratorRunner
{
    private const string LegacySplitPluginName = "CalamityAffixes_Keywords.esp";
    private const string LegacyMcmOnlyPluginName = "CalamityAffixes.esp";
    private const string LegacySplitInventoryInjectorFileName = "CalamityAffixes_Keywords.json";

    public static void Generate(AffixSpec spec, string dataDir, string? mastersDir = null)
    {
        Directory.CreateDirectory(dataDir);

        File.WriteAllText(
            Path.Combine(dataDir, "CalamityAffixes_KID.ini"),
            KidIniWriter.Render(spec));

        File.WriteAllText(
            Path.Combine(dataDir, "CalamityAffixes_DISTR.ini"),
            SpidIniWriter.Render(spec));

        var pluginPath = Path.Combine(dataDir, spec.ModKey);
        var leveledListResolver = CreateLeveledListResolver(spec, mastersDir);
        var mod = KeywordPluginBuilder.Build(spec, leveledListResolver);
        ((IModGetter)mod).WriteToBinary(new FilePath(pluginPath));

        DeleteLegacyPluginIfDifferent(dataDir, spec.ModKey, LegacySplitPluginName);
        DeleteLegacyPluginIfDifferent(dataDir, spec.ModKey, LegacyMcmOnlyPluginName);

        var configDir = Path.Combine(dataDir, "SKSE", "Plugins", "CalamityAffixes");
        Directory.CreateDirectory(configDir);

        var json = JsonSerializer.Serialize(spec, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(Path.Combine(configDir, "affixes.json"), json);
        File.WriteAllText(Path.Combine(configDir, "runtime_contract.json"), AffixSpecLoader.BuildValidationContractJson());

        var inventoryInjectorDir = Path.Combine(dataDir, "SKSE", "Plugins", "InventoryInjector");
        Directory.CreateDirectory(inventoryInjectorDir);

        var inventoryInjectorFileName = Path.GetFileNameWithoutExtension(spec.ModKey) + ".json";
        var inventoryInjectorPath = Path.Combine(inventoryInjectorDir, inventoryInjectorFileName);
        File.WriteAllText(inventoryInjectorPath, InventoryInjectorJsonWriter.Render(spec));
        DeleteLegacyInventoryInjectorIfDifferent(inventoryInjectorPath, LegacySplitInventoryInjectorFileName);
    }

    private static Func<FormKey, ILeveledItemGetter?>? CreateLeveledListResolver(AffixSpec spec, string? mastersDir)
    {
        if (string.IsNullOrWhiteSpace(mastersDir))
        {
            return null;
        }

        var resolvedMastersDir = Path.GetFullPath(mastersDir);
        if (!Directory.Exists(resolvedMastersDir))
        {
            throw new DirectoryNotFoundException($"Masters directory not found: {resolvedMastersDir}");
        }

        var officialMasters = new[]
        {
            "Skyrim.esm",
            "Update.esm",
            "Dawnguard.esm",
            "HearthFires.esm",
            "Dragonborn.esm",
        };

        var requiredFromTargets = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        if (spec.Loot is { } loot && RequiresLeveledListResolver(loot))
        {
            foreach (var modFileName in ExtractTargetModFileNames(loot))
            {
                requiredFromTargets.Add(modFileName);
            }
        }

        foreach (var requiredMod in requiredFromTargets)
        {
            var requiredPath = Path.Combine(resolvedMastersDir, requiredMod);
            if (!File.Exists(requiredPath))
            {
                throw new InvalidDataException(
                    $"Required target plugin '{requiredMod}' was not found in masters dir '{resolvedMastersDir}'.");
            }
        }

        var candidates = new HashSet<string>(officialMasters, StringComparer.OrdinalIgnoreCase);
        foreach (var requiredMod in requiredFromTargets)
        {
            candidates.Add(requiredMod);
        }

        var loadOrder = new List<string>();
        foreach (var master in officialMasters)
        {
            if (candidates.Contains(master))
            {
                loadOrder.Add(master);
            }
        }
        foreach (var extra in candidates
                     .Except(officialMasters, StringComparer.OrdinalIgnoreCase)
                     .OrderBy(x => x, StringComparer.OrdinalIgnoreCase))
        {
            loadOrder.Add(extra);
        }

        var loadedMasters = new List<ISkyrimModGetter>();
        foreach (var pluginFileName in loadOrder)
        {
            var pluginPath = Path.Combine(resolvedMastersDir, pluginFileName);
            if (!File.Exists(pluginPath))
            {
                continue;
            }

            loadedMasters.Add(SkyrimMod.CreateFromBinary(pluginPath, SkyrimRelease.SkyrimSE));
        }

        if (loadedMasters.Count == 0)
        {
            throw new InvalidDataException(
                $"No supported master plugins were found in '{resolvedMastersDir}'. " +
                "Expected at least Skyrim.esm.");
        }

        var cache = loadedMasters.ToImmutableLinkCache();
        return formKey => cache.TryResolve<ILeveledItemGetter>(formKey, out var record) ? record : null;
    }

    private static bool RequiresLeveledListResolver(LootSpec loot)
    {
        var mode = loot.CurrencyDropMode?.Trim().ToLowerInvariant() ?? "runtime";
        return mode is "leveledlist" or "hybrid";
    }

    private static IEnumerable<string> ExtractTargetModFileNames(LootSpec loot)
    {
        if (loot.CurrencyLeveledListTargets is null || loot.CurrencyLeveledListTargets.Count == 0)
        {
            yield break;
        }

        foreach (var raw in loot.CurrencyLeveledListTargets)
        {
            if (string.IsNullOrWhiteSpace(raw))
            {
                continue;
            }

            var parts = raw.Split('|', 2, StringSplitOptions.TrimEntries);
            if (parts.Length == 2 && !string.IsNullOrWhiteSpace(parts[0]))
            {
                yield return parts[0];
            }
        }
    }

    private static void DeleteLegacyPluginIfDifferent(string dataDir, string generatedModKey, string legacyFileName)
    {
        if (string.Equals(generatedModKey, legacyFileName, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        var legacyPath = Path.Combine(dataDir, legacyFileName);
        if (File.Exists(legacyPath))
        {
            File.Delete(legacyPath);
        }
    }

    private static void DeleteLegacyInventoryInjectorIfDifferent(string generatedPath, string legacyFileName)
    {
        var generatedFileName = Path.GetFileName(generatedPath);
        if (string.Equals(generatedFileName, legacyFileName, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        var legacyPath = Path.Combine(Path.GetDirectoryName(generatedPath)!, legacyFileName);
        if (File.Exists(legacyPath))
        {
            File.Delete(legacyPath);
        }
    }
}
