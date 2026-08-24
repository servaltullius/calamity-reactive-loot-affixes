using System.Text;
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
        new("keywords.affixes.runewords.json", "runeword_dream_final", "Skyrim.esm|0x0005B1BC", "Skyrim.esm|0x0003F206", "Target", 0.55, "Proc", true),
        new("keywords.affixes.runewords.json", "runeword_fury_final", "CAFF_ARTO_VFX_FURY_SURGE", "Skyrim.esm|0x000A28C4", "Owner", 0.45, "Proc"),
        new("keywords.affixes.runewords.json", "runeword_wealth_final", "CAFF_ARTO_VFX_WEALTH_PASSIVE", "Skyrim.esm|0x0003E952", "Owner", 0.60, "PassiveAdd"),
    ];

    private static readonly ArtExpectation[] ExpectedArt =
    [
        new("CAFF_ARTO_VFX_VOICE_OF_POWER", @"Magic\ShoutSelfAreaEffect01.nif"),
        new("CAFF_ARTO_VFX_SPIRIT_ABSORB", @"Magic\AbsorbSpellHitEffect01.nif"),
        new("CAFF_ARTO_VFX_SMOKE_SLOW", @"Actors\Wisp\Character Assets\FXWispParticleAttach.nif"),
        new("CAFF_ARTO_VFX_FURY_SURGE", @"Magic\IllusionMassRedCastBodyFX.nif"),
        new("CAFF_ARTO_VFX_WEALTH_PASSIVE", @"Magic\HealRitualCastBodyFX.nif"),
        // Particle-backed trap markers must be self-playing FX NIFs (rune glyphs / hazards):
        // the 2026-08-10 in-game probes proved BSTempEffectParticle never
        // applies a world transform to static world meshes and never animates
        // externally-driven art-object FX, so those breeds render nothing.
        // Retained as an unused append-only tombstone so every existing FormID and
        // record signature stays stable after the bear marker moves to a world object.
        new("CAFF_ARTO_VFX_TRAP_BEAR_MARKER", @"Magic\RuneFrostProjectile01.nif"),
        new("CAFF_ARTO_VFX_TRAP_BEAR_BURST", @"Magic\ExplosionFrost01.nif"),
        new("CAFF_ARTO_VFX_TRAP_RUNE_MARKER", @"Magic\RuneFireProjectile01.nif"),
        new("CAFF_ARTO_VFX_TRAP_RUNE_BURST", @"Magic\ExplosionFrost01.nif"),
        new("CAFF_ARTO_VFX_TRAP_PLAGUE_MARKER", @"DLC02\Effects\RunePoisonProjectile.nif"),
        new("CAFF_ARTO_VFX_TRAP_PLAGUE_BURST", @"Effects\FXGasTrapBlast.nif"),
        new("CAFF_ARTO_VFX_TRAP_TAR_MARKER", @"DLC02\Effects\AshRuneProjectile01.nif"),
        new("CAFF_ARTO_VFX_TRAP_TAR_BURST", @"Effects\FXGasTrap01.nif"),
        new("CAFF_ARTO_VFX_TRAP_SIPHON_MARKER", @"DLC02\Effects\RuneFrenzyProjectile.nif"),
        new("CAFF_ARTO_VFX_TRAP_SIPHON_BURST", @"Magic\AbsorbSpellHitEffect01.nif"),
        new("CAFF_ARTO_VFX_TRAP_CHAOS_MARKER", @"Magic\RuneLightningProjectile01.nif"),
        new("CAFF_ARTO_VFX_TRAP_CHAOS_BURST", @"Magic\ExplosionShock01.nif"),
        new("CAFF_ARTO_VFX_CORPSE_FIRE", @"Magic\FireBallExp01.nif"),
        new("CAFF_ARTO_VFX_CORPSE_PLAGUE", @"Effects\FXGasTrapBlast.nif"),
        new("CAFF_ARTO_VFX_PROC_EMERGENCY_HEAL", @"Magic\HealTargetFX.nif"),
        new("CAFF_ARTO_VFX_PROC_PHASE", @"Magic\InvisFXBody01.nif"),
        new("CAFF_ARTO_VFX_PROC_PHYSICAL_BULWARK", @"Magic\ShieldSpellBodyFX.nif"),
        new("CAFF_ARTO_VFX_PROC_MAGIC_WARD", @"Magic\WardBodyFX.nif"),
        new("CAFF_ARTO_VFX_PROC_REFLECT", @"Actors\Spriggan\FXSprigganAttachments.nif"),
        new("CAFF_ARTO_VFX_PROC_RADIANCE", @"Magic\HealRitualCastBodyFX.nif"),
        new("CAFF_ARTO_VFX_PROC_DRAGON_SCALE", @"Magic\FXFireCloak01.nif"),
    ];

    private static readonly WorldMarkerExpectation[] ExpectedWorldMarkers =
    [
        new(0x000B00u, "bear_trap", "CAFF_MSTT_TRAP_BEAR_VISUAL", @"Traps\BearTrap\BearTrap01.nif", true),
        new(0x000B01u, "rune_trap", "CAFF_MSTT_TRAP_RUNE_VISUAL", @"Traps\PressurePlate\TrapStonePressurePlate01.nif", true),
        new(0x000B02u, "plague_spore", "CAFF_MSTT_TRAP_PLAGUE_VISUAL", @"actors\DLC02\Spider_poison\CharacterAssets\spidersackdead.nif", false),
        new(0x000B03u, "tar_blight", "CAFF_MSTT_TRAP_TAR_VISUAL", @"Traps\OilTrapPuddle01\OilTrapPuddle01.nif", true),
        new(0x000B04u, "siphon_spore", "CAFF_MSTT_TRAP_SIPHON_VISUAL", @"Actors\DLC02\Spider_poison\CharacterAssets\ExpSpiderEggsAlbino.nif", false),
        new(0x000B05u, "chaos_rune", "CAFF_MSTT_TRAP_CHAOS_VISUAL", @"Traps\PressurePlateMetal\TrapPressurePlateMetal01.nif", true),
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
            var actualSpatialSound = feedback.TryGetProperty("spatialSound", out var spatialSound) && spatialSound.GetBoolean();
            Assert.Equal(expected.SpatialSound, actualSpatialSound);
        }
    }

    [Fact]
    public void TypedTail_AppendsTwentySixMagicHitEffectArtObjectsAndSixWorldMarkersAfterTheNineEffectRecords()
    {
        var root = ReadJson(Path.Combine("affixes", "modules", "spec.root.json"));
        var records = root.GetProperty("keywords").GetProperty("appendedRecords").EnumerateArray().ToArray();

        Assert.Equal(50, records.Length);
        var artRecords = records
            .Where(record => record.GetProperty("type").GetString() == "ArtObject")
            .ToArray();
        Assert.Equal(ExpectedArt.Length, artRecords.Length);
        for (var index = 0; index < ExpectedArt.Length; index++)
        {
            var record = artRecords[index];
            Assert.Equal("ArtObject", record.GetProperty("type").GetString());
            var art = record.GetProperty("artObject");
            Assert.Equal(ExpectedArt[index].EditorId, art.GetProperty("editorId").GetString());
            Assert.Equal(ExpectedArt[index].ModelPath, art.GetProperty("modelPath").GetString());
            Assert.Equal("MagicHitEffect", art.GetProperty("artType").GetString());
        }

        var worldObjects = records
            .Where(record => record.GetProperty("type").GetString() == "MovableStatic")
            .ToArray();
        Assert.Equal(ExpectedWorldMarkers.Length, worldObjects.Length);
        for (var index = 0; index < ExpectedWorldMarkers.Length; index++)
        {
            var expected = ExpectedWorldMarkers[index];
            var movableStatic = worldObjects[index].GetProperty("movableStatic");
            Assert.Equal(expected.EditorId, movableStatic.GetProperty("editorId").GetString());
            Assert.Equal(expected.ModelPath, movableStatic.GetProperty("modelPath").GetString());
            Assert.Equal(expected.MustUpdateAnimations, movableStatic.GetProperty("mustUpdateAnimations").GetBoolean());
        }
    }

    [Fact]
    public void BearTrapFeedback_UsesVanillaAnimationEventsWithoutDuplicateTriggerOrRearmSounds()
    {
        var affix = ReadAffix("keywords.affixes.core.json", "bear_trap");
        var feedback = affix.GetProperty("runtime").GetProperty("action").GetProperty("trapFeedback");

        var markerEditorId = feedback.GetProperty("markerWorldObjectEditorId").GetString();
        Assert.Equal("CAFF_MSTT_TRAP_BEAR_VISUAL", markerEditorId);
        Assert.DoesNotContain('|', markerEditorId!);
        Assert.Equal(1.0, feedback.GetProperty("unarmedScale").GetDouble());
        Assert.Equal(1.0, feedback.GetProperty("armedScale").GetDouble());
        Assert.False(feedback.TryGetProperty("markerArtObjectEditorId", out _));
        Assert.False(feedback.TryGetProperty("expired", out _));

        var animation = feedback.GetProperty("worldMarkerAnimation");
        Assert.Equal("StartOpen", animation.GetProperty("initialEvent").GetString());
        Assert.Equal("Trigger01", animation.GetProperty("triggerEvent").GetString());
        Assert.Equal("Reset01", animation.GetProperty("rearmEvent").GetString());
        Assert.Equal(900.0, animation.GetProperty("openGateMilliseconds").GetDouble());

        var placed = feedback.GetProperty("placed");
        Assert.Equal("Skyrim.esm|0x0001828A", placed.GetProperty("soundForm").GetString());
        Assert.False(feedback.TryGetProperty("armed", out _));

        var triggered = feedback.GetProperty("triggered");
        Assert.Equal("CAFF_ARTO_VFX_TRAP_BEAR_BURST", triggered.GetProperty("artObjectEditorId").GetString());
        Assert.False(triggered.TryGetProperty("soundForm", out _));
    }

    [Fact]
    public void RemainingTrapFeedback_UsesUnitScaleWorldMarkersWithoutAnimationEvents()
    {
        foreach (var expected in ExpectedWorldMarkers.Skip(1))
        {
            var affix = ReadAffix("keywords.affixes.core.json", expected.AffixId);
            var feedback = affix.GetProperty("runtime").GetProperty("action").GetProperty("trapFeedback");

            Assert.Equal(expected.EditorId, feedback.GetProperty("markerWorldObjectEditorId").GetString());
            Assert.Equal(1.0, feedback.GetProperty("unarmedScale").GetDouble());
            Assert.Equal(1.0, feedback.GetProperty("armedScale").GetDouble());
            Assert.False(feedback.TryGetProperty("markerArtObjectEditorId", out _));
            Assert.False(feedback.TryGetProperty("worldMarkerAnimation", out _));
        }
    }

    [Fact]
    public void FeedbackCoverage_CountsTrapsCorpsesTargetAndOwnerProcLanes()
    {
        var actions = new List<JsonElement>();
        foreach (var moduleName in new[] { "keywords.affixes.core.json", "keywords.affixes.runewords.json", "keywords.affixes.suffixes.json" })
        {
            var module = ReadJson(Path.Combine("affixes", "modules", moduleName));
            actions.AddRange(module.EnumerateArray().Select(affix => affix.GetProperty("runtime").GetProperty("action").Clone()));
        }

        Assert.Equal(6, actions.Count(action => action.TryGetProperty("trapFeedback", out _)));
        Assert.Equal(8, actions.Count(action =>
            action.TryGetProperty("feedback", out var feedback) &&
            feedback.GetProperty("target").GetString() == "Corpse"));
        Assert.Equal(59, actions.Count(action =>
            action.TryGetProperty("feedback", out var feedback) &&
            feedback.GetProperty("target").GetString() == "Target"));
        Assert.Equal(67, actions.Count(action =>
            action.TryGetProperty("feedback", out var feedback) &&
            feedback.GetProperty("target").GetString() == "Owner" &&
            feedback.TryGetProperty("spatialSound", out var spatial) && spatial.GetBoolean()));
    }

    [Fact]
    public void EveryCastSpellProcLane_DeclaresFeedback()
    {
        foreach (var moduleName in new[] { "keywords.affixes.core.json", "keywords.affixes.runewords.json", "keywords.affixes.suffixes.json" })
        {
            var module = ReadJson(Path.Combine("affixes", "modules", moduleName));
            foreach (var affix in module.EnumerateArray())
            {
                var action = affix.GetProperty("runtime").GetProperty("action");
                var type = action.GetProperty("type").GetString();
                if (type is not ("CastSpell" or "CastSpellAdaptiveElement"))
                {
                    continue;
                }

                // A CastSpell proc without a feedback block is invisible in-game for
                // instant effects (no projectile, no cast art, duration-0 hit shader),
                // so declaring feedback is a data contract, not a nicety.
                Assert.True(
                    action.TryGetProperty("feedback", out _),
                    $"{affix.GetProperty("id").GetString()}: CastSpell-lane action must declare feedback.");
            }
        }
    }

    [Fact]
    public void GeneratedPlugin_UsesApprovedModelsAndMagicHitEffectType()
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
            // Raw 1 = Skyrim.esm's Magic Hit Effect. Mutagen 0.52.0's named
            // TypeEnum.MagicHitEffect serializes as raw 2 (Enchantment Effect in-game),
            // so the builder writes the raw value and tests assert numerically.
            Assert.Equal((ArtObject.TypeEnum)1, artObjects[index].Type);
        }

        var worldMarkers = mod.MoveableStatics.OrderBy(record => record.FormKey.ID).ToArray();
        Assert.Equal(ExpectedWorldMarkers.Length, worldMarkers.Length);
        for (var index = 0; index < ExpectedWorldMarkers.Length; index++)
        {
            var expected = ExpectedWorldMarkers[index];
            var actual = worldMarkers[index];
            Assert.Equal(expected.FormId, actual.FormKey.ID);
            Assert.Equal(expected.EditorId, actual.EditorID);
            Assert.Equal(expected.ModelPath, actual.Model?.File);
            Assert.Equal(
                expected.MustUpdateAnimations,
                actual.MajorFlags.HasFlag(MoveableStatic.MajorFlag.MustUpdateAnims));
        }
    }

    [Fact]
    public void GeneratedDataEsp_PersistsApprovedModelsAndMagicHitEffectType()
    {
        var pluginPath = Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp");
        using var mod = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
        var artObjects = mod.ArtObjects.OrderBy(record => record.FormKey.ID).ToArray();

        Assert.Equal(ExpectedArt.Length, artObjects.Length);
        for (var index = 0; index < ExpectedArt.Length; index++)
        {
            Assert.Equal(ExpectedArt[index].EditorId, artObjects[index].EditorID);
            Assert.Equal(ExpectedArt[index].ModelPath, artObjects[index].Model?.File);
            Assert.Equal((ArtObject.TypeEnum)1, artObjects[index].Type);
        }
        var worldMarkers = mod.MoveableStatics.OrderBy(record => record.FormKey.ID).ToArray();
        Assert.Equal(ExpectedWorldMarkers.Length, worldMarkers.Length);
        for (var index = 0; index < ExpectedWorldMarkers.Length; index++)
        {
            var expected = ExpectedWorldMarkers[index];
            var actual = worldMarkers[index];
            Assert.Equal(expected.FormId, actual.FormKey.ID);
            Assert.Equal(expected.EditorId, actual.EditorID);
            Assert.Equal(expected.ModelPath, actual.Model?.File);
            Assert.Equal(
                expected.MustUpdateAnimations,
                actual.MajorFlags.HasFlag(MoveableStatic.MajorFlag.MustUpdateAnims));
        }
    }

    [Fact]
    public void GeneratedDataEsp_StoresRawMagicHitEffectDnamAndDataMeshesRelativeModelPaths()
    {
        var pluginPath = Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp");
        var artObjects = ReadRawArtObjects(pluginPath);

        Assert.Equal(ExpectedArt.Length, artObjects.Count);
        foreach (var (editorId, modelPath, dnam) in artObjects)
        {
            // Assert on the bytes actually persisted in the plugin, not on Mutagen's enum
            // mapping: raw DNAM=1 is Magic Hit Effect in Skyrim.esm (0=Magic Casting,
            // 2=Enchantment Effect). A Mutagen upgrade that changes the enum-to-raw
            // mapping must fail here instead of silently shipping broken hit art.
            Assert.True(dnam == 1u, $"{editorId}: raw DNAM must be 1 (Magic Hit Effect), got {dnam}.");
            Assert.False(string.IsNullOrWhiteSpace(modelPath), $"{editorId}: MODL missing.");
            Assert.False(
                modelPath.Replace('/', '\\').TrimStart().StartsWith(@"Meshes\", StringComparison.OrdinalIgnoreCase),
                $"{editorId}: MODL must be Data\\Meshes-relative, got '{modelPath}'.");
        }
    }

    [Fact]
    public void GeneratedDataEsp_WorldMarkersAreScriptlessMovableStaticsAtAppendOnlyTail()
    {
        var pluginPath = Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp");
        var worldMarkers = ReadRawMovableStatics(pluginPath)
            .OrderBy(record => record.FormId & 0x00FFFFFFu)
            .ToArray();

        Assert.Equal(ExpectedWorldMarkers.Length, worldMarkers.Length);
        for (var index = 0; index < ExpectedWorldMarkers.Length; index++)
        {
            var expected = ExpectedWorldMarkers[index];
            var actual = worldMarkers[index];
            // Raw plugin FormIDs encode the owning file index in the high byte.
            Assert.Equal(expected.FormId, actual.FormId & 0x00FFFFFFu);
            Assert.Equal(expected.EditorId, actual.EditorId);
            Assert.Equal(expected.ModelPath, actual.ModelPath);
            Assert.Equal(expected.MustUpdateAnimations, (actual.RecordFlags & 0x00000100u) != 0);
            Assert.DoesNotContain("VMAD", actual.Subrecords);
        }
    }

    [Fact]
    public void RuneTrapMarker_KeepsFormIdAndSelfPlayingGlyphModel()
    {
        var pluginPath = Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp");
        using var mod = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
        var marker = Assert.Single(mod.ArtObjects, record =>
            record.EditorID == "CAFF_ARTO_VFX_TRAP_RUNE_MARKER");

        Assert.Equal(mod.ModKey, marker.FormKey.ModKey);
        Assert.Equal(0x000AEDu, marker.FormKey.ID);
        Assert.Equal(@"Magic\RuneFireProjectile01.nif", marker.Model?.File);
    }

    [Fact]
    public void RuntimeFeedback_UsesShortHitArtAndNeverRefreshesPassiveVfxOnLoad()
    {
        var feedbackSource = ReadText(Path.Combine("skse", "CalamityAffixes", "src", "EventBridge.Actions.Feedback.cpp"));
        var castSource = ReadText(Path.Combine("skse", "CalamityAffixes", "src", "EventBridge.Actions.Cast.cpp"));
        var passiveSource = ReadText(Path.Combine("skse", "CalamityAffixes", "src", "EventBridge.Triggers.ActiveCounts.cpp"));

        Assert.Contains("InstantiateHitArt(", feedbackSource, StringComparison.Ordinal);
        Assert.DoesNotContain("InstantiateHitShader(", feedbackSource, StringComparison.Ordinal);
        Assert.Contains("recipient->Is3DLoaded()", feedbackSource, StringComparison.Ordinal);
        Assert.DoesNotContain("recipient->IsDead()", feedbackSource, StringComparison.Ordinal);
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

    [Fact]
    public void ConversionMagicEffects_StoreRawOnHitSoundsPerElement()
    {
        var pluginPath = Path.Combine(FindRepoRoot(), "Data", "CalamityAffixes.esp");
        var sounds = ReadRawMagicEffectSounds(pluginPath);

        // SNDD raw type 5 is Skyrim.esm's "On Hit" slot — it plays on effect
        // application without a casting cycle (vanilla: potions, apparel enchants).
        // Fire shipped first as the B1 prototype and passed the in-game listen test;
        // frost/shock were then extended with their own verified SNDR records.
        // The shock spell is also cast by the Archmage lane, which rides along.
        var expected = new (string EditorId, uint Sndr)[]
        {
            ("CAFF_MGEF_DMG_FIRE_DYNAMIC", 0x0003C8FCu),   // MAGFireboltImpactSD
            ("CAFF_MGEF_DMG_FROST_DYNAMIC", 0x0003E5CCu),  // MAGFrostBiteImpactSD
            ("CAFF_MGEF_DMG_SHOCK_DYNAMIC", 0x0003F20Du),  // MAGShockImpactSD
        };
        foreach (var (editorId, sndr) in expected)
        {
            Assert.True(sounds.TryGetValue(editorId, out var mgefSounds), $"{editorId} missing");
            var entry = Assert.Single(mgefSounds!);
            Assert.Equal(5u, entry.Type);
            Assert.Equal(sndr, entry.Sound);
        }
    }

    // Minimal TES5 plugin reader: walks GRUPs and collects MGEF (EDID, raw SNDD pairs).
    private static Dictionary<string, List<(uint Type, uint Sound)>> ReadRawMagicEffectSounds(string pluginPath)
    {
        var buffer = File.ReadAllBytes(pluginPath);
        var result = new Dictionary<string, List<(uint, uint)>>();
        var tes4DataSize = BitConverter.ToInt32(buffer, 4);
        WalkRawMagicEffects(buffer, 24 + tes4DataSize, buffer.Length, result);
        return result;
    }

    private static void WalkRawMagicEffects(byte[] buffer, int start, int end, Dictionary<string, List<(uint, uint)>> result)
    {
        var pos = start;
        while (pos + 24 <= end)
        {
            var recordType = Encoding.ASCII.GetString(buffer, pos, 4);
            if (recordType == "GRUP")
            {
                var groupSize = BitConverter.ToInt32(buffer, pos + 4);
                Assert.True(groupSize >= 24, "Malformed GRUP size.");
                WalkRawMagicEffects(buffer, pos + 24, pos + groupSize, result);
                pos += groupSize;
                continue;
            }

            var dataSize = BitConverter.ToInt32(buffer, pos + 4);
            if (recordType == "MGEF")
            {
                var flags = BitConverter.ToUInt32(buffer, pos + 8);
                Assert.True((flags & 0x00040000u) == 0, "Compressed MGEF records are not expected.");
                var editorId = string.Empty;
                var entries = new List<(uint, uint)>();
                var subPos = pos + 24;
                var subEnd = pos + 24 + dataSize;
                while (subPos + 6 <= subEnd)
                {
                    var subType = Encoding.ASCII.GetString(buffer, subPos, 4);
                    int subSize = BitConverter.ToUInt16(buffer, subPos + 4);
                    subPos += 6;
                    if (subType == "EDID")
                    {
                        editorId = ReadZString(buffer, subPos, subSize);
                    }
                    else if (subType == "SNDD")
                    {
                        for (var i = 0; i + 8 <= subSize; i += 8)
                        {
                            entries.Add((
                                BitConverter.ToUInt32(buffer, subPos + i),
                                BitConverter.ToUInt32(buffer, subPos + i + 4)));
                        }
                    }
                    subPos += subSize;
                }
                if (editorId.Length > 0)
                {
                    result[editorId] = entries;
                }
            }
            pos += 24 + dataSize;
        }
    }

    // Minimal TES5 plugin reader: walks GRUPs and collects ARTO (EDID, MODL, raw DNAM).
    // Deliberately independent of Mutagen so DNAM assertions observe the persisted bytes.
    private static List<(string EditorId, string ModelPath, uint Dnam)> ReadRawArtObjects(string pluginPath)
    {
        var buffer = File.ReadAllBytes(pluginPath);
        var results = new List<(string, string, uint)>();
        var tes4DataSize = BitConverter.ToInt32(buffer, 4);
        WalkRawRecords(buffer, 24 + tes4DataSize, buffer.Length, results);
        return results;
    }

    private static void WalkRawRecords(byte[] buffer, int start, int end, List<(string, string, uint)> results)
    {
        var pos = start;
        while (pos + 24 <= end)
        {
            var recordType = Encoding.ASCII.GetString(buffer, pos, 4);
            if (recordType == "GRUP")
            {
                var groupSize = BitConverter.ToInt32(buffer, pos + 4);
                Assert.True(groupSize >= 24, "Malformed GRUP size.");
                WalkRawRecords(buffer, pos + 24, pos + groupSize, results);
                pos += groupSize;
                continue;
            }

            var dataSize = BitConverter.ToInt32(buffer, pos + 4);
            if (recordType == "ARTO")
            {
                var flags = BitConverter.ToUInt32(buffer, pos + 8);
                Assert.True((flags & 0x00040000u) == 0, "Compressed ARTO records are not expected.");
                results.Add(ParseRawArtObject(buffer, pos + 24, dataSize));
            }
            pos += 24 + dataSize;
        }
    }

    private static (string EditorId, string ModelPath, uint Dnam) ParseRawArtObject(byte[] buffer, int start, int size)
    {
        var editorId = string.Empty;
        var modelPath = string.Empty;
        var dnam = uint.MaxValue;
        var pos = start;
        var end = start + size;
        while (pos + 6 <= end)
        {
            var subType = Encoding.ASCII.GetString(buffer, pos, 4);
            int subSize = BitConverter.ToUInt16(buffer, pos + 4);
            pos += 6;
            switch (subType)
            {
                case "EDID":
                    editorId = ReadZString(buffer, pos, subSize);
                    break;
                case "MODL":
                    modelPath = ReadZString(buffer, pos, subSize);
                    break;
                case "DNAM":
                    Assert.True(subSize >= 4, "ARTO DNAM must hold a uint32.");
                    dnam = BitConverter.ToUInt32(buffer, pos);
                    break;
            }
            pos += subSize;
        }
        return (editorId, modelPath, dnam);
    }

    private static List<(uint FormId, uint RecordFlags, string EditorId, string ModelPath, string[] Subrecords)> ReadRawMovableStatics(string pluginPath)
    {
        var buffer = File.ReadAllBytes(pluginPath);
        var results = new List<(uint, uint, string, string, string[])>();
        var tes4DataSize = BitConverter.ToInt32(buffer, 4);
        WalkRawMovableStatics(buffer, 24 + tes4DataSize, buffer.Length, results);
        return results;
    }

    private static void WalkRawMovableStatics(
        byte[] buffer,
        int start,
        int end,
        List<(uint FormId, uint RecordFlags, string EditorId, string ModelPath, string[] Subrecords)> results)
    {
        var pos = start;
        while (pos + 24 <= end)
        {
            var recordType = Encoding.ASCII.GetString(buffer, pos, 4);
            if (recordType == "GRUP")
            {
                var groupSize = BitConverter.ToInt32(buffer, pos + 4);
                Assert.True(groupSize >= 24, "Malformed GRUP size.");
                WalkRawMovableStatics(buffer, pos + 24, pos + groupSize, results);
                pos += groupSize;
                continue;
            }

            var dataSize = BitConverter.ToInt32(buffer, pos + 4);
            if (recordType == "MSTT")
            {
                var flags = BitConverter.ToUInt32(buffer, pos + 8);
                Assert.True((flags & 0x00040000u) == 0, "Compressed MSTT records are not expected.");
                var formId = BitConverter.ToUInt32(buffer, pos + 12);
                var editorId = string.Empty;
                var modelPath = string.Empty;
                var subrecords = new List<string>();
                var subPos = pos + 24;
                var subEnd = subPos + dataSize;
                while (subPos + 6 <= subEnd)
                {
                    var subType = Encoding.ASCII.GetString(buffer, subPos, 4);
                    int subSize = BitConverter.ToUInt16(buffer, subPos + 4);
                    subrecords.Add(subType);
                    subPos += 6;
                    if (subType == "EDID")
                    {
                        editorId = ReadZString(buffer, subPos, subSize);
                    }
                    else if (subType == "MODL")
                    {
                        modelPath = ReadZString(buffer, subPos, subSize);
                    }
                    subPos += subSize;
                }
                results.Add((formId, flags, editorId, modelPath, subrecords.ToArray()));
            }
            pos += 24 + dataSize;
        }
    }

    private static string ReadZString(byte[] buffer, int start, int size)
    {
        var text = Encoding.ASCII.GetString(buffer, start, size);
        var nul = text.IndexOf('\0');
        return nul >= 0 ? text[..nul] : text;
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
        string PlayOn,
        bool SpatialSound = false);

    private sealed record ArtExpectation(string EditorId, string ModelPath);

    private sealed record WorldMarkerExpectation(
        uint FormId,
        string AffixId,
        string EditorId,
        string ModelPath,
        bool MustUpdateAnimations);
}
