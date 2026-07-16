using System.Text.Json;
using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Tests;

public sealed class VfxFeedbackContractTests
{
    private static readonly FeedbackExpectation[] ExpectedFeedback =
    [
        new("keywords.affixes.core.json", "voice_of_power", "CAFF_ARTO_VFX_VOICE_OF_POWER", "Skyrim.esm|0x000A0F52", "Owner", 0.35, "Proc"),
        new("keywords.affixes.runewords.json", "runeword_spirit_final", "CAFF_ARTO_VFX_SPIRIT_ABSORB", "Skyrim.esm|0x0003D119", "Owner", 0.45, "Proc"),
        new("keywords.affixes.runewords.json", "runeword_smoke_final", "CAFF_ARTO_VFX_SMOKE_SLOW", "Skyrim.esm|0x0006A165", "Target", 0.75, "Proc"),
        new("keywords.affixes.runewords.json", "runeword_fury_final", "CAFF_ARTO_VFX_FURY_SURGE", "Skyrim.esm|0x000A28C4", "Owner", 0.45, "Proc"),
        new("keywords.affixes.runewords.json", "runeword_wealth_final", "CAFF_ARTO_VFX_WEALTH_PASSIVE", "Skyrim.esm|0x0003E952", "Owner", 0.60, "PassiveAdd"),
    ];

    private static readonly ArtExpectation[] ExpectedArt =
    [
        new("CAFF_ARTO_VFX_VOICE_OF_POWER", @"Meshes\Magic\ShoutSelfAreaEffect01.nif"),
        new("CAFF_ARTO_VFX_SPIRIT_ABSORB", @"Meshes\Magic\AbsorbSpellHitEffect01.nif"),
        new("CAFF_ARTO_VFX_SMOKE_SLOW", @"Meshes\Actors\Wisp\Character Assets\FXWispParticleAttach.nif"),
        new("CAFF_ARTO_VFX_FURY_SURGE", @"Meshes\Magic\IllusionMassRedCastBodyFX.nif"),
        new("CAFF_ARTO_VFX_WEALTH_PASSIVE", @"Meshes\Magic\HealRitualCastBodyFX.nif"),
    ];

    [Fact]
    public void ApprovedEffects_DeclareDataDrivenFeedbackWithCorrectRecipientAndTiming()
    {
        foreach (var expected in ExpectedFeedback)
        {
            var affix = ReadAffix(expected.ModuleFile, expected.AffixId);
            var feedback = affix.GetProperty("runtime").GetProperty("action").GetProperty("feedback");

            Assert.Equal(expected.ArtObjectEditorId, feedback.GetProperty("artObjectEditorId").GetString());
            Assert.Equal(expected.SoundForm, feedback.GetProperty("soundForm").GetString());
            Assert.Equal(expected.Target, feedback.GetProperty("target").GetString());
            Assert.Equal(expected.DurationSeconds, feedback.GetProperty("durationSeconds").GetDouble(), precision: 6);
            Assert.Equal(expected.PlayOn, feedback.GetProperty("playOn").GetString());
        }
    }

    [Fact]
    public void TypedTail_AppendsFiveMagicHitEffectArtObjectsAfterTheNineEffectRecords()
    {
        var root = ReadJson(Path.Combine("affixes", "modules", "spec.root.json"));
        var records = root.GetProperty("keywords").GetProperty("appendedRecords").EnumerateArray().ToArray();

        Assert.Equal(14, records.Length);
        for (var index = 0; index < ExpectedArt.Length; index++)
        {
            var record = records[index + 9];
            Assert.Equal("ArtObject", record.GetProperty("type").GetString());
            var art = record.GetProperty("artObject");
            Assert.Equal(ExpectedArt[index].EditorId, art.GetProperty("editorId").GetString());
            Assert.Equal(ExpectedArt[index].ModelPath, art.GetProperty("modelPath").GetString());
            Assert.Equal("MagicHitEffect", art.GetProperty("artType").GetString());
        }
    }

    [Fact]
    public void GeneratedPlugin_UsesApprovedVanillaModelsAndMagicHitEffectType()
    {
        var repoRoot = FindRepoRoot();
        var spec = AffixSpecLoader.Load(Path.Combine(repoRoot, "affixes", "affixes.json"));
        var mod = KeywordPluginBuilder.Build(spec);
        var artObjects = mod.ArtObjects.OrderBy(record => record.FormKey.ID).ToArray();

        Assert.Equal(ExpectedArt.Length, artObjects.Length);
        for (var index = 0; index < ExpectedArt.Length; index++)
        {
            Assert.Equal(ExpectedArt[index].EditorId, artObjects[index].EditorID);
            Assert.Equal(ExpectedArt[index].ModelPath, artObjects[index].Model?.File);
            Assert.Equal(ArtObject.TypeEnum.MagicHitEffect, artObjects[index].Type);
        }
    }

