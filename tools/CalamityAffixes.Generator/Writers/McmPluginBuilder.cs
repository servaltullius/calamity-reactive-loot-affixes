using Mutagen.Bethesda;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Writers;

public static class McmPluginBuilder
{

    public static void AddMcmQuest(SkyrimMod mod)
    {
        foreach (var existing in mod.Quests)
        {
            if (string.Equals(existing.EditorID, "CalamityAffixes_MCM", StringComparison.Ordinal))
            {
                return;
            }
        }

        var quest = mod.Quests.AddNew();
        quest.EditorID = "CalamityAffixes_MCM";
        quest.Name = "Calamity Affixes MCM";
        quest.Flags = Quest.Flag.StartGameEnabled;
        quest.Priority = 60;
        quest.Type = Quest.TypeEnum.None;

        quest.VirtualMachineAdapter = new QuestAdapter
        {
            Version = 5,
            ObjectFormat = 2,
        };
        quest.VirtualMachineAdapter.Scripts.Add(new ScriptEntry
        {
            Name = "CalamityAffixes_MCMConfig",
            Flags = ScriptEntry.Flag.Local,
        });

        // NOTE: Player alias with SKI_PlayerLoadGameAlias is intentionally omitted.
        // CalamityAffixes_MCMConfig extends MCM_ConfigBase, which handles game-load
        // events internally. The alias + QuestFragmentAlias VMAD structure caused
        // Wrye Bash parsing exceptions in some environments.
    }
}
