using System.Text;
using System.Text.Json;
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
            "currencyDropMode": "hybrid"
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
            Assert.Equal("hybrid", spec.Loot.CurrencyDropMode);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenLegacyLeveledListFieldsProvided_Throws()
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
            "currencyDropMode": "hybrid",
            "currencyLeveledListAutoDiscoverDeathItems": true,
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
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("currencyLeveledListTargets was removed", ex.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenCurrencyDropModeIsNotHybrid_Throws()
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
            "currencyDropMode": "runtime"
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
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("currencyDropMode supports only 'hybrid'", ex.Message, StringComparison.OrdinalIgnoreCase);
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

    [Fact]
    public void GetRunewordRuneLadder_ReturnsExpectedEndpointsAndUniqueRunes()
    {
        var ladder = AffixSpecLoader.GetRunewordRuneLadder();

        Assert.NotNull(ladder);
        Assert.True(ladder.Count >= 33, "Rune ladder should include full D2-style rune tiers.");
        Assert.Equal("El", ladder[0]);
        Assert.Equal("Zod", ladder[^1]);

        var unique = new HashSet<string>(ladder, StringComparer.OrdinalIgnoreCase);
        Assert.Equal(ladder.Count, unique.Count);
    }

    [Fact]
    public void BuildValidationContractJson_IncludesRunewordCatalogAndRuneWeights()
    {
        using var doc = JsonDocument.Parse(AffixSpecLoader.BuildValidationContractJson(indented: false));
        var root = doc.RootElement;

        Assert.True(root.TryGetProperty("runewordCatalog", out var runewordCatalog));
        Assert.Equal(JsonValueKind.Array, runewordCatalog.ValueKind);
        Assert.True(runewordCatalog.GetArrayLength() >= 90);

        Assert.True(root.TryGetProperty("runewordRuneWeights", out var runewordRuneWeights));
        Assert.Equal(JsonValueKind.Array, runewordRuneWeights.ValueKind);

        var seenRunes = new Dictionary<string, double>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in runewordRuneWeights.EnumerateArray())
        {
            if (entry.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            if (!entry.TryGetProperty("rune", out var runeElement) ||
                runeElement.ValueKind != JsonValueKind.String ||
                !entry.TryGetProperty("weight", out var weightElement) ||
                weightElement.ValueKind != JsonValueKind.Number ||
                !weightElement.TryGetDouble(out var weight))
            {
                continue;
            }

            var rune = runeElement.GetString();
            if (string.IsNullOrWhiteSpace(rune))
            {
                continue;
            }

            seenRunes[rune] = weight;
        }

        Assert.True(seenRunes.TryGetValue("El", out var elWeight));
        Assert.True(seenRunes.TryGetValue("Zod", out var zodWeight));
        Assert.True(elWeight > zodWeight, "Low-tier rune weight should be higher than high-tier rune weight.");
    }
}
