using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Tests;

public sealed class McmPluginBuilderTests
{
    [Fact]
    public void AddMcmQuest_AddsQuestToMainPlugin()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes.esp",
            EslFlag = true,
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        Assert.Equal(ModKey.FromFileName(spec.ModKey), mod.ModKey);

        var quest = Assert.Single(mod.Quests, q => q.EditorID == "CalamityAffixes_MCM");
        Assert.True(quest.Flags.HasFlag(Quest.Flag.StartGameEnabled));
        Assert.NotNull(quest.VirtualMachineAdapter);
        Assert.Contains(quest.VirtualMachineAdapter!.Scripts, s => s.Name == "CalamityAffixes_MCMConfig");

        // Player alias is kept for quest-structure stability, but no scripts are
        // attached via QuestFragmentAlias (avoids Wrye Bash VMAD parsing exception).
        var alias = Assert.Single(quest.Aliases, a => a.Name == "PlayerAlias_MCM");
        Assert.Equal(QuestAlias.TypeEnum.Reference, alias.Type);
        Assert.Equal(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x000014), alias.ForcedReference.FormKeyNullable);
        Assert.Empty(quest.VirtualMachineAdapter.Aliases);
    }
}
