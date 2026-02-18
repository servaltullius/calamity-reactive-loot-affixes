using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;

namespace CalamityAffixes.Generator.Tests;

public sealed class KidIniRendererTests
{
    [Fact]
    public void Render_IncludesRuleLinesAndSkipsAffixDistributionLines()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Keywords = new KeywordSpec
            {
                Tags =
                [
                    new KeywordDefinition { EditorId = "CAFF_TAG_DOT", Name = "Calamity: DoT Tag" },
                ],
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
                KidRules =
                [
                    new KidRuleEntry
                    {
                        Comment = "Tag hostile alchemy magic effects (vanilla poisons) for DotApply.",
                        KeywordEditorId = "CAFF_TAG_DOT",
                        Type = "Magic Effect",
                        Strings = "*Alch*",
                        FormFilters = "NONE",
                        Traits = "H",
                        Chance = 100.0,
                    },
                    new KidRuleEntry
                    {
                        Comment = "Unsafe example (should be disabled automatically).",
                        KeywordEditorId = "CAFF_TAG_DOT",
                        Type = "Magic Effect",
                        Strings = "NONE",
                        FormFilters = "NONE",
                        Traits = "NONE",
                        Chance = 100.0,
                    },
                ],
                SpidRules = [],
            },
        };

        var ini = KidIniWriter.Render(spec);

        Assert.Contains("Instance mode: affix keywords are NOT distributed via KID.", ini);
        Assert.DoesNotContain("ExclusiveGroup = CalamityAffixes_Affixes|", ini);
        Assert.DoesNotContain("Keyword = LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING|", ini);
        Assert.Contains("Keyword = CAFF_TAG_DOT|Magic Effect|*Alch*|H|100", ini);
        Assert.DoesNotContain("; Keyword = CAFF_TAG_DOT|Magic Effect|*Alch*|H|100", ini);
        Assert.Contains("WARNING: Skipped unsafe KID rule", ini);
        Assert.DoesNotContain("Keyword = CAFF_TAG_DOT|Magic Effect|NONE|NONE|100", ini);
    }

    [Fact]
    public void Render_WithLootSettings_SkipsAffixDistributionLines()
    {
        // NOTE: JSON-first test to ensure deserialization stays compatible with on-disk spec format.
        const string json = """
            {
              "version": 1,
              "modKey": "CalamityAffixes_Keywords.esp",
              "eslFlag": true,
              "loot": {
                "chancePercent": 25.0,
                "renameItem": true,
                "nameMarkerPosition": "trailing",
                "nameFormat": "{base} [{affix}]"
              },
              "keywords": {
                "tags": [
                  { "editorId": "CAFF_TAG_DOT", "name": "Calamity: DoT Tag" }
                ],
                "affixes": [
                  {
                    "id": "arc_lightning",
                    "editorId": "LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING",
                    "name": "Affix: Arc Lightning",
                    "kid": {
                      "type": "Weapon",
                      "strings": "NONE",
                      "formFilters": "NONE",
                      "traits": "-E",
                      "chance": 2.0
                    },
                    "runtime": {
                      "trigger": "Hit",
                      "action": {
                        "type": "DebugNotify"
                      }
                    }
                  }
                ],
                "kidRules": [
                  {
                    "comment": "Tag hostile alchemy magic effects (vanilla poisons) for DotApply.",
                    "keywordEditorId": "CAFF_TAG_DOT",
                    "type": "Magic Effect",
                    "strings": "*Alch*",
                    "formFilters": "NONE",
                    "traits": "H",
                    "chance": 100.0
                  }
                ],
                "spidRules": []
              }
            }
            """;

        var path = Path.GetTempFileName();
        try
        {
            File.WriteAllText(path, json);
            var spec = AffixSpecLoader.Load(path);

            var ini = KidIniWriter.Render(spec);

            Assert.Contains("Instance mode: affix keywords are NOT distributed via KID.", ini);
            Assert.DoesNotContain("ExclusiveGroup = CalamityAffixes_Affixes|", ini);
            Assert.DoesNotContain("Keyword = LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING|", ini);
            Assert.Contains("Keyword = CAFF_TAG_DOT|Magic Effect|*Alch*|H|100", ini);
            Assert.DoesNotContain("; Keyword = CAFF_TAG_DOT|Magic Effect|*Alch*|H|100", ini);
        }
        finally
        {
            File.Delete(path);
        }
    }
}