    [Fact]
    public void GeneratedDataEsp_PersistsApprovedVanillaModelsAndMagicHitEffectType()
    {
        var pluginPath = Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp");
        using var mod = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
        var artObjects = mod.ArtObjects.OrderBy(record => record.FormKey.ID).ToArray();

        Assert.Equal(ExpectedArt.Length, artObjects.Length);
        for (var index = 0; index < ExpectedArt.Length; index++)
        {
            Assert.Equal(ExpectedArt[index].EditorId, artObjects[index].EditorID);
            Assert.Equal(ExpectedArt[index].ModelPath, artObjects[index].Model?.File);
            Assert.Equal(ArtObject.TypeEnum.MagicHitEffect, artObjects[index].Type);
        }
    }

    [Fact]
    public void RuntimeFeedback_UsesShortHitArtAndNeverRefreshesPassiveVfxOnLoad()
    {
        var feedbackSource = ReadText(Path.Combine("skse", "CalamityAffixes", "src", "EventBridge.Actions.Feedback.cpp"));
        var castSource = ReadText(Path.Combine("skse", "CalamityAffixes", "src", "EventBridge.Actions.Cast.cpp"));
        var passiveSource = ReadText(Path.Combine("skse", "CalamityAffixes", "src", "EventBridge.Triggers.ActiveCounts.cpp"));

        Assert.Contains("InstantiateHitArt(", feedbackSource, StringComparison.Ordinal);
        Assert.DoesNotContain("InstantiateHitShader(", feedbackSource, StringComparison.Ordinal);
        Assert.Contains("BSAudioManager::GetSingleton", feedbackSource, StringComparison.Ordinal);
        Assert.Equal(2, CountOccurrences(castSource, "ActionFeedbackPlayOn::kProc"));

        var addBlock = ExtractBetween(
            passiveSource,
            "case detail::PassiveSpellReconcileAction::kAdd:",
            "case detail::PassiveSpellReconcileAction::kRemove:");
        var refreshBlock = ExtractBetween(
            passiveSource,
            "case detail::PassiveSpellReconcileAction::kRefresh:",
            "case detail::PassiveSpellReconcileAction::kKeep:");
        Assert.Contains("ActionFeedbackPlayOn::kPassiveAdd", addBlock, StringComparison.Ordinal);
        Assert.DoesNotContain("PlayActionFeedback", refreshBlock, StringComparison.Ordinal);
    }

    private static JsonElement ReadAffix(string moduleFile, string id)
    {
        var module = ReadJson(Path.Combine("affixes", "modules", moduleFile));
        return module.EnumerateArray().Single(candidate => candidate.GetProperty("id").GetString() == id).Clone();
    }

    private static JsonElement ReadJson(params string[] relativePath)
    {
        using var document = JsonDocument.Parse(ReadText(Path.Combine(relativePath)));
        return document.RootElement.Clone();
    }

    private static string ReadText(string relativePath) =>
        File.ReadAllText(Path.Combine(FindRepoRoot(), relativePath));

    private static int CountOccurrences(string text, string value)
    {
        var count = 0;
        var offset = 0;
        while ((offset = text.IndexOf(value, offset, StringComparison.Ordinal)) >= 0)
        {
            count += 1;
            offset += value.Length;
        }
        return count;
    }

    private static string ExtractBetween(string text, string start, string end)
    {
        var startIndex = text.IndexOf(start, StringComparison.Ordinal);
        Assert.True(startIndex >= 0, $"Missing start marker: {start}");
        var endIndex = text.IndexOf(end, startIndex + start.Length, StringComparison.Ordinal);
        Assert.True(endIndex >= 0, $"Missing end marker: {end}");
        return text[startIndex..endIndex];
    }

    private static string FindRepoRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "affixes", "affixes.modules.json")))
            {
                return directory.FullName;
            }
            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate repository root.");
    }

    private sealed record FeedbackExpectation(
        string ModuleFile,
        string AffixId,
        string ArtObjectEditorId,
        string SoundForm,
        string Target,
        double DurationSeconds,
        string PlayOn);

    private sealed record ArtExpectation(string EditorId, string ModelPath);
}
