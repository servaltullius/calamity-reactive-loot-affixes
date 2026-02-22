using System.Text;
using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;

namespace CalamityAffixes.Generator.Tests;

public sealed class SpidIniWriterTests
{
    [Fact]
    public void Render_WhenSpecIsLoadedFromJson_WritesSpidRules()
    {
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, """
        {
          "version": 1,
          "modKey": "CalamityAffixes.esp",
          "eslFlag": true,
          "keywords": {
            "tags": [],
            "affixes": [],
            "kidRules": [],
            "spidRules": [
              {
                "comment": "Test SPID rule from JSON",
                "line": "Perk = CAFF_Perk_DeathDropRunewordFragment|ActorTypeNPC|NONE|NONE|NONE|1|100"
              }
            ]
          }
        }
        """, Encoding.UTF8);

        try
        {
            AffixSpec spec = AffixSpecLoader.Load(specPath);
            var ini = SpidIniWriter.Render(spec);

            Assert.Contains("; Test SPID rule from JSON", ini);
            Assert.Contains("Perk = CAFF_Perk_DeathDropRunewordFragment|ActorTypeNPC|NONE|NONE|NONE|1|100", ini);
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
