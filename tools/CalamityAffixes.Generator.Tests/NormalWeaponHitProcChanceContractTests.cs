using System.Globalization;
using System.Text;
using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Tests;

public sealed class NormalWeaponHitProcChanceContractTests
{
    [Theory]
    [InlineData(0.0)]
    [InlineData(5.0)]
    [InlineData(100.0)]
    public void Load_WhenNormalWeaponHitProcChanceIsInRange_PreservesTypedValue(double chance)
    {
        var (tempRoot, specPath) = WriteSpec(chance);

        try
        {
            var spec = AffixSpecLoader.Load(specPath);
            var affix = Assert.Single(spec.Keywords.Affixes);
            Assert.Equal(chance, affix.Runtime.NormalWeaponHitProcChancePercent);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Theory]
    [InlineData(-0.1)]
    [InlineData(100.1)]
    public void Load_WhenNormalWeaponHitProcChanceIsOutOfRange_Throws(double chance)
    {
        var (tempRoot, specPath) = WriteSpec(chance);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("normalWeaponHitProcChancePercent", ex.Message, StringComparison.Ordinal);
            Assert.Contains("range 0..100", ex.Message, StringComparison.Ordinal);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Theory]
    [InlineData("DebugNotify", false)]
    [InlineData("SpawnTrap", false)]
    public void Load_WhenNormalWeaponHitProcChanceUsesUnsupportedAction_Throws(
        string actionType,
        bool requireCritOrPowerAttack)
    {
        var (tempRoot, specPath) = WriteSpec(
            5.0,
            actionType,
            requireCritOrPowerAttack);

        try
        {
            var ex = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("normalWeaponHitProcChancePercent", ex.Message, StringComparison.Ordinal);
            Assert.Contains("only supported", ex.Message, StringComparison.Ordinal);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenNormalWeaponHitProcChanceUsesCritGatedSpawnTrap_PreservesTypedValue()
    {
        var (tempRoot, specPath) = WriteSpec(
            5.0,
            "SpawnTrap",
            requireCritOrPowerAttack: true);

        try
        {
            var spec = AffixSpecLoader.Load(specPath);
            Assert.Equal(5.0, Assert.Single(spec.Keywords.Affixes).Runtime.NormalWeaponHitProcChancePercent);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    private static (string TempRoot, string SpecPath) WriteSpec(
        double chance,
        string actionType = "CastOnCrit",
        bool requireCritOrPowerAttack = false)
    {
        var tempRoot = Path.Combine(
            Path.GetTempPath(),
            "CalamityAffixes.Generator.Tests",
            Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        var specPath = Path.Combine(tempRoot, "affixes.json");
        var json = """
        {
          "version": 1,
          "modKey": "CalamityAffixes_Keywords.esp",
          "eslFlag": true,
          "keywords": {
            "tags": [],
            "affixes": [
              {
                "id": "test_normal_weapon_hit_proc",
                "editorId": "CAFF_AFFIX_NORMAL_WEAPON_HIT_PROC",
                "name": "Affix: Normal Weapon Hit Proc",
                "kid": { "type": "Weapon", "strings": "NONE", "formFilters": "NONE", "traits": "-E", "chance": 1.0 },
                "runtime": {
                  "trigger": "Hit",
                  "normalWeaponHitProcChancePercent": __CHANCE__,
                  "procChancePercent": 100.0,
                  "action": __ACTION__
                }
              }
            ],
            "kidRules": [],
            "spidRules": []
          }
        }
        """.Replace(
            "__CHANCE__",
            chance.ToString(CultureInfo.InvariantCulture),
            StringComparison.Ordinal).Replace(
            "__ACTION__",
            BuildActionJson(actionType, requireCritOrPowerAttack),
            StringComparison.Ordinal);
        File.WriteAllText(specPath, json, Encoding.UTF8);
        return (tempRoot, specPath);
    }

    private static string BuildActionJson(string actionType, bool requireCritOrPowerAttack)
    {
        return actionType switch
        {
            "CastOnCrit" => "{ \"type\": \"CastOnCrit\", \"spellForm\": \"Skyrim.esm|00012FD0\" }",
            "SpawnTrap" => "{ \"type\": \"SpawnTrap\", \"spellForm\": \"Skyrim.esm|00012FD0\", " +
                $"\"radius\": 100.0, \"ttlSeconds\": 5.0, \"requireCritOrPowerAttack\": {requireCritOrPowerAttack.ToString().ToLowerInvariant()} }}",
            _ => $"{{ \"type\": \"{actionType}\" }}",
        };
    }
}
