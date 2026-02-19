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
            "armorEditorIdDenyContains": ["rewardbox", "lootbox"],
            "currencyDropMode": "leveledList",
            "currencyLeveledListTargets": ["Skyrim.esm|0009AF0A"]
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
            Assert.Equal("leveledList", spec.Loot.CurrencyDropMode);
            Assert.Equal(new[] { "Skyrim.esm|0009AF0A" }, spec.Loot.CurrencyLeveledListTargets);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenModKeyContainsPathSegments_Throws()
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, """
        {
          "version": 1,
          "modKey": "../CalamityAffixes.esp",
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
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("file name only", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenModKeyIsRootedPath_Throws()
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        var rootedModKey = Path.Combine(Path.GetTempPath(), "CalamityAffixes.esp").Replace("\\", "\\\\");
        File.WriteAllText(specPath, $$"""
        {
          "version": 1,
          "modKey": "{{rootedModKey}}",
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
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("must not be rooted", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenRuntimeTriggerMissing_Throws()
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
                "runtime": {
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
        """, Encoding.UTF8);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("runtime.trigger", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenRuntimeActionTypeUnsupported_Throws()
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
                "runtime": {
                  "trigger": "Hit",
                  "action": {
                    "type": "UnsupportedAction"
                  }
                }
              }
            ],
            "kidRules": [],
            "spidRules": []
          }
        }
        """, Encoding.UTF8);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("runtime.action.type", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenRuntimeProcChanceOutOfRange_Throws()
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
                "runtime": {
                  "trigger": "Hit",
                  "procChancePercent": 200.0,
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
        """, Encoding.UTF8);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("procChancePercent", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenRuntimeIcdSecondsNegative_Throws()
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
                "runtime": {
                  "trigger": "Hit",
                  "icdSeconds": -0.1,
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
        """, Encoding.UTF8);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("test_affix", ex.Message, StringComparison.OrdinalIgnoreCase);
            Assert.Contains("icdSeconds", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenRuntimePerTargetIcdSecondsIsNotNumber_Throws()
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
                "runtime": {
                  "trigger": "Hit",
                  "perTargetICDSeconds": "oops",
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
        """, Encoding.UTF8);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("test_affix", ex.Message, StringComparison.OrdinalIgnoreCase);
            Assert.Contains("perTargetICDSeconds", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }
}
