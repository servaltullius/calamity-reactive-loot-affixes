using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Writers;

public static class CurrencyUserPatchBuilder
{
    public static SkyrimMod Build(
        string patchModKey,
        FormKey runewordDropListKey,
        FormKey reforgeDropListKey,
        IReadOnlyList<FormKey> targetLeveledListKeys,
        Func<FormKey, ILeveledItemGetter?> leveledListResolver)
    {
        if (string.IsNullOrWhiteSpace(patchModKey))
        {
            throw new InvalidDataException("patchModKey is required.");
        }

        if (!patchModKey.EndsWith(".esp", StringComparison.OrdinalIgnoreCase) &&
            !patchModKey.EndsWith(".esm", StringComparison.OrdinalIgnoreCase) &&
            !patchModKey.EndsWith(".esl", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException($"patchModKey must end with .esp/.esm/.esl (got: {patchModKey}).");
        }

        if (targetLeveledListKeys.Count == 0)
        {
            throw new InvalidDataException("No target leveled lists were provided for currency user patch generation.");
        }

        var mod = new SkyrimMod(ModKey.FromFileName(patchModKey), SkyrimRelease.SkyrimSE);
        // UserPatch only writes leveled-list overrides, so keep it light-plugin flagged (ESPFE)
        // to avoid consuming a full plugin slot.
        mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;
        var runewordDropListLink = new FormLink<IItemGetter>(runewordDropListKey);
        var reforgeDropListLink = new FormLink<IItemGetter>(reforgeDropListKey);

        foreach (var targetListKey in DistinctSortedTargets(targetLeveledListKeys))
        {
            var targetList = GetOrAddLeveledListOverride(mod, targetListKey, leveledListResolver);
            AddLeveledItemEntryIfMissing(targetList, runewordDropListLink, level: 1, count: 1);
            AddLeveledItemEntryIfMissing(targetList, reforgeDropListLink, level: 1, count: 1);
        }

        return mod;
    }

    private static IReadOnlyList<FormKey> DistinctSortedTargets(IReadOnlyList<FormKey> targets)
    {
        var unique = new List<FormKey>(targets.Count);
        var seen = new HashSet<FormKey>();
        foreach (var target in targets)
        {
            if (seen.Add(target))
            {
                unique.Add(target);
            }
        }

        unique.Sort((left, right) =>
        {
            var byMod = StringComparer.OrdinalIgnoreCase.Compare(
                left.ModKey.FileName.String,
                right.ModKey.FileName.String);
            return byMod != 0 ? byMod : left.ID.CompareTo(right.ID);
        });

        return unique;
    }

    private static LeveledItem GetOrAddLeveledListOverride(
        SkyrimMod mod,
        FormKey targetListFormKey,
        Func<FormKey, ILeveledItemGetter?> leveledListResolver)
    {
        var existing = mod.LeveledItems.FirstOrDefault(record => record.FormKey == targetListFormKey);
        if (existing is not null)
        {
            return existing;
        }

        var baseRecord = leveledListResolver(targetListFormKey);
        if (baseRecord is null)
        {
            throw new InvalidDataException(
                $"Failed to resolve target leveled list from load order: {targetListFormKey}.");
        }

        return mod.LeveledItems.GetOrAddAsOverride(baseRecord);
    }

    private static void AddLeveledItemEntryIfMissing(
        LeveledItem target,
        IFormLink<IItemGetter> reference,
        short level,
        short count)
    {
        target.Entries ??= [];
        var hasEntry = target.Entries.Any(entry => entry.Data?.Reference.FormKey == reference.FormKey);
        if (hasEntry)
        {
            return;
        }

        target.Entries.Add(new LeveledItemEntry
        {
            Data = new LeveledItemEntryData
            {
                Level = level,
                Count = count,
                Reference = reference,
            },
        });
    }
}
