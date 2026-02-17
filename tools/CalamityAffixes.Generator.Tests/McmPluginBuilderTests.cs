using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Tests;

public sealed class McmPluginBuilderTests
{
    private static AffixSpec CreateSpec(int affixCount = 0)
    {
        var affixes = new List<AffixDefinition>();
        for (var i = 0; i < affixCount; i++)
        {
            affixes.Add(new AffixDefinition
            {
                Id = $"test_affix_{i}",
                EditorId = $"CAFF_TEST_AFFIX_{i}",
                Name = $"Test Affix {i}",
                Kid = new KidRule
                {
                    Type = "Weapon",
                    Strings = "NONE",
                    FormFilters = "NONE",
                    Traits = "-E",
                    Chance = 1.0,
                },
                Runtime = new Dictionary<string, object?>(),
            });
        }

        return new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes.esp",
            EslFlag = true,
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = affixes,
                KidRules = [],
                SpidRules = [],
            },
        };
    }

    [Fact]
    public void AddMcmQuest_AddsQuestToMainPlugin()
    {
        var spec = CreateSpec();

        var mod = KeywordPluginBuilder.Build(spec);

        Assert.Equal(ModKey.FromFileName(spec.ModKey), mod.ModKey);

        var quest = Assert.Single(mod.Quests, q => q.EditorID == "CalamityAffixes_MCM");
        Assert.True(quest.Flags.HasFlag(Quest.Flag.StartGameEnabled));
        Assert.Equal(0x000800u, quest.FormKey.ID);
        Assert.Equal((uint)1, quest.NextAliasID);
        Assert.NotNull(quest.VirtualMachineAdapter);
        Assert.Contains(quest.VirtualMachineAdapter!.Scripts, s => s.Name == "CalamityAffixes_MCMConfig");

        var alias = Assert.Single(quest.Aliases, a => a.Name == "PlayerAlias_MCM");
        Assert.Equal(QuestAlias.TypeEnum.Reference, alias.Type);
        Assert.Equal(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x000014), alias.ForcedReference.FormKeyNullable);

        Assert.Empty(quest.VirtualMachineAdapter.Aliases);
    }

    [Fact]
    public void AddMcmQuest_KeepsStableFormKeyWhenAffixCountChanges()
    {
        var baseMod = KeywordPluginBuilder.Build(CreateSpec(0));
        var expandedMod = KeywordPluginBuilder.Build(CreateSpec(50));

        var baseQuest = Assert.Single(baseMod.Quests, q => q.EditorID == "CalamityAffixes_MCM");
        var expandedQuest = Assert.Single(expandedMod.Quests, q => q.EditorID == "CalamityAffixes_MCM");

        Assert.Equal(baseQuest.FormKey, expandedQuest.FormKey);
    }
}
