using System.Text;
using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Tests;

public sealed class AffixSpecLoaderTests
{
    [Fact]
    public void Load_ReadsJsonSpec()
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, """
        {
          "version": 1,
          "modKey": "CalamityAffixes_Keywords.esp",
          "eslFlag": true,
          "keywords": {
            "tags": [{"editorId":"CAFF_TAG_DOT","name":"dot"}],
            "affixes": [],
            "kidRules": [],
            "spidRules": []
          }
        }
        """, Encoding.UTF8);

        try
        {
            var spec = AffixSpecLoader.Load(specPath);
            Assert.Equal("CalamityAffixes_Keywords.esp", spec.ModKey);
            Assert.Single(spec.Keywords.Tags);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenAffixEditorIdMissingLoreBoxPrefix_DoesNotThrow()
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, """
        {
          "version": 1,
          "modKey": "CalamityAffixes_Keywords.esp",
          "eslFlag": true,
          "keywords": {
            "tags": [{"editorId":"CAFF_TAG_DOT","name":"dot"}],
            "affixes": [
              {
                "id": "test_affix",
                "editorId": "CAFF_AFFIX_TEST",
                "name": "Affix: Test",
                "kid": {
                  "type": "Weapon",
                  "strings": "NONE",
                  "formFilters": "NONE",
                  "traits": "-E",
                  "chance": 100.0
                },
                "runtime": {}
              }
            ],
            "kidRules": [],
            "spidRules": []
          }
        }
        """, Encoding.UTF8);

        try
        {
            var spec = AffixSpecLoader.Load(specPath);
            Assert.Single(spec.Keywords.Affixes);
            Assert.Equal("CAFF_AFFIX_TEST", spec.Keywords.Affixes[0].EditorId);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_ReadsLootArmorEligibilityOverrides()
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, """
        {
          "version": 1,
          "modKey": "CalamityAffixes_Keywords.esp",
          "eslFlag": true,
          "loot": {
            "cleanupInvalidLegacyAffixes": true,
            "armorEditorIdDenyContains": ["rewardbox", "lootbox"]
          },
          "keywords": {
            "tags": [{"editorId":"CAFF_TAG_DOT","name":"dot"}],
            "affixes": [],
            "kidRules": [],
            "spidRules": []
          }
        }
        """, Encoding.UTF8);

        try
        {
            var spec = AffixSpecLoader.Load(specPath);
            Assert.NotNull(spec.Loot);
            Assert.True(spec.Loot!.CleanupInvalidLegacyAffixes);
            Assert.Equal(new[] { "rewardbox", "lootbox" }, spec.Loot.ArmorEditorIdDenyContains);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }
}
