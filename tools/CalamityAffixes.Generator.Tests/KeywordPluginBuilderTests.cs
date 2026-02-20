using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;
using Noggog;

namespace CalamityAffixes.Generator.Tests;

public sealed class KeywordPluginBuilderTests
{
    [Fact]
    public void BuildKeywordPlugin_CreatesKeywordsWithEditorIds()
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
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        Assert.Contains(mod.Keywords, kw => kw.EditorID == "CAFF_TAG_DOT");
        Assert.Contains(mod.Keywords, kw => kw.EditorID == "LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING");
    }

    [Fact]
    public void BuildKeywordPlugin_CreatesRunewordRuneFragmentMiscItems()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
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

        const string expectedModel = @"Meshes\Clutter\SoulGem\SoulGemPiece01.nif";

        var expected = new (string EditorId, string Rune)[]
        {
            ("CAFF_RuneFrag_El", "El"),
            ("CAFF_RuneFrag_Eld", "Eld"),
            ("CAFF_RuneFrag_Tir", "Tir"),
            ("CAFF_RuneFrag_Ral", "Ral"),
            ("CAFF_RuneFrag_Nef", "Nef"),
            ("CAFF_RuneFrag_Eth", "Eth"),
            ("CAFF_RuneFrag_Ith", "Ith"),
            ("CAFF_RuneFrag_Tal", "Tal"),
            ("CAFF_RuneFrag_Ort", "Ort"),
            ("CAFF_RuneFrag_Thul", "Thul"),
            ("CAFF_RuneFrag_Amn", "Amn"),
            ("CAFF_RuneFrag_Sol", "Sol"),
            ("CAFF_RuneFrag_Shael", "Shael"),
            ("CAFF_RuneFrag_Dol", "Dol"),
            ("CAFF_RuneFrag_Hel", "Hel"),
            ("CAFF_RuneFrag_Io", "Io"),
            ("CAFF_RuneFrag_Lum", "Lum"),
            ("CAFF_RuneFrag_Ko", "Ko"),
            ("CAFF_RuneFrag_Fal", "Fal"),
            ("CAFF_RuneFrag_Lem", "Lem"),
            ("CAFF_RuneFrag_Pul", "Pul"),
            ("CAFF_RuneFrag_Um", "Um"),
            ("CAFF_RuneFrag_Mal", "Mal"),
            ("CAFF_RuneFrag_Ist", "Ist"),
            ("CAFF_RuneFrag_Gul", "Gul"),
            ("CAFF_RuneFrag_Vex", "Vex"),
            ("CAFF_RuneFrag_Ohm", "Ohm"),
            ("CAFF_RuneFrag_Lo", "Lo"),
            ("CAFF_RuneFrag_Sur", "Sur"),
            ("CAFF_RuneFrag_Ber", "Ber"),
            ("CAFF_RuneFrag_Jah", "Jah"),
            ("CAFF_RuneFrag_Cham", "Cham"),
            ("CAFF_RuneFrag_Zod", "Zod"),
        };

        foreach (var (editorId, rune) in expected)
        {
            var item = Assert.Single(mod.MiscItems, misc => misc.EditorID == editorId);
            Assert.Equal($"Rune Fragment: {rune}", item.Name?.String);
            Assert.NotNull(item.Model);
            Assert.Equal(expectedModel, item.Model!.File.ToString());
        }
    }

    [Fact]
    public void BuildKeywordPlugin_CreatesReforgeOrbMiscItem()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
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

        var orb = Assert.Single(mod.MiscItems, item => item.EditorID == "CAFF_Misc_ReforgeOrb");
        Assert.Equal("Reforge Orb", orb.Name?.String);
        Assert.NotNull(orb.Model);
        Assert.Equal(@"Meshes\Clutter\SoulGem\SoulGemPiece01.nif", orb.Model!.File.ToString());
    }

    [Fact]
    public void BuildKeywordPlugin_WhenCurrencyDropModeIsLeveledList_CreatesAndInjectsLeveledLists()
    {
        var lItemGemsFormKey = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x00068525);
        var deathItemBanditFormKey = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x000C3C9B);
        var resolver = CreateLeveledListResolver(
            (lItemGemsFormKey, LeveledItem.Flag.CalculateForEachItemInCount, new Percent(0.0)),
            (deathItemBanditFormKey, LeveledItem.Flag.UseAll, new Percent(0.0)));

        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Loot = new LootSpec
            {
                RunewordFragmentChancePercent = 6.0,
                ReforgeOrbChancePercent = 2.0,
                CurrencyDropMode = "leveledList",
                CurrencyLeveledListTargets =
                [
                    "Skyrim.esm|00068525",
                    "Skyrim.esm|000C3C9B",
                ],
            },
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec, resolver);

        var runewordDropList = Assert.Single(mod.LeveledItems, list => list.EditorID == "CAFF_LItem_RunewordFragmentDrops");
        var reforgeDropList = Assert.Single(mod.LeveledItems, list => list.EditorID == "CAFF_LItem_ReforgeOrbDrops");
        Assert.Equal(new Percent(0.94), runewordDropList.ChanceNone);
        Assert.Equal(new Percent(0.98), reforgeDropList.ChanceNone);

        var lItemGemsOverride = Assert.Single(mod.LeveledItems, list => list.FormKey == lItemGemsFormKey);
        Assert.NotNull(lItemGemsOverride.Entries);
        Assert.True(lItemGemsOverride.Flags.HasFlag(LeveledItem.Flag.CalculateForEachItemInCount));
        Assert.Equal(3, lItemGemsOverride.Entries!.Count);
        Assert.Contains(lItemGemsOverride.Entries, entry => entry.Data?.Reference.FormKey == new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x00000800));
        Assert.Contains(
            lItemGemsOverride.Entries,
            entry => entry.Data?.Reference.FormKey == runewordDropList.FormKey);
        Assert.Contains(
            lItemGemsOverride.Entries,
            entry => entry.Data?.Reference.FormKey == reforgeDropList.FormKey);

        // Curated defaults should include hostile death-item routing for true world-drop feel.
        var deathItemBanditOverride = Assert.Single(mod.LeveledItems, list => list.FormKey == deathItemBanditFormKey);
        Assert.NotNull(deathItemBanditOverride.Entries);
        Assert.True(deathItemBanditOverride.Flags.HasFlag(LeveledItem.Flag.UseAll));
        Assert.Equal(3, deathItemBanditOverride.Entries!.Count);
        Assert.Contains(deathItemBanditOverride.Entries, entry => entry.Data?.Reference.FormKey == new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x00000800));
        Assert.Contains(
            deathItemBanditOverride.Entries,
            entry => entry.Data?.Reference.FormKey == runewordDropList.FormKey);
        Assert.Contains(
            deathItemBanditOverride.Entries,
            entry => entry.Data?.Reference.FormKey == reforgeDropList.FormKey);
    }

    [Fact]
    public void BuildKeywordPlugin_WhenCurrencyDropModeIsRuntime_CreatesCurrencyDropListsWithoutMasterInjection()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Loot = new LootSpec
            {
                RunewordFragmentChancePercent = 16.0,
                ReforgeOrbChancePercent = 10.0,
                CurrencyDropMode = "runtime",
            },
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        var runewordDropList = Assert.Single(mod.LeveledItems, list => list.EditorID == "CAFF_LItem_RunewordFragmentDrops");
        var reforgeDropList = Assert.Single(mod.LeveledItems, list => list.EditorID == "CAFF_LItem_ReforgeOrbDrops");

        Assert.Equal(new Percent(0.84), runewordDropList.ChanceNone);
        Assert.Equal(new Percent(0.90), reforgeDropList.ChanceNone);
        Assert.Equal(2, mod.LeveledItems.Count);
    }

    [Fact]
    public void BuildKeywordPlugin_WhenCurrencyTargetsProvided_UsesCustomTargetOnly()
    {
        var customTarget = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x0009AF0A);
        var resolver = CreateLeveledListResolver((customTarget, LeveledItem.Flag.UseAll, new Percent(0.0)));

        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Loot = new LootSpec
            {
                CurrencyDropMode = "leveledList",
                CurrencyLeveledListTargets = ["Skyrim.esm|0009AF0A"],
            },
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec, resolver);

        var defaultTarget = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x00068525);

        Assert.Contains(mod.LeveledItems, list => list.FormKey == customTarget);
        Assert.DoesNotContain(mod.LeveledItems, list => list.FormKey == defaultTarget);
    }

    [Fact]
    public void BuildKeywordPlugin_WhenAutoDiscoveredTargetsProvided_MergesWithConfiguredTargets()
    {
        var configuredTarget = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x0009AF0A);
        var autoTarget = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x000C3C9B);
        var resolver = CreateLeveledListResolver(
            (configuredTarget, LeveledItem.Flag.UseAll, new Percent(0.0)),
            (autoTarget, LeveledItem.Flag.UseAll, new Percent(0.0)));

        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Loot = new LootSpec
            {
                CurrencyDropMode = "leveledList",
                CurrencyLeveledListTargets = ["Skyrim.esm|0009AF0A"],
            },
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec, resolver, [autoTarget]);

        Assert.Contains(mod.LeveledItems, list => list.FormKey == configuredTarget);
        Assert.Contains(mod.LeveledItems, list => list.FormKey == autoTarget);
    }

    [Fact]
    public void BuildKeywordPlugin_WhenLeveledListModeAndResolverMissing_Throws()
    {
        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes_Keywords.esp",
            EslFlag = true,
            Loot = new LootSpec
            {
                CurrencyDropMode = "leveledList",
                CurrencyLeveledListTargets = ["Skyrim.esm|00068525"],
            },
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes = [],
                KidRules = [],
                SpidRules = [],
            },
        };

        var ex = Assert.Throws<InvalidDataException>(() => KeywordPluginBuilder.Build(spec));
        Assert.Contains("requires a leveled-list resolver", ex.Message);
    }

    [Fact]
    public void BuildKeywordPlugin_CreatesGeneratedMagicEffectAndSpellRecords()
    {
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
                        Id = "test_affix",
                        EditorId = "LoreBox_CAFF_AFFIX_TEST",
                        Name = "Affix: Test",
                        Records = new AffixRecordSpec
                        {
                            MagicEffect = new MagicEffectRecordSpec
                            {
                                EditorId = "CAFF_MGEF_TEST",
                                Name = "Calamity: Test Effect",
                                ActorValue = "Health",
                                Hostile = true,
                                ResistValue = "ResistShock",
                            },
                            Spell = new SpellRecordSpec
                            {
                                EditorId = "CAFF_SPEL_TEST",
                                Name = "Calamity: Test Spell",
                                Delivery = "TargetActor",
                                Effect = new SpellEffectRecordSpec
                                {
                                    MagicEffectEditorId = "CAFF_MGEF_TEST",
                                    Magnitude = 10,
                                    Duration = 1,
                                    Area = 0,
                                },
                            },
                        },
                        Kid = new KidRule
                        {
                            Type = "Weapon",
                            Strings = "NONE",
                            FormFilters = "NONE",
                            Traits = "-E",
                            Chance = 100.0,
                        },
                        Runtime = new Dictionary<string, object?>(),
                    },
                ],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        var mgef = Assert.Single(mod.MagicEffects, m => m.EditorID == "CAFF_MGEF_TEST");
        var spell = Assert.Single(mod.Spells, s => s.EditorID == "CAFF_SPEL_TEST");

        var archetype = Assert.IsType<MagicEffectArchetype>(mgef.Archetype);
        Assert.Equal(MagicEffectArchetype.TypeEnum.ValueModifier, archetype.Type);
        Assert.Equal(ActorValue.Health, archetype.ActorValue);
        Assert.True(mgef.Flags.HasFlag(MagicEffect.Flag.Hostile));
        Assert.Equal(CastType.FireAndForget, mgef.CastType);
        Assert.Equal(TargetType.TargetActor, mgef.TargetType);
        Assert.Equal(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x057C67), mgef.HitShader.FormKey);
        Assert.Equal(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x038B05), mgef.ImpactData.FormKey);

        Assert.Equal(TargetType.TargetActor, spell.TargetType);
        Assert.Equal(CastType.FireAndForget, spell.CastType);
        Assert.True(spell.Range > 0, "TargetActor spells must have non-zero range (or in-game casting can silently fail).");

        var effect = Assert.Single(spell.Effects);
        Assert.Equal(mgef.FormKey, effect.BaseEffect.FormKey);
        var data = effect.Data;
        Assert.NotNull(data);
        Assert.Equal(10f, data!.Magnitude);
        Assert.Equal(1, data.Duration);
        Assert.Equal(0, data.Area);
    }

    [Fact]
    public void BuildKeywordPlugin_WhenMagicEffectRecoverTrue_SetsRecoverFlag()
    {
        var json = """
                   {
                     "version": 1,
                     "modKey": "CalamityAffixes_Keywords.esp",
                     "eslFlag": true,
                     "keywords": {
                       "tags": [],
                       "affixes": [
                         {
                           "id": "test_affix",
                           "editorId": "LoreBox_CAFF_AFFIX_TEST",
                           "name": "Affix: Test",
                           "records": {
                             "magicEffect": {
                               "editorId": "CAFF_MGEF_TEST",
                               "actorValue": "ResistFire",
                               "hostile": true,
                               "recover": true
                             },
                             "spell": {
                               "editorId": "CAFF_SPEL_TEST",
                               "delivery": "TargetActor",
                               "effect": {
                                 "magicEffectEditorId": "CAFF_MGEF_TEST",
                                 "magnitude": 10,
                                 "duration": 3,
                                 "area": 0
                               }
                             }
                           },
                           "kid": {
                             "type": "Weapon",
                             "strings": "NONE",
                             "formFilters": "NONE",
                             "traits": "-E",
                             "chance": 100.0
                           },
                           "runtime": {
                             "trigger": "Hit",
                             "action": {
                               "type": "DebugNotify"
                             }
                           }
                         }
                       ],
                       "kidRules": [],
                       "spidRules": []
                     }
                   }
                   """;

        var path = Path.Combine(Path.GetTempPath(), $"calamity-affixspec-{Guid.NewGuid():N}.json");
        File.WriteAllText(path, json);

        try
        {
            var spec = AffixSpecLoader.Load(path);
            var mod = KeywordPluginBuilder.Build(spec);

            var mgef = Assert.Single(mod.MagicEffects, m => m.EditorID == "CAFF_MGEF_TEST");
            Assert.True(mgef.Flags.HasFlag(MagicEffect.Flag.Recover));
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void BuildKeywordPlugin_WhenMagicEffectHasResistValueAndMagicSkill_SetsFields()
    {
        var json = """
                   {
                     "version": 1,
                     "modKey": "CalamityAffixes_Keywords.esp",
                     "eslFlag": true,
                     "keywords": {
                       "tags": [],
                       "affixes": [
                         {
                           "id": "test_affix",
                           "editorId": "LoreBox_CAFF_AFFIX_TEST",
                           "name": "Affix: Test",
                           "records": {
                             "magicEffect": {
                               "editorId": "CAFF_MGEF_TEST",
                               "name": "Calamity: Test Effect",
                               "actorValue": "Health",
                               "hostile": true,
                               "resistValue": "ResistFire",
                               "magicSkill": "Destruction"
                             },
                             "spell": {
                               "editorId": "CAFF_SPEL_TEST",
                               "name": "Calamity: Test Spell",
                               "delivery": "TargetActor",
                               "effect": {
                                 "magicEffectEditorId": "CAFF_MGEF_TEST",
                                 "magnitude": 10,
                                 "duration": 0,
                                 "area": 0
                               }
                             }
                           },
                           "kid": {
                             "type": "Weapon",
                             "strings": "NONE",
                             "formFilters": "NONE",
                             "traits": "-E",
                             "chance": 100.0
                           },
                           "runtime": {
                             "trigger": "Hit",
                             "action": {
                               "type": "DebugNotify"
                             }
                           }
                         }
                       ],
                       "kidRules": [],
                       "spidRules": []
                     }
                   }
                   """;

        var path = Path.Combine(Path.GetTempPath(), $"calamity-affixspec-{Guid.NewGuid():N}.json");
        File.WriteAllText(path, json);

        try
        {
            var spec = AffixSpecLoader.Load(path);
            var mod = KeywordPluginBuilder.Build(spec);

            var mgef = Assert.Single(mod.MagicEffects, m => m.EditorID == "CAFF_MGEF_TEST");
            Assert.Equal("Calamity: Test Effect", mgef.Name?.String);
            Assert.Equal(ActorValue.ResistFire, mgef.ResistValue);
            Assert.Equal(ActorValue.Destruction, mgef.MagicSkill);

            var spell = Assert.Single(mod.Spells, s => s.EditorID == "CAFF_SPEL_TEST");
            Assert.Equal("Calamity: Test Spell", spell.Name?.String);
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void BuildKeywordPlugin_WhenArchetypeInvisibility_SetsInvisibilityArchetype()
    {
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
                        Id = "test_invisibility",
                        EditorId = "LoreBox_CAFF_AFFIX_TEST_INVIS",
                        Name = "Affix: Test Invisibility",
                        Records = new AffixRecordSpec
                        {
                            MagicEffect = new MagicEffectRecordSpec
                            {
                                EditorId = "CAFF_MGEF_TEST_INVIS",
                                Name = "Calamity: Test Invisibility",
                                ActorValue = "Invisibility",
                                Archetype = "Invisibility",
                                Hostile = false,
                            },
                            Spell = new SpellRecordSpec
                            {
                                EditorId = "CAFF_SPEL_TEST_INVIS",
                                Name = "Calamity: Test Invisibility",
                                Delivery = "Self",
                                Effect = new SpellEffectRecordSpec
                                {
                                    MagicEffectEditorId = "CAFF_MGEF_TEST_INVIS",
                                    Magnitude = 1,
                                    Duration = 30,
                                    Area = 0,
                                },
                            },
                        },
                        Kid = new KidRule
                        {
                            Type = "Weapon",
                            Strings = "NONE",
                            FormFilters = "NONE",
                            Traits = "-E",
                            Chance = 100.0,
                        },
                        Runtime = new Dictionary<string, object?>(),
                    },
                ],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        var mgef = Assert.Single(mod.MagicEffects, m => m.EditorID == "CAFF_MGEF_TEST_INVIS");
        var archetype = Assert.IsType<MagicEffectArchetype>(mgef.Archetype);
        Assert.Equal(MagicEffectArchetype.TypeEnum.Invisibility, archetype.Type);
        Assert.Equal(ActorValue.Invisibility, archetype.ActorValue);
        Assert.False(mgef.Flags.HasFlag(MagicEffect.Flag.Hostile));

        var spell = Assert.Single(mod.Spells, s => s.EditorID == "CAFF_SPEL_TEST_INVIS");
        Assert.Equal(TargetType.Self, spell.TargetType);
        Assert.Equal(CastType.FireAndForget, spell.CastType);
    }

    [Fact]
    public void BuildKeywordPlugin_SpellFlagsExplicitlyZeroForNonHostile()
    {
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
                        Id = "test_passive",
                        EditorId = "LoreBox_CAFF_AFFIX_TEST_PASSIVE",
                        Name = "Affix: Test Passive",
                        Records = new AffixRecordSpec
                        {
                            MagicEffect = new MagicEffectRecordSpec
                            {
                                EditorId = "CAFF_MGEF_TEST_PASSIVE",
                                ActorValue = "Health",
                                Hostile = false,
                                Recover = true,
                            },
                            Spell = new SpellRecordSpec
                            {
                                EditorId = "CAFF_SPEL_TEST_PASSIVE",
                                Delivery = "Self",
                                SpellType = "Ability",
                                CastType = "ConstantEffect",
                                Effect = new SpellEffectRecordSpec
                                {
                                    MagicEffectEditorId = "CAFF_MGEF_TEST_PASSIVE",
                                    Magnitude = 50,
                                    Duration = 0,
                                    Area = 0,
                                },
                            },
                        },
                        Kid = new KidRule
                        {
                            Type = "Weapon",
                            Strings = "NONE",
                            FormFilters = "NONE",
                            Traits = "-E",
                            Chance = 100.0,
                        },
                        Runtime = new Dictionary<string, object?>(),
                    },
                ],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        var spell = Assert.Single(mod.Spells, s => s.EditorID == "CAFF_SPEL_TEST_PASSIVE");
        Assert.Equal((SpellDataFlag)0, spell.Flags);
    }

    [Fact]
    public void BuildKeywordPlugin_WhenRecordNameIsBilingual_UsesAsciiPluginName()
    {
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
                        Id = "test_bilingual_name",
                        EditorId = "CAFF_AFFIX_TEST_BILINGUAL",
                        Name = "Affix: Test",
                        Records = new AffixRecordSpec
                        {
                            MagicEffect = new MagicEffectRecordSpec
                            {
                                EditorId = "CAFF_MGEF_TEST_BILINGUAL",
                                Name = "Calamity Suffix: Critical Damage +10% / 치명타 피해 +10%",
                                ActorValue = "Health",
                                Hostile = false,
                            },
                            Spell = new SpellRecordSpec
                            {
                                EditorId = "CAFF_SPEL_TEST_BILINGUAL",
                                Name = "Calamity Suffix: Critical Damage +10% / 치명타 피해 +10%",
                                Delivery = "Self",
                                SpellType = "Ability",
                                CastType = "ConstantEffect",
                                Effect = new SpellEffectRecordSpec
                                {
                                    MagicEffectEditorId = "CAFF_MGEF_TEST_BILINGUAL",
                                    Magnitude = 10,
                                    Duration = 0,
                                    Area = 0,
                                },
                            },
                        },
                        Kid = new KidRule
                        {
                            Type = "Weapon",
                            Strings = "NONE",
                            FormFilters = "NONE",
                            Traits = "-E",
                            Chance = 100.0,
                        },
                        Runtime = new Dictionary<string, object?>(),
                    },
                ],
                KidRules = [],
                SpidRules = [],
            },
        };

        var mod = KeywordPluginBuilder.Build(spec);

        var mgef = Assert.Single(mod.MagicEffects, m => m.EditorID == "CAFF_MGEF_TEST_BILINGUAL");
        var spell = Assert.Single(mod.Spells, s => s.EditorID == "CAFF_SPEL_TEST_BILINGUAL");

        Assert.Equal("Calamity Suffix: Critical Damage +10%", mgef.Name?.String);
        Assert.Equal("Calamity Suffix: Critical Damage +10%", spell.Name?.String);
    }

    private static Func<FormKey, ILeveledItemGetter?> CreateLeveledListResolver(
        params (FormKey target, LeveledItem.Flag flags, Percent chanceNone)[] targets)
    {
        var baseMod = new SkyrimMod(ModKey.FromNameAndExtension("Skyrim.esm"), SkyrimRelease.SkyrimSE);
        var dummyItem = baseMod.MiscItems.AddNew(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x00000800));
        dummyItem.EditorID = "Gold001";
        dummyItem.Name = "Gold";

        var map = new Dictionary<FormKey, ILeveledItemGetter>();
        foreach (var (target, flags, chanceNone) in targets)
        {
            var record = baseMod.LeveledItems.AddNew(target);
            record.EditorID = $"BASE_{target.ID:X8}";
            record.Flags = flags;
            record.ChanceNone = chanceNone;
            record.Entries = [];
            record.Entries.Add(new LeveledItemEntry
            {
                Data = new LeveledItemEntryData
                {
                    Level = 1,
                    Count = 1,
                    Reference = dummyItem.ToLink<IItemGetter>(),
                },
            });
            map[target] = record;
        }

        return key => map.TryGetValue(key, out var value) ? value : null;
    }
}
