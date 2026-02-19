using CalamityAffixes.Generator.Spec;
using Mutagen.Bethesda.Plugins.Records;
using Noggog;
using System.Text.Json;

namespace CalamityAffixes.Generator.Writers;

public static class GeneratorRunner
{
    private const string LegacySplitPluginName = "CalamityAffixes_Keywords.esp";
    private const string LegacyMcmOnlyPluginName = "CalamityAffixes.esp";
    private const string LegacySplitInventoryInjectorFileName = "CalamityAffixes_Keywords.json";

    public static void Generate(AffixSpec spec, string dataDir)
    {
        Directory.CreateDirectory(dataDir);

        File.WriteAllText(
            Path.Combine(dataDir, "CalamityAffixes_KID.ini"),
            KidIniWriter.Render(spec));

        File.WriteAllText(
            Path.Combine(dataDir, "CalamityAffixes_DISTR.ini"),
            SpidIniWriter.Render(spec));

        var pluginPath = Path.Combine(dataDir, spec.ModKey);
        var mod = KeywordPluginBuilder.Build(spec);
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
