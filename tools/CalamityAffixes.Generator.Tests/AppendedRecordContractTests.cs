using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;

namespace CalamityAffixes.Generator.Tests;

public sealed class AppendedRecordContractTests
{
    public static TheoryData<string> InvalidTaggedUnionItems => new()
    {
        """{"type":"Unknown","magicEffect":{"editorId":"CAFF_MGEF_TEST","actorValue":"Health"}}""",
        """{"type":"MagicEffect"}""",
        """{"type":"Spell"}""",
        """{"type":"MagicEffect","magicEffect":{"editorId":"CAFF_MGEF_TEST","actorValue":"Health"},"spell":{"editorId":"CAFF_SPEL_TEST","delivery":"Self","effect":{"magicEffectEditorId":"CAFF_MGEF_TEST","magnitude":1,"duration":0,"area":0}}}""",
        """{"type":"MagicEffect","magicEffect":{"editorId":"CAFF_MGEF_TEST","actorValue":"Health"},"unexpected":true}""",
        """{"type":"MagicEffect","spell":{"editorId":"CAFF_SPEL_TEST","delivery":"Self","effect":{"magicEffectEditorId":"CAFF_MGEF_TEST","magnitude":1,"duration":0,"area":0}}}""",
        """{"type":"ArtObject"}""",
        """{"type":"ArtObject","magicEffect":{"editorId":"CAFF_MGEF_TEST","actorValue":"Health"}}""",
        """{"type":"ArtObject","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"Meshes\\Magic\\Test.nif","artType":"MagicHitEffect"},"spell":{"editorId":"CAFF_SPEL_TEST","delivery":"Self","effect":{"magicEffectEditorId":"CAFF_MGEF_TEST","magnitude":1,"duration":0,"area":0}}}""",
        """{"type":"ArtObject","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"Meshes\\Magic\\Test.nif","artType":"Unknown"}}""",
        """{"type":"ArtObject","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"Meshes\\Magic\\Test.nif","artType":"MagicCasting"}}""",
        """{"type":"ArtObject","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"Meshes\\Magic\\Test.nif","artType":"MagicHitEffect"}}""",
        """{"type":"ArtObject","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"","artType":"MagicHitEffect"}}""",
        """{"type":"MovableStatic"}""",
        """{"type":"MovableStatic","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"Magic\\Test.nif","artType":"MagicHitEffect"}}""",
        """{"type":"MovableStatic","movableStatic":{"editorId":"CAFF_MSTT_TEST","modelPath":"Meshes\\Traps\\Test.nif"}}""",
        """{"type":"MovableStatic","movableStatic":{"editorId":"CAFF_MSTT_TEST","modelPath":""}}""",
        """{"type":"MovableStatic","movableStatic":{"editorId":"CAFF_MSTT_TEST","modelPath":"Traps\\Test.nif","mustUpdateAnimations":"true"}}""",
    };

    public static TheoryData<string> InvalidLegacyDragonBlocks => new()
    {
        """
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FIRE","actorValue":"ResistFire"}
        """,
        """
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FROST","actorValue":"ResistFrost"},
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FIRE","actorValue":"ResistFire"},
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_SHOCK","actorValue":"ResistShock"}
        """,
        """
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FIRE","actorValue":"ResistFire"},
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FROST","actorValue":"ResistFrost"},
        {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_OTHER","actorValue":"ResistShock"}
        """,
    };

