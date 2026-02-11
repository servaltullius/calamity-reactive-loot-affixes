using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Writers;

public static class McmPluginBuilder
{
    private static readonly ModKey SkyrimModKey = ModKey.FromNameAndExtension("Skyrim.esm");

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

        // Keep the player alias for quest-structure stability across save games,
        // but do NOT attach scripts via QuestFragmentAlias in the VMAD.
        // The QuestFragmentAlias serialization caused Wrye Bash parsing exceptions.
        // CalamityAffixes_MCMConfig extends MCM_ConfigBase, so MCM Helper handles
        // game-load events internally — SKI_PlayerLoadGameAlias is not needed.
        var playerAlias = new QuestAlias
        {
            ID = 0,
            Type = QuestAlias.TypeEnum.Reference,
            Name = "PlayerAlias_MCM",
            Flags = QuestAlias.Flag.AllowReserved,
        };
        playerAlias.ForcedReference.SetTo(new FormKey(SkyrimModKey, 0x000014));
        quest.Aliases.Add(playerAlias);
    }
}
