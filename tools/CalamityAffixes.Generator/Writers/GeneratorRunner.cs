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
    private static readonly string[] OfficialMasterFileNames =
    [
        "Skyrim.esm",
        "Update.esm",
        "Dawnguard.esm",
        "HearthFires.esm",
        "Dragonborn.esm",
    ];
    private static readonly string[] SupportedPluginExtensions = [".esm", ".esp", ".esl"];

    private sealed record LeveledListContext(
        Func<FormKey, ILeveledItemGetter?> Resolver,
        IReadOnlyList<FormKey> AutoDiscoveredCurrencyTargets);

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
        var leveledListContext = CreateLeveledListContext(spec, mastersDir);
        var mod = KeywordPluginBuilder.Build(
            spec,
            leveledListContext?.Resolver,
            leveledListContext?.AutoDiscoveredCurrencyTargets);
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

    private static LeveledListContext? CreateLeveledListContext(AffixSpec spec, string? mastersDir)
    {
        if (spec.Loot is not { } loot || !RequiresLeveledListResolver(loot))
        {
            return null;
        }

        if (string.IsNullOrWhiteSpace(mastersDir))
        {
            return null;
        }

        var resolvedMastersDir = Path.GetFullPath(mastersDir);
        if (!Directory.Exists(resolvedMastersDir))
        {
            throw new DirectoryNotFoundException($"Masters directory not found: {resolvedMastersDir}");
        }

        var requiredFromTargets = new HashSet<string>(ExtractTargetModFileNames(loot), StringComparer.OrdinalIgnoreCase);

        foreach (var requiredMod in requiredFromTargets)
        {
            var requiredPath = Path.Combine(resolvedMastersDir, requiredMod);
            if (!File.Exists(requiredPath))
            {
                throw new InvalidDataException(
                    $"Required target plugin '{requiredMod}' was not found in masters dir '{resolvedMastersDir}'.");
            }
        }

        var candidates = new HashSet<string>(OfficialMasterFileNames, StringComparer.OrdinalIgnoreCase);
        foreach (var requiredMod in requiredFromTargets)
        {
            candidates.Add(requiredMod);
        }

        if (loot.CurrencyLeveledListAutoDiscoverDeathItems)
        {
            foreach (var pluginFileName in EnumeratePluginFileNames(resolvedMastersDir))
            {
                candidates.Add(pluginFileName);
            }
        }

        var loadOrder = new List<string>();
        foreach (var master in OfficialMasterFileNames)
        {
            if (candidates.Contains(master))
            {
                loadOrder.Add(master);
            }
        }
        foreach (var extra in candidates
                     .Except(OfficialMasterFileNames, StringComparer.OrdinalIgnoreCase)
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

            try
            {
                loadedMasters.Add(SkyrimMod.CreateFromBinary(pluginPath, SkyrimRelease.SkyrimSE));
            }
            catch (Exception ex)
            {
                if (requiredFromTargets.Contains(pluginFileName) || IsOfficialMaster(pluginFileName))
                {
                    throw new InvalidDataException(
                        $"Failed to load required plugin '{pluginFileName}' from masters dir '{resolvedMastersDir}'.",
                        ex);
                }

                Console.Error.WriteLine(
                    $"WARN: skipping plugin '{pluginFileName}' for leveled-list discovery ({ex.Message}).");
            }
        }

        if (loadedMasters.Count == 0)
        {
            throw new InvalidDataException(
                $"No supported master plugins were found in '{resolvedMastersDir}'. " +
                "Expected at least Skyrim.esm.");
        }

        var cache = loadedMasters.ToImmutableLinkCache();
        var resolver = new Func<FormKey, ILeveledItemGetter?>(
            formKey => cache.TryResolve<ILeveledItemGetter>(formKey, out var record) ? record : null);

        var autoDiscoveredTargets = loot.CurrencyLeveledListAutoDiscoverDeathItems
            ? DiscoverModDeathItemTargets(loadedMasters)
            : Array.Empty<FormKey>();

        return new LeveledListContext(resolver, autoDiscoveredTargets);
    }

    private static bool RequiresLeveledListResolver(LootSpec loot)
    {
        var mode = loot.CurrencyDropMode?.Trim().ToLowerInvariant() ?? "hybrid";
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

    private static IEnumerable<string> EnumeratePluginFileNames(string mastersDir)
    {
        foreach (var path in Directory.EnumerateFiles(mastersDir, "*", SearchOption.TopDirectoryOnly))
        {
            var fileName = Path.GetFileName(path);
            if (HasSupportedPluginExtension(fileName))
            {
                yield return fileName;
            }
        }
    }

    private static bool HasSupportedPluginExtension(string fileName)
    {
        var extension = Path.GetExtension(fileName);
        return SupportedPluginExtensions.Contains(extension, StringComparer.OrdinalIgnoreCase);
    }

    private static bool IsOfficialMaster(string fileName)
    {
        return OfficialMasterFileNames.Contains(fileName, StringComparer.OrdinalIgnoreCase);
    }

    private static IReadOnlyList<FormKey> DiscoverModDeathItemTargets(IEnumerable<ISkyrimModGetter> loadedMods)
    {
        var discovered = new List<FormKey>();
        var seen = new HashSet<FormKey>();

        foreach (var mod in loadedMods)
        {
            var modFileName = mod.ModKey.FileName.String;
            if (IsOfficialMaster(modFileName))
            {
                continue;
            }

            foreach (var leveledItem in mod.LeveledItems)
            {
                if (leveledItem.FormKey.ModKey != mod.ModKey)
                {
                    continue;
                }

                if (string.IsNullOrWhiteSpace(leveledItem.EditorID) ||
                    !leveledItem.EditorID.StartsWith("DeathItem", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (seen.Add(leveledItem.FormKey))
                {
                    discovered.Add(leveledItem.FormKey);
                }
            }
        }

        discovered.Sort((left, right) =>
        {
            var byMod = StringComparer.OrdinalIgnoreCase.Compare(
                left.ModKey.FileName.String,
                right.ModKey.FileName.String);
            return byMod != 0 ? byMod : left.ID.CompareTo(right.ID);
        });

        return discovered;
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