    [Fact]
    public void Load_WhenAppendedRecordTaggedUnionIsValid_PreservesTypedOrder()
    {
        const string records = """
        {
          "type": "MagicEffect",
          "magicEffect": {
            "editorId": "CAFF_MGEF_TEST",
            "actorValue": "Health",
            "hostile": false,
            "recover": false
          }
        },
        {
          "type": "Spell",
          "spell": {
            "editorId": "CAFF_SPEL_TEST",
            "delivery": "Self",
            "effect": {
              "magicEffectEditorId": "CAFF_MGEF_TEST",
              "magnitude": 1,
              "duration": 0,
              "area": 0
            }
          }
        },
        {
          "type": "ArtObject",
          "artObject": {
            "editorId": "CAFF_ARTO_TEST",
            "modelPath": "Magic\\Test.nif",
            "artType": "MagicHitEffect"
          }
        },
        {
          "type": "MovableStatic",
          "movableStatic": {
            "editorId": "CAFF_MSTT_TEST",
            "modelPath": "Traps\\Test.nif",
            "mustUpdateAnimations": true
          }
        }
        """;
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, MinimalSpec(records));

        try
        {
            var spec = AffixSpecLoader.Load(specPath);

            Assert.Collection(
                spec.Keywords.AppendedRecords,
                record =>
                {
                    Assert.Equal("MagicEffect", record.Type);
                    Assert.Equal("CAFF_MGEF_TEST", record.MagicEffect?.EditorId);
                    Assert.Null(record.Spell);
                },
                record =>
                {
                    Assert.Equal("Spell", record.Type);
                    Assert.Equal("CAFF_SPEL_TEST", record.Spell?.EditorId);
                    Assert.Null(record.MagicEffect);
                    Assert.Null(record.ArtObject);
                },
                record =>
                {
                    Assert.Equal("ArtObject", record.Type);
                    Assert.Equal("CAFF_ARTO_TEST", record.ArtObject?.EditorId);
                    Assert.Equal(@"Magic\Test.nif", record.ArtObject?.ModelPath);
                    Assert.Equal("MagicHitEffect", record.ArtObject?.ArtType);
                    Assert.Null(record.MagicEffect);
                    Assert.Null(record.Spell);
                    Assert.Null(record.MovableStatic);
                },
                record =>
                {
                    Assert.Equal("MovableStatic", record.Type);
                    Assert.Equal("CAFF_MSTT_TEST", record.MovableStatic?.EditorId);
                    Assert.Equal(@"Traps\Test.nif", record.MovableStatic?.ModelPath);
                    Assert.True(record.MovableStatic?.MustUpdateAnimations);
                    Assert.Null(record.MagicEffect);
                    Assert.Null(record.Spell);
                    Assert.Null(record.ArtObject);
                });
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void BuildAndReimport_MovableStaticAnimationUpdatesAreOptInAndDefaultOff()
    {
        const string records = """
        {
          "type": "MovableStatic",
          "movableStatic": {
            "editorId": "CAFF_MSTT_STATIC",
            "modelPath": "Clutter\\StaticMarker.nif"
          }
        },
        {
          "type": "MovableStatic",
          "movableStatic": {
            "editorId": "CAFF_MSTT_EXPLICIT_FALSE",
            "modelPath": "Clutter\\StaticMarkerFalse.nif",
            "mustUpdateAnimations": false
          }
        },
        {
          "type": "MovableStatic",
          "movableStatic": {
            "editorId": "CAFF_MSTT_ANIMATED",
            "modelPath": "Traps\\AnimatedMarker.nif",
            "mustUpdateAnimations": true
          }
        }
        """;
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        var specPath = Path.Combine(tempRoot, "affixes.json");
        var pluginPath = Path.Combine(tempRoot, "CalamityAffixes.esp");
        File.WriteAllText(specPath, MinimalSpec(records));

        try
        {
            var spec = AffixSpecLoader.Load(specPath);
            var generated = KeywordPluginBuilder.Build(spec);
            ((IModGetter)generated).WriteToBinary(new FilePath(pluginPath));

            using var reimported = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
            var staticMarker = Assert.Single(reimported.MoveableStatics, record =>
                record.EditorID == "CAFF_MSTT_STATIC");
            var explicitFalseMarker = Assert.Single(reimported.MoveableStatics, record =>
                record.EditorID == "CAFF_MSTT_EXPLICIT_FALSE");
            var animatedMarker = Assert.Single(reimported.MoveableStatics, record =>
                record.EditorID == "CAFF_MSTT_ANIMATED");

            Assert.Equal(0u, (uint)staticMarker.MajorFlags & 0x00000100u);
            Assert.Equal(0u, (uint)explicitFalseMarker.MajorFlags & 0x00000100u);
            Assert.Equal(0x00000100u, (uint)animatedMarker.MajorFlags & 0x00000100u);
            Assert.False(staticMarker.MajorFlags.HasFlag(MoveableStatic.MajorFlag.MustUpdateAnims));
            Assert.False(explicitFalseMarker.MajorFlags.HasFlag(MoveableStatic.MajorFlag.MustUpdateAnims));
            Assert.True(animatedMarker.MajorFlags.HasFlag(MoveableStatic.MajorFlag.MustUpdateAnims));
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenContainerCasingWouldBypassTaggedUnionShape_Throws()
    {
        const string json = """
        {
          "version": 1,
          "modKey": "CalamityAffixes.esp",
          "eslFlag": true,
          "Keywords": {
            "tags": [],
            "affixes": [],
            "kidRules": [],
            "spidRules": [],
            "AppendedRecords": [
              {
                "type": "MagicEffect",
                "magicEffect": {"editorId":"CAFF_MGEF_TEST","actorValue":"Health"},
                "unexpected": true
              }
            ]
          }
        }
        """;
        AssertInvalidSpec(json);
    }

    [Theory]
    [InlineData("""{"type":"MiscItem"}""")]
    [InlineData("""{"type":"MiscItem","miscItem":null}""")]
    [InlineData("""{"type":"MiscItem","miscItem":{},"spell":{}}""")]
    [InlineData("""{"type":"Spell","spell":{},"miscItem":{}}""")]
    [MemberData(nameof(InvalidLegacyDragonBlocks))]
    public void Load_WhenLegacyDragonAppendBlockIsNotExactlySealed_Throws(string legacyRecords)
    {
        AssertInvalidSpec(MinimalSpecWithLegacy(legacyRecords));
    }

    [Theory]
    [MemberData(nameof(InvalidTaggedUnionItems))]
    public void Load_WhenAppendedRecordTaggedUnionIsInvalid_Throws(string appendedRecord)
    {
        AssertInvalidSpec(MinimalSpec(appendedRecord));
    }

    private static void AssertInvalidSpec(string json)
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, json);

        try
        {
            var exception = Record.Exception(() => AffixSpecLoader.Load(specPath));
            Assert.NotNull(exception);
            Assert.True(
                exception is InvalidDataException or System.Text.Json.JsonException,
                $"Expected invalid append-only contract to fail, got {exception.GetType().Name}: {exception.Message}");
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    private static string MinimalSpec(string appendedRecord) => $$"""
    {
      "version": 1,
      "modKey": "CalamityAffixes.esp",
      "eslFlag": true,
      "keywords": {
        "tags": [],
        "affixes": [],
        "kidRules": [],
        "spidRules": [],
        "appendedMagicEffects": [
          {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FIRE","actorValue":"ResistFire"},
          {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_FROST","actorValue":"ResistFrost"},
          {"editorId":"CAFF_MGEF_RW_DRAGON_RESIST_SHOCK","actorValue":"ResistShock"}
        ],
        "appendedRecords": [
          {{appendedRecord}}
        ]
      }
    }
    """;

    private static string MinimalSpecWithLegacy(string legacyRecords) => $$"""
    {
      "version": 1,
      "modKey": "CalamityAffixes.esp",
      "eslFlag": true,
      "keywords": {
        "tags": [],
        "affixes": [],
        "kidRules": [],
        "spidRules": [],
        "appendedMagicEffects": [
          {{legacyRecords}}
        ]
      }
    }
    """;
}
