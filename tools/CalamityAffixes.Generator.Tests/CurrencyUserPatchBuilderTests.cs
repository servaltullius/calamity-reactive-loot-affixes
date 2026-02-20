using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;
using Noggog;

namespace CalamityAffixes.Generator.Tests;

public sealed class CurrencyUserPatchBuilderTests
{
    [Fact]
    public void Build_InsertsDropListsIntoTargetOverrides()
    {
        var runewordDropListKey = new FormKey(ModKey.FromNameAndExtension("CalamityAffixes.esp"), 0x00000810);
        var reforgeDropListKey = new FormKey(ModKey.FromNameAndExtension("CalamityAffixes.esp"), 0x00000811);
        var targetOne = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x0009AF0A);
        var targetTwo = new FormKey(ModKey.FromNameAndExtension("ExampleCreatures.esp"), 0x00000801);

        var resolver = CreateLeveledListResolver(
            (targetOne, LeveledItem.Flag.UseAll),
            (targetTwo, LeveledItem.Flag.CalculateForEachItemInCount));

        var patch = CurrencyUserPatchBuilder.Build(
            patchModKey: "CalamityAffixes_UserPatch.esp",
            runewordDropListKey,
            reforgeDropListKey,
            [targetOne, targetTwo],
            resolver);

        var one = Assert.Single(patch.LeveledItems, list => list.FormKey == targetOne);
        var two = Assert.Single(patch.LeveledItems, list => list.FormKey == targetTwo);

        Assert.NotNull(one.Entries);
        Assert.NotNull(two.Entries);
        Assert.Contains(one.Entries!, entry => entry.Data?.Reference.FormKey == runewordDropListKey);
        Assert.Contains(one.Entries!, entry => entry.Data?.Reference.FormKey == reforgeDropListKey);
        Assert.Contains(two.Entries!, entry => entry.Data?.Reference.FormKey == runewordDropListKey);
        Assert.Contains(two.Entries!, entry => entry.Data?.Reference.FormKey == reforgeDropListKey);
    }

    [Fact]
    public void Build_WhenTargetListMissingFromResolver_Throws()
    {
        var runewordDropListKey = new FormKey(ModKey.FromNameAndExtension("CalamityAffixes.esp"), 0x00000810);
        var reforgeDropListKey = new FormKey(ModKey.FromNameAndExtension("CalamityAffixes.esp"), 0x00000811);
        var missingTarget = new FormKey(ModKey.FromNameAndExtension("Missing.esp"), 0x00000801);
        Func<FormKey, ILeveledItemGetter?> resolver = _ => null;

        var ex = Assert.Throws<InvalidDataException>(() => CurrencyUserPatchBuilder.Build(
            patchModKey: "CalamityAffixes_UserPatch.esp",
            runewordDropListKey,
            reforgeDropListKey,
            [missingTarget],
            resolver));

        Assert.Contains("Failed to resolve target leveled list", ex.Message);
    }

    [Fact]
    public void Build_WhenTargetHasDuplicateEntries_DoesNotDuplicateDropListReferences()
    {
        var runewordDropListKey = new FormKey(ModKey.FromNameAndExtension("CalamityAffixes.esp"), 0x00000810);
        var reforgeDropListKey = new FormKey(ModKey.FromNameAndExtension("CalamityAffixes.esp"), 0x00000811);
        var target = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x0009AF0A);
        var resolver = CreateLeveledListResolver((target, LeveledItem.Flag.UseAll));

        var patch = CurrencyUserPatchBuilder.Build(
            patchModKey: "CalamityAffixes_UserPatch.esp",
            runewordDropListKey,
            reforgeDropListKey,
            [target, target, target],
            resolver);

        var overrideList = Assert.Single(patch.LeveledItems, list => list.FormKey == target);
        Assert.NotNull(overrideList.Entries);
        Assert.Equal(3, overrideList.Entries!.Count);
        Assert.Equal(1, overrideList.Entries.Count(entry => entry.Data?.Reference.FormKey == runewordDropListKey));
        Assert.Equal(1, overrideList.Entries.Count(entry => entry.Data?.Reference.FormKey == reforgeDropListKey));
    }

    private static Func<FormKey, ILeveledItemGetter?> CreateLeveledListResolver(
        params (FormKey target, LeveledItem.Flag flags)[] targets)
    {
        var skyrimMod = new SkyrimMod(ModKey.FromNameAndExtension("Skyrim.esm"), SkyrimRelease.SkyrimSE);
        var exampleMod = new SkyrimMod(ModKey.FromNameAndExtension("ExampleCreatures.esp"), SkyrimRelease.SkyrimSE);

        var skyrimGold = skyrimMod.MiscItems.AddNew(new FormKey(skyrimMod.ModKey, 0x00000800));
        skyrimGold.EditorID = "Gold001";
        skyrimGold.Name = "Gold";

        var exampleLoot = exampleMod.MiscItems.AddNew(new FormKey(exampleMod.ModKey, 0x00000800));
        exampleLoot.EditorID = "ExampleLoot";
        exampleLoot.Name = "Example Loot";

        var map = new Dictionary<FormKey, ILeveledItemGetter>();
        foreach (var (target, flags) in targets)
        {
            var owner = target.ModKey == exampleMod.ModKey ? exampleMod : skyrimMod;
            var item = target.ModKey == exampleMod.ModKey
                ? exampleLoot.ToLink<IItemGetter>()
                : skyrimGold.ToLink<IItemGetter>();

            var record = owner.LeveledItems.AddNew(target);
            record.EditorID = $"BASE_{target.ID:X8}";
            record.Flags = flags;
            record.ChanceNone = new Percent(0.0);
            record.Entries = [];
            record.Entries.Add(new LeveledItemEntry
            {
                Data = new LeveledItemEntryData
                {
                    Level = 1,
                    Count = 1,
                    Reference = item,
                },
            });
            map[target] = record;
        }

        return key => map.TryGetValue(key, out var value) ? value : null;
    }
}
