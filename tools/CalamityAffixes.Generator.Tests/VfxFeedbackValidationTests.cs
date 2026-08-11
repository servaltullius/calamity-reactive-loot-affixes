using System.Text;
using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Tests;

public sealed class VfxFeedbackValidationTests
{
    [Theory]
    [InlineData("\"spatialSound\": true", "\"spatialSound\": \"yes\"", "spatialSound must be a boolean")]
    [InlineData("\"trapFeedback\": {", "\"trapFeedback\": { \"unexpected\": true,", "unsupported property 'unexpected'")]
    [InlineData("\"unarmedScale\": 1.0,", "", "unarmedScale must be a number")]
    [InlineData("\"durationSeconds\": 0.2", "\"durationSeconds\": 4.2", "durationSeconds must be a number in range")]
    [InlineData("\"soundForm\": \"Skyrim.esm|0x0001828A\"", "\"soundForm\": \"bad-form\"", "Plugin|0xFORMID syntax")]
    [InlineData("\"placed\": {\n                \"soundForm\": \"Skyrim.esm|0x0001828A\"\n              }", "\"placed\": {}", "requires artObjectEditorId or soundForm")]
    [InlineData("CAFF_MSTT_TRAP_BEAR_VISUAL", "CAFF_MSTT_TRAP_BEAR_MISSING", "references missing MovableStatic")]
    [InlineData("\"markerWorldObjectEditorId\": \"CAFF_MSTT_TRAP_BEAR_VISUAL\",", "\"markerWorldObjectEditorId\": \"CalamityAffixes.esp|0x000B00\",", "markerWorldObjectEditorId must be an EditorID")]
    [InlineData("\"markerWorldObjectEditorId\": \"CAFF_MSTT_TRAP_BEAR_VISUAL\",", "\"markerWorldObjectEditorId\": \"CAFF_MSTT_TRAP_BEAR_VISUAL\", \"markerArtObjectEditorId\": \"CAFF_ARTO_VFX_TRAP_BEAR_MARKER\",", "requires exactly one")]
    [InlineData("\"worldMarkerAnimation\": {", "\"worldMarkerAnimation\": { \"unexpected\": true,", "worldMarkerAnimation contains unsupported property 'unexpected'")]
    [InlineData("\"initialEvent\": \"StartOpen\",", "", "worldMarkerAnimation.initialEvent must be a non-empty string")]
    [InlineData("\"openGateMilliseconds\": 900", "\"openGateMilliseconds\": 5001", "openGateMilliseconds must be an integer in range 0..5000")]
    [InlineData("\"openGateMilliseconds\": 900", "\"openGateMilliseconds\": 900.5", "openGateMilliseconds must be an integer in range 0..5000")]
    [InlineData("\"markerWorldObjectEditorId\": \"CAFF_MSTT_TRAP_BEAR_VISUAL\",", "\"markerArtObjectEditorId\": \"CAFF_ARTO_VFX_TRAP_BEAR_MARKER\",", "worldMarkerAnimation is supported only with markerWorldObjectEditorId")]
    public void Load_WhenVfxFeedbackContractIsInvalid_Throws(
        string original,
        string replacement,
        string expectedMessage)
    {
        var repoRoot = FindRepoRoot();
        var source = File.ReadAllText(Path.Combine(repoRoot, "affixes", "affixes.json"));
        Assert.Contains(original, source, StringComparison.Ordinal);
        var modified = ReplaceFirst(source, original, replacement);
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, modified, Encoding.UTF8);

        try
        {
            var error = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains(expectedMessage, error.Message, StringComparison.Ordinal);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    [Fact]
    public void Load_WhenTrapFeedbackIsUsedByNonTrapAction_Throws()
    {
        var repoRoot = FindRepoRoot();
        var source = File.ReadAllText(Path.Combine(repoRoot, "affixes", "affixes.json"));
        var modified = ReplaceFirst(source, "\"feedback\": {", "\"trapFeedback\": {");
        var tempRoot = Path.Combine(Path.GetTempPath(), "CalamityAffixes.Generator.Tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        var specPath = Path.Combine(tempRoot, "affixes.json");
        File.WriteAllText(specPath, modified, Encoding.UTF8);

        try
        {
            var error = Assert.Throws<InvalidDataException>(() => AffixSpecLoader.Load(specPath));
            Assert.Contains("only for SpawnTrap", error.Message, StringComparison.Ordinal);
        }
        finally
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }

    private static string ReplaceFirst(string value, string original, string replacement)
    {
        var index = value.IndexOf(original, StringComparison.Ordinal);
        Assert.True(index >= 0, $"Missing replacement source: {original}");
        return string.Concat(value.AsSpan(0, index), replacement, value.AsSpan(index + original.Length));
    }

    private static string FindRepoRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "affixes", "affixes.json")))
            {
                return directory.FullName;
            }
            directory = directory.Parent;
        }
        throw new DirectoryNotFoundException("Could not locate repository root.");
    }
}
