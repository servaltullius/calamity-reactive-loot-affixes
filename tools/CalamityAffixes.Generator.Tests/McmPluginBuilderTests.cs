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

        // Alias with SKI_PlayerLoadGameAlias is intentionally omitted.
        // MCM_ConfigBase (MCM Helper) handles game-load events internally,
        // and the QuestFragmentAlias VMAD structure caused Wrye Bash exceptions.
        Assert.Empty(quest.Aliases);
        Assert.Empty(quest.VirtualMachineAdapter.Aliases);
    }
}
