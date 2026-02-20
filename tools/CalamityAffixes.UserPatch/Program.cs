using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Cache;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;
using System.Globalization;

static class Program
{
    private static readonly string[] OfficialMasterFileNames =
    [
        "Skyrim.esm",
        "Update.esm",
        "Dawnguard.esm",
        "HearthFires.esm",
        "Dragonborn.esm",
    ];

    private const string RunewordDropListEditorId = "CAFF_LItem_RunewordFragmentDrops";
    private const string ReforgeDropListEditorId = "CAFF_LItem_ReforgeOrbDrops";

    private sealed class Options
    {
        public string SpecPath { get; init; } = "affixes/affixes.json";
        public string? MastersDir { get; init; }
        public string? LoadOrderPath { get; init; }
        public string OutputPath { get; init; } = "Data/CalamityAffixes_UserPatch.esp";
        public string? PatchModKey { get; init; }
        public string? BaseModKey { get; init; }
    }

    public static int Main(string[] args)
    {
        var launchedWithoutArgs = args.Length == 0;
        try
        {
            // Double-clicking the published EXE without args is a common user mistake.
            // In that case, show clear guidance instead of failing silently and closing.
            if (launchedWithoutArgs && string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("CALAMITY_MASTERS_DIR")))
            {
                Console.WriteLine("CalamityAffixes.UserPatch");
                Console.WriteLine();
                Console.WriteLine("No arguments were provided.");
                Console.WriteLine("For most users, run 'build_user_patch_wizard.cmd' instead of the EXE directly.");
                Console.WriteLine();
                PrintUsage();
                PauseForInteractiveNoArgsLaunch();
                return 2;
            }

            var options = ParseArgs(args);
            var specPath = Path.GetFullPath(options.SpecPath);
            var outputPath = Path.GetFullPath(options.OutputPath);
            var spec = AffixSpecLoader.Load(specPath);

            var baseModKey = options.BaseModKey ?? spec.ModKey;
            var patchModKey = options.PatchModKey ?? Path.GetFileName(outputPath);
            var mastersDir = ResolveMastersDir(options.MastersDir);
            var loadOrderPath = ResolveLoadOrderPath(options.LoadOrderPath);

            var loadOrder = ReadLoadOrder(loadOrderPath);
            var loadedMods = LoadMods(mastersDir, loadOrder, baseModKey);
            if (loadedMods.Count == 0)
            {
                throw new InvalidDataException("No plugins were loaded for user patch generation.");
            }

            var cache = loadedMods.ToImmutableLinkCache();
            Func<FormKey, ILeveledItemGetter?> leveledListResolver =
                formKey => cache.TryResolve<ILeveledItemGetter>(formKey, out var record) ? record : null;

            var runewordDropList = ResolveLeveledListByEditorId(loadedMods, RunewordDropListEditorId, baseModKey)
                ?? throw new InvalidDataException(
                    $"Could not resolve '{RunewordDropListEditorId}'. Ensure {baseModKey} is active in load order.");
            var reforgeDropList = ResolveLeveledListByEditorId(loadedMods, ReforgeDropListEditorId, baseModKey)
                ?? throw new InvalidDataException(
                    $"Could not resolve '{ReforgeDropListEditorId}'. Ensure {baseModKey} is active in load order.");

            var targetLists = ResolveUserPatchTargets(spec, loadedMods, baseModKey);
            if (targetLists.Count == 0)
            {
                throw new InvalidDataException(
                    "No non-vanilla DeathItem* targets were discovered. " +
                    "If you expect mod-added enemies, verify loadorder/plugins and rerun.");
            }

            var patch = CurrencyUserPatchBuilder.Build(
                patchModKey,
                runewordDropList.FormKey,
                reforgeDropList.FormKey,
                targetLists,
                leveledListResolver);

            Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
            ((IModGetter)patch).WriteToBinary(new FilePath(outputPath));

            Console.WriteLine("CalamityAffixes.UserPatch complete:");
            Console.WriteLine($"- Spec: {specPath}");
            Console.WriteLine($"- Masters: {mastersDir}");
            Console.WriteLine($"- LoadOrder: {loadOrderPath}");
            Console.WriteLine($"- BaseMod: {baseModKey}");
            Console.WriteLine($"- PatchMod: {patchModKey}");
            Console.WriteLine($"- Targets: {targetLists.Count}");
            Console.WriteLine($"- Output: {outputPath}");
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"ERROR: {ex.Message}");
            if (launchedWithoutArgs)
            {
                Console.Error.WriteLine("Tip: launch build_user_patch_wizard.cmd for guided mode.");
                PauseForInteractiveNoArgsLaunch();
            }
            return 1;
        }
    }

    private static void PauseForInteractiveNoArgsLaunch()
    {
        if (
            !OperatingSystem.IsWindows() ||
            !Environment.UserInteractive ||
            Console.IsInputRedirected ||
            Console.IsOutputRedirected)
        {
            return;
        }

        Console.WriteLine();
        Console.Write("Press Enter to close...");
        _ = Console.ReadLine();
    }

    private static Options ParseArgs(string[] args)
    {
        string specPath = "affixes/affixes.json";
        string? mastersDir = Environment.GetEnvironmentVariable("CALAMITY_MASTERS_DIR");
        string? loadOrderPath = Environment.GetEnvironmentVariable("CALAMITY_LOADORDER_PATH");
        string outputPath = "Data/CalamityAffixes_UserPatch.esp";
        string? patchModKey = null;
        string? baseModKey = null;

        for (var i = 0; i < args.Length; i++)
        {
            var arg = args[i];
            switch (arg)
            {
                case "--spec" when i + 1 < args.Length:
                    specPath = args[++i];
                    break;
                case "--masters" when i + 1 < args.Length:
                case "-d" when i + 1 < args.Length:
                case "--DataFolderPath" when i + 1 < args.Length:
                    mastersDir = args[++i];
                    break;
                case "--loadorder" when i + 1 < args.Length:
                case "-l" when i + 1 < args.Length:
                case "--LoadOrderFilePath" when i + 1 < args.Length:
                    loadOrderPath = args[++i];
                    break;
                case "--output" when i + 1 < args.Length:
                case "-o" when i + 1 < args.Length:
                case "--OutputPath" when i + 1 < args.Length:
                    outputPath = args[++i];
                    break;
                case "--patch-mod-key" when i + 1 < args.Length:
                    patchModKey = args[++i];
                    break;
                case "--base-mod-key" when i + 1 < args.Length:
                    baseModKey = args[++i];
                    break;
                case "-h":
                case "--help":
                    PrintUsage();
                    Environment.Exit(0);
                    break;
                default:
                    throw new InvalidDataException($"Unknown or incomplete argument: {arg}");
            }
        }

        return new Options
        {
            SpecPath = specPath,
            MastersDir = mastersDir,
            LoadOrderPath = loadOrderPath,
            OutputPath = outputPath,
            PatchModKey = patchModKey,
            BaseModKey = baseModKey,
        };
    }

    private static void PrintUsage()
    {
        Console.WriteLine("CalamityAffixes.UserPatch");
        Console.WriteLine();
        Console.WriteLine("Creates CalamityAffixes_UserPatch.esp from active load order.");
        Console.WriteLine("The patch injects currency drop lists into mod-added DeathItem* leveled lists.");
        Console.WriteLine();
        Console.WriteLine("Usage:");
        Console.WriteLine("  dotnet run --project tools/CalamityAffixes.UserPatch -- [options]");
        Console.WriteLine();
        Console.WriteLine("Options:");
        Console.WriteLine("  --spec <path>                Spec JSON (default: affixes/affixes.json)");
        Console.WriteLine("  --masters <path>             Skyrim Data folder with plugin files");
        Console.WriteLine("  --loadorder <path>           plugins.txt or loadorder.txt");
        Console.WriteLine("  --output <path>              Output ESP path (default: Data/CalamityAffixes_UserPatch.esp)");
        Console.WriteLine("  --patch-mod-key <file>       Output plugin file name (default: output file name)");
        Console.WriteLine("  --base-mod-key <file>        Base Calamity plugin (default: spec.modKey)");
        Console.WriteLine();
        Console.WriteLine("Synthesis-compatible aliases:");
        Console.WriteLine("  -d / --DataFolderPath");
        Console.WriteLine("  -l / --LoadOrderFilePath");
        Console.WriteLine("  -o / --OutputPath");
        Console.WriteLine();
        Console.WriteLine("Env fallback:");
        Console.WriteLine("  CALAMITY_MASTERS_DIR, CALAMITY_LOADORDER_PATH");
    }

    private static string ResolveMastersDir(string? mastersDir)
    {
        if (string.IsNullOrWhiteSpace(mastersDir))
        {
            throw new InvalidDataException(
                "Masters directory is required. Pass --masters <SkyrimDataPath> or set CALAMITY_MASTERS_DIR.");
        }

        var resolved = Path.GetFullPath(mastersDir);
        if (!Directory.Exists(resolved))
        {
            throw new DirectoryNotFoundException($"Masters directory not found: {resolved}");
        }

        return resolved;
    }

    private static string ResolveLoadOrderPath(string? loadOrderPath)
    {
        if (!string.IsNullOrWhiteSpace(loadOrderPath))
        {
            var explicitPath = Path.GetFullPath(loadOrderPath);
            if (!File.Exists(explicitPath))
            {
                throw new FileNotFoundException($"Load order file not found: {explicitPath}");
            }

            return explicitPath;
        }

        var localAppData = Environment.GetEnvironmentVariable("LOCALAPPDATA");
        if (!string.IsNullOrWhiteSpace(localAppData))
        {
            var pluginsTxt = Path.Combine(localAppData, "Skyrim Special Edition", "plugins.txt");
            if (File.Exists(pluginsTxt))
            {
                return pluginsTxt;
            }

            var loadOrderTxt = Path.Combine(localAppData, "Skyrim Special Edition", "loadorder.txt");
            if (File.Exists(loadOrderTxt))
            {
                return loadOrderTxt;
            }
        }

        throw new InvalidDataException(
            "Load order file is required. Pass --loadorder <plugins.txt|loadorder.txt> " +
            "or set CALAMITY_LOADORDER_PATH.");
    }

    private static IReadOnlyList<string> ReadLoadOrder(string path)
    {
        var plugins = new List<string>();
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (var rawLine in File.ReadAllLines(path))
        {
            var line = rawLine.Trim();
            if (line.Length == 0 || line.StartsWith('#'))
            {
                continue;
            }

            if (line.StartsWith('-'))
            {
                continue;
            }

            if (line.StartsWith('*') || line.StartsWith('+'))
            {
                line = line[1..].Trim();
            }

            if (line.EndsWith(".ghost", StringComparison.OrdinalIgnoreCase))
            {
                line = line[..^".ghost".Length];
            }

            if (!line.EndsWith(".esm", StringComparison.OrdinalIgnoreCase) &&
                !line.EndsWith(".esp", StringComparison.OrdinalIgnoreCase) &&
                !line.EndsWith(".esl", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            if (seen.Add(line))
            {
                plugins.Add(line);
            }
        }

        return plugins;
    }

    private static IReadOnlyList<ISkyrimModGetter> LoadMods(
        string mastersDir,
        IReadOnlyList<string> loadOrder,
        string baseModKey)
    {
        var pluginsToLoad = new List<string>();
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (var official in OfficialMasterFileNames)
        {
            if (seen.Add(official))
            {
                pluginsToLoad.Add(official);
            }
        }

        foreach (var plugin in loadOrder)
        {
            if (seen.Add(plugin))
            {
                pluginsToLoad.Add(plugin);
            }
        }

        if (seen.Add(baseModKey))
        {
            pluginsToLoad.Add(baseModKey);
        }

        var loaded = new List<ISkyrimModGetter>();
        foreach (var pluginFileName in pluginsToLoad)
        {
            var pluginPath = Path.Combine(mastersDir, pluginFileName);
            if (!File.Exists(pluginPath))
            {
                if (string.Equals(pluginFileName, "Skyrim.esm", StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(pluginFileName, baseModKey, StringComparison.OrdinalIgnoreCase))
                {
                    throw new FileNotFoundException(
                        $"Required plugin '{pluginFileName}' was not found in masters dir '{mastersDir}'.");
                }

                continue;
            }

            try
            {
                loaded.Add(SkyrimMod.CreateFromBinary(pluginPath, SkyrimRelease.SkyrimSE));
            }
            catch (Exception ex)
            {
                if (string.Equals(pluginFileName, "Skyrim.esm", StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(pluginFileName, baseModKey, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException(
                        $"Failed to load required plugin '{pluginFileName}' from '{mastersDir}'.",
                        ex);
                }
            }
        }

        return loaded;
    }

    private static IReadOnlyList<FormKey> ResolveUserPatchTargets(
        AffixSpec spec,
        IReadOnlyList<ISkyrimModGetter> loadedMods,
        string baseModKey)
    {
        var targets = new List<FormKey>();
        var seen = new HashSet<FormKey>();

        foreach (var explicitTarget in ParseExplicitTargets(spec.Loot))
        {
            if (ShouldIncludeForUserPatch(explicitTarget, baseModKey) && seen.Add(explicitTarget))
            {
                targets.Add(explicitTarget);
            }
        }

        if (spec.Loot?.CurrencyLeveledListAutoDiscoverDeathItems != false)
        {
            foreach (var discovered in DiscoverModDeathItemTargets(loadedMods, baseModKey))
            {
                if (seen.Add(discovered))
                {
                    targets.Add(discovered);
                }
            }
        }

        targets.Sort((left, right) =>
        {
            var byMod = StringComparer.OrdinalIgnoreCase.Compare(
                left.ModKey.FileName.String,
                right.ModKey.FileName.String);
            return byMod != 0 ? byMod : left.ID.CompareTo(right.ID);
        });

        return targets;
    }

    private static IEnumerable<FormKey> ParseExplicitTargets(LootSpec? loot)
    {
        if (loot?.CurrencyLeveledListTargets is null)
        {
            yield break;
        }

        foreach (var raw in loot.CurrencyLeveledListTargets)
        {
            if (string.IsNullOrWhiteSpace(raw))
            {
                continue;
            }

            yield return ParseFormKey(raw.Trim(), "loot.currencyLeveledListTargets");
        }
    }

    private static FormKey ParseFormKey(string value, string fieldName)
    {
        var split = value.Split('|', 2, StringSplitOptions.TrimEntries);
        if (split.Length != 2 || string.IsNullOrWhiteSpace(split[0]) || string.IsNullOrWhiteSpace(split[1]))
        {
            throw new InvalidDataException($"{fieldName} entry must be 'ModName.esm|00ABCDEF' (got: {value}).");
        }

        var modKey = ModKey.FromNameAndExtension(split[0]);
        var formIdText = split[1].StartsWith("0x", StringComparison.OrdinalIgnoreCase)
            ? split[1][2..]
            : split[1];
        if (!uint.TryParse(formIdText, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var formId))
        {
            throw new InvalidDataException($"{fieldName} entry has invalid FormID hex value: {value}");
        }

        return new FormKey(modKey, formId);
    }

    private static ILeveledItemGetter? ResolveLeveledListByEditorId(
        IReadOnlyList<ISkyrimModGetter> loadedMods,
        string editorId,
        string baseModKey)
    {
        var preferred = loadedMods.FirstOrDefault(mod =>
            string.Equals(mod.ModKey.FileName.String, baseModKey, StringComparison.OrdinalIgnoreCase));
        if (preferred is not null)
        {
            var inPreferred = preferred.LeveledItems
                .FirstOrDefault(record => string.Equals(record.EditorID, editorId, StringComparison.OrdinalIgnoreCase));
            if (inPreferred is not null)
            {
                return inPreferred;
            }
        }

        for (var i = loadedMods.Count - 1; i >= 0; i--)
        {
            var resolved = loadedMods[i].LeveledItems
                .FirstOrDefault(record => string.Equals(record.EditorID, editorId, StringComparison.OrdinalIgnoreCase));
            if (resolved is not null)
            {
                return resolved;
            }
        }

        return null;
    }

    private static IEnumerable<FormKey> DiscoverModDeathItemTargets(
        IReadOnlyList<ISkyrimModGetter> loadedMods,
        string baseModKey)
    {
        foreach (var mod in loadedMods)
        {
            if (IsOfficialMaster(mod.ModKey.FileName.String) ||
                string.Equals(mod.ModKey.FileName.String, baseModKey, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            foreach (var leveledItem in mod.LeveledItems)
            {
                if (leveledItem.FormKey.ModKey != mod.ModKey)
                {
                    continue;
                }

                if (!string.IsNullOrWhiteSpace(leveledItem.EditorID) &&
                    leveledItem.EditorID.StartsWith("DeathItem", StringComparison.OrdinalIgnoreCase))
                {
                    yield return leveledItem.FormKey;
                }
            }
        }
    }

    private static bool ShouldIncludeForUserPatch(FormKey target, string baseModKey)
    {
        var modFileName = target.ModKey.FileName.String;
        if (IsOfficialMaster(modFileName))
        {
            return false;
        }

        return !string.Equals(modFileName, baseModKey, StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsOfficialMaster(string fileName)
    {
        return OfficialMasterFileNames.Contains(fileName, StringComparer.OrdinalIgnoreCase);
    }
}
