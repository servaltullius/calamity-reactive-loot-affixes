using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using System.Text.Json;

namespace CalamityAffixes.Generator.Tests;

public sealed class InventoryInjectorJsonWriterTests
{
    [Fact]
    public void Render_DoesNotOverrideSkyUiListEntryText()
    {
        // InventoryInjector's "text" field maps to SkyUI's list entry display name.
        // For CalamityAffixes we keep item names unchanged; affix details are shown via Prisma UI overlay.
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes =
                [
                    new AffixDefinition
                    {
                        Id = "arc_lightning",
                        EditorId = "LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING",
                        Name = "Affix: Arc Lightning",
                        Kid = new KidRule
                        {
                            Type = "Weapon",
                            Strings = "NONE",
                            FormFilters = "NONE",
                            Traits = "-E",
                            Chance = 2.0,
                        },
                        Runtime = new Dictionary<string, object?>(),
                    },
                ],
                KidRules = [],
                SpidRules = [],
            },
        };

        var json = InventoryInjectorJsonWriter.Render(spec);

        using var doc = JsonDocument.Parse(json);
        var rules = doc.RootElement.GetProperty("rules");
        Assert.Equal(0, rules.GetArrayLength());
    }
}
