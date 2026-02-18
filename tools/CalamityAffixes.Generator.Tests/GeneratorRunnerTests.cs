using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using System.Text;
using System.Text.Json;

namespace CalamityAffixes.Generator.Tests;

public sealed class GeneratorRunnerTests
{
    [Fact]
    public void Generate_WritesKidIniAndKeywordPlugin()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes.esp",
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
                KidRules = [],
                SpidRules = [],
            },
        };

        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        var dataDir = Path.Combine(tempRoot, "Data");
        Directory.CreateDirectory(dataDir);

        try
        {
            GeneratorRunner.Generate(spec, dataDir);

            Assert.True(File.Exists(Path.Combine(dataDir, "CalamityAffixes_KID.ini")));
            Assert.True(File.Exists(Path.Combine(dataDir, spec.ModKey)));
            Assert.False(File.Exists(Path.Combine(dataDir, "CalamityAffixes_Keywords.esp")));
            Assert.True(File.Exists(Path.Combine(dataDir, "SKSE", "Plugins", "CalamityAffixes", "affixes.json")));
            Assert.True(File.Exists(Path.Combine(dataDir, "SKSE", "Plugins", "InventoryInjector", "CalamityAffixes.json")));
            Assert.False(Directory.Exists(Path.Combine(dataDir, "Interface")));
        }
        finally
        {
            if (Directory.Exists(tempRoot))
            {
                Directory.Delete(tempRoot, recursive: true);
            }
        }
    }

    [Fact]
    public void Generate_DoesNotWriteLoreBoxTranslations()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes.esp",
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

        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        var dataDir = Path.Combine(tempRoot, "Data");
        Directory.CreateDirectory(dataDir);

        try
        {
            GeneratorRunner.Generate(spec, dataDir);
            Assert.False(Directory.Exists(Path.Combine(dataDir, "Interface")));
        }
        finally
        {
            if (Directory.Exists(tempRoot))
            {
                Directory.Delete(tempRoot, recursive: true);
            }
        }
    }

    [Fact]
    public void Generate_WritesSpidIniRules()
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
                SpidRules =
                [
                    new Dictionary<string, object?>
                    {
                        ["comment"] = "Test SPID rule",
                        ["line"] = "Spell = Fireball|NONE|NONE|NONE|NONE|NONE|100",
                    },
                ],
            },
        };

        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        var dataDir = Path.Combine(tempRoot, "Data");
        Directory.CreateDirectory(dataDir);

        try
        {
            GeneratorRunner.Generate(spec, dataDir);

            var text = File.ReadAllText(Path.Combine(dataDir, "CalamityAffixes_DISTR.ini"));
            Assert.Contains("; Test SPID rule", text);
            Assert.Contains("Spell = Fireball|NONE|NONE|NONE|NONE|NONE|100", text);
        }
        finally
        {
            if (Directory.Exists(tempRoot))
            {
                Directory.Delete(tempRoot, recursive: true);
            }
        }
    }

    [Fact]
    public void Generate_PreservesLootBudgetFieldsInRuntimeJson()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes.esp",
            EslFlag = true,
            Loot = new LootSpec
            {
                ChancePercent = 30.0,
                RunewordFragmentChancePercent = 40.0,
                ReforgeOrbChancePercent = 7.0,
                LootSourceChanceMultCorpse = 0.75,
                LootSourceChanceMultContainer = 1.1,
                LootSourceChanceMultBossContainer = 1.5,
                LootSourceChanceMultWorld = 0.9,
                RenameItem = true,
                SharedPool = true,
                DebugLog = false,
                DotTagSafetyAutoDisable = false,
                DotTagSafetyUniqueEffectThreshold = 96,
                TrapGlobalMaxActive = 64,
                TrapCastBudgetPerTick = 7,
                TriggerProcBudgetPerWindow = 11,
                TriggerProcBudgetWindowMs = 90,
                CleanupInvalidLegacyAffixes = true,
                ArmorEditorIdDenyContains = ["lootbox"],
                BossContainerEditorIdAllowContains = ["bosschest", "boss"],
                BossContainerEditorIdDenyContains = ["fakeboss"],
                NameFormat = "{base}[{affix}]",
            },
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        var dataDir = Path.Combine(tempRoot, "Data");
        Directory.CreateDirectory(dataDir);

        try
        {
            GeneratorRunner.Generate(spec, dataDir);

            var runtimeJsonPath = Path.Combine(dataDir, "SKSE", "Plugins", "CalamityAffixes", "affixes.json");
            Assert.True(File.Exists(runtimeJsonPath));

            using var doc = JsonDocument.Parse(File.ReadAllText(runtimeJsonPath, Encoding.UTF8));
            var loot = doc.RootElement.GetProperty("loot");
            Assert.Equal(7.0, loot.GetProperty("reforgeOrbChancePercent").GetDouble());
            Assert.Equal(0.75, loot.GetProperty("lootSourceChanceMultCorpse").GetDouble());
            Assert.Equal(1.1, loot.GetProperty("lootSourceChanceMultContainer").GetDouble());
            Assert.Equal(1.5, loot.GetProperty("lootSourceChanceMultBossContainer").GetDouble());
            Assert.Equal(0.9, loot.GetProperty("lootSourceChanceMultWorld").GetDouble());
            Assert.Equal(7, loot.GetProperty("trapCastBudgetPerTick").GetInt32());
            Assert.Equal(11, loot.GetProperty("triggerProcBudgetPerWindow").GetInt32());
            Assert.Equal(90, loot.GetProperty("triggerProcBudgetWindowMs").GetInt32());
            Assert.Equal("bosschest", loot.GetProperty("bossContainerEditorIdAllowContains")[0].GetString());
            Assert.Equal("fakeboss", loot.GetProperty("bossContainerEditorIdDenyContains")[0].GetString());
        }
        finally
        {
            if (Directory.Exists(tempRoot))
            {
                Directory.Delete(tempRoot, recursive: true);
            }
        }
    }
}
