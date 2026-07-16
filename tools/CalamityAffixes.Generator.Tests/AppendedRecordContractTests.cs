using CalamityAffixes.Generator.Spec;

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
        """{"type":"ArtObject","artObject":{"editorId":"CAFF_ARTO_TEST","modelPath":"","artType":"MagicHitEffect"}}""",
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
            "modelPath": "Meshes\\Magic\\Test.nif",
            "artType": "MagicHitEffect"
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
                    Assert.Equal(@"Meshes\Magic\Test.nif", record.ArtObject?.ModelPath);
                    Assert.Equal("MagicHitEffect", record.ArtObject?.ArtType);
                    Assert.Null(record.MagicEffect);
                    Assert.Null(record.Spell);
                });
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
