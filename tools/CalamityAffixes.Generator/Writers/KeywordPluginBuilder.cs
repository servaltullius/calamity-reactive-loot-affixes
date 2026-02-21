using CalamityAffixes.Generator.Spec;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;
using System.IO;
using System.Linq;
using System.Text;

namespace CalamityAffixes.Generator.Writers;

public static class KeywordPluginBuilder
{
    private const string RunewordFragmentEditorIdPrefix = "CAFF_RuneFrag_";
    private const double DefaultRunewordFragmentWeight = 25.0;
    private const int MaxRunewordFragmentWeightTickets = 64;
    private const double TargetMaxRunewordFragmentWeightTickets = 48.0;

    // Non-localized ESP records cannot safely carry Hangul in all Skyrim UI paths.
    // For ESP-facing display names, keep a printable ASCII variant (usually English side of "EN / KO").
    private static string ToPluginSafeName(string rawName, string editorId)
    {
        if (string.IsNullOrWhiteSpace(rawName))
        {
            return string.Empty;
        }

        var preferred = rawName;
        var slashIndex = preferred.IndexOf('/');
        if (slashIndex > 0)
        {
            preferred = preferred[..slashIndex].Trim();
        }

        var asciiPreferred = ToPrintableAscii(preferred);
        if (!string.IsNullOrWhiteSpace(asciiPreferred))
        {
            return asciiPreferred;
        }

        var asciiFull = ToPrintableAscii(rawName);
        if (!string.IsNullOrWhiteSpace(asciiFull))
        {
            return asciiFull;
        }

        return editorId;
    }

    private static string ToPrintableAscii(string value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return string.Empty;
        }

        var sb = new StringBuilder(value.Length);
        var previousWasSpace = false;

        foreach (var ch in value)
        {
            if (char.IsWhiteSpace(ch))
            {
                if (sb.Length > 0 && !previousWasSpace)
                {
                    sb.Append(' ');
                    previousWasSpace = true;
                }
                continue;
            }

            if (ch is >= ' ' and <= '~')
            {
                sb.Append(ch);
                previousWasSpace = false;
            }
        }

        return sb.ToString().Trim();
    }

    public static SkyrimMod Build(AffixSpec spec)
    {
        var mod = new SkyrimMod(ModKey.FromFileName(spec.ModKey), SkyrimRelease.SkyrimSE);

        if (spec.EslFlag)
        {
            mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;
        }

        // Keep the MCM quest FormID stable across content expansion by creating it first.
        // This prevents MCM duplicate/ghost entries caused by shifting generated record order.
        McmPluginBuilder.AddMcmQuest(mod);

        var seenMagicEffects = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenSpells = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var magicEffectsByEditorId = new Dictionary<string, MagicEffect>(StringComparer.OrdinalIgnoreCase);

        foreach (var tag in spec.Keywords.Tags)
        {
            AddKeyword(mod, tag.EditorId);
        }

        foreach (var affix in spec.Keywords.Affixes)
        {
            AddKeyword(mod, affix.EditorId);

            if (affix.Records?.MagicEffect is { } magicEffect &&
                !seenMagicEffects.Contains(magicEffect.EditorId))
            {
                var created = AddMagicEffect(mod, magicEffect);
                seenMagicEffects.Add(magicEffect.EditorId);
                magicEffectsByEditorId[magicEffect.EditorId] = created;
            }

            if (affix.Records?.Spell is { } spell &&
                !seenSpells.Contains(spell.EditorId))
            {
                AddSpell(mod, spell, magicEffectsByEditorId);
                seenSpells.Add(spell.EditorId);
            }
        }

        AddRunewordRuneFragments(mod);
        AddReforgeOrb(mod);

        if (spec.Loot is not null)
        {
            EnsureCurrencyDropLists(mod, spec.Loot);
        }

        return mod;
    }

    private static void AddRunewordRuneFragments(SkyrimMod mod)
    {
        // Rune fragment records are generated from the shared runeword contract data.
        // This keeps generator output in sync with runtime contract snapshots.
        const string sharedFragmentModel = @"Meshes\Clutter\SoulGem\SoulGemPiece01.nif";
        var runes = AffixSpecLoader.GetRunewordRuneLadder();

        foreach (var rune in runes)
        {
            var item = mod.MiscItems.AddNew();
            item.EditorID = $"CAFF_RuneFrag_{rune}";
            // Use ASCII-only for maximum font compatibility across UI mods.
            item.Name = $"Rune Fragment: {rune}";
            item.Weight = 0.0f;
            item.Value = 0;
            item.Model = new Model { File = sharedFragmentModel };
        }
    }

    private static void AddReforgeOrb(SkyrimMod mod)
    {
        // Runtime uses this exact EditorID in SKSE reforge grant/consume paths.
        // Keep visual consistency with rune fragments (soul gem shard style).
        const string sharedFragmentModel = @"Meshes\Clutter\SoulGem\SoulGemPiece01.nif";
        var item = mod.MiscItems.AddNew();
        item.EditorID = "CAFF_Misc_ReforgeOrb";
        item.Name = "Reforge Orb";
        item.Weight = 0.0f;
        item.Value = 0;
        item.Model = new Model { File = sharedFragmentModel };
    }

    private static (LeveledItem RunewordFragmentDropList, LeveledItem ReforgeOrbDropList) EnsureCurrencyDropLists(
        SkyrimMod mod,
        LootSpec lootSpec)
    {
        var runewordRuneWeightByName = BuildRunewordRuneWeightMap();
        var runewordFragmentItems = mod.MiscItems
            .Where(item => item.EditorID?.StartsWith(RunewordFragmentEditorIdPrefix, StringComparison.Ordinal) == true)
            .OrderBy(item => item.EditorID, StringComparer.Ordinal)
            .ToArray();
        if (runewordFragmentItems.Length == 0)
        {
            throw new InvalidDataException("No runeword fragment misc items generated before currency list setup.");
        }

        var runewordFragmentDropList = mod.LeveledItems.AddNew();
        runewordFragmentDropList.EditorID = "CAFF_LItem_RunewordFragmentDrops";
        runewordFragmentDropList.ChanceNone = ToChanceNonePercent(lootSpec.RunewordFragmentChancePercent);
        var maxConfiguredWeight = runewordFragmentItems
            .Select(item => ResolveRunewordFragmentWeight(item.EditorID, runewordRuneWeightByName))
            .DefaultIfEmpty(DefaultRunewordFragmentWeight)
            .Max();
        var weightScale = Math.Max(1.0, maxConfiguredWeight / TargetMaxRunewordFragmentWeightTickets);
        foreach (var fragmentItem in runewordFragmentItems)
        {
            var tickets = ResolveRunewordFragmentWeightTickets(
                fragmentItem.EditorID,
                runewordRuneWeightByName,
                weightScale);
            for (var ticket = 0; ticket < tickets; ++ticket)
            {
                AddLeveledItemEntry(runewordFragmentDropList, fragmentItem.ToLink<IItemGetter>(), level: 1, count: 1);
            }
        }

        var reforgeOrbItem = mod.MiscItems.SingleOrDefault(item => item.EditorID == "CAFF_Misc_ReforgeOrb");
        if (reforgeOrbItem is null)
        {
            throw new InvalidDataException("Reforge orb misc item is missing before currency list setup.");
        }

        var reforgeOrbDropList = mod.LeveledItems.AddNew();
        reforgeOrbDropList.EditorID = "CAFF_LItem_ReforgeOrbDrops";
        reforgeOrbDropList.ChanceNone = ToChanceNonePercent(lootSpec.ReforgeOrbChancePercent);
        AddLeveledItemEntryIfMissing(reforgeOrbDropList, reforgeOrbItem.ToLink<IItemGetter>(), level: 1, count: 1);

        return (runewordFragmentDropList, reforgeOrbDropList);
    }

    private static IReadOnlyDictionary<string, double> BuildRunewordRuneWeightMap()
    {
        var entries = RunewordContractCatalog.Load().RuneWeights;
        var map = new Dictionary<string, double>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in entries)
        {
            if (string.IsNullOrWhiteSpace(entry.Rune))
            {
                continue;
            }

            var normalizedWeight = entry.Weight > 0.0
                ? entry.Weight
                : DefaultRunewordFragmentWeight;
            map[entry.Rune.Trim()] = normalizedWeight;
        }

        return map;
    }

    private static double ResolveRunewordFragmentWeight(
        string? fragmentEditorId,
        IReadOnlyDictionary<string, double> runeWeightByName)
    {
        var runeName = TryExtractRuneName(fragmentEditorId);
        if (runeName is null)
        {
            return DefaultRunewordFragmentWeight;
        }

        return runeWeightByName.TryGetValue(runeName, out var configuredWeight)
            ? configuredWeight
            : DefaultRunewordFragmentWeight;
    }

    private static int ResolveRunewordFragmentWeightTickets(
        string? fragmentEditorId,
        IReadOnlyDictionary<string, double> runeWeightByName,
        double weightScale)
    {
        var weight = ResolveRunewordFragmentWeight(fragmentEditorId, runeWeightByName);
        var normalizedScale = weightScale > 0.0 ? weightScale : 1.0;
        var tickets = (int)Math.Round(weight / normalizedScale, MidpointRounding.AwayFromZero);
        return Math.Clamp(tickets, 1, MaxRunewordFragmentWeightTickets);
    }

    private static string? TryExtractRuneName(string? fragmentEditorId)
    {
        if (string.IsNullOrWhiteSpace(fragmentEditorId) ||
            !fragmentEditorId.StartsWith(RunewordFragmentEditorIdPrefix, StringComparison.Ordinal))
        {
            return null;
        }

        var runeName = fragmentEditorId[RunewordFragmentEditorIdPrefix.Length..].Trim();
        return runeName.Length == 0 ? null : runeName;
    }

    private static Percent ToChanceNonePercent(double dropChancePercent)
    {
        var clampedChance = Math.Clamp(dropChancePercent, 0.0, 100.0);
        var chanceNoneFraction = (100.0 - clampedChance) / 100.0;
        return new Percent(chanceNoneFraction);
    }

    private static void AddLeveledItemEntryIfMissing(
        LeveledItem target,
        IFormLink<IItemGetter> reference,
        short level,
        short count)
    {
        target.Entries ??= [];
        var hasEntry = target.Entries.Any(entry => entry.Data?.Reference.FormKey == reference.FormKey);
        if (hasEntry)
        {
            return;
        }

        target.Entries.Add(new LeveledItemEntry
        {
            Data = new LeveledItemEntryData
            {
                Level = level,
                Count = count,
                Reference = reference,
            },
        });
    }

    private static void AddLeveledItemEntry(
        LeveledItem target,
        IFormLink<IItemGetter> reference,
        short level,
        short count)
    {
        target.Entries ??= [];
        target.Entries.Add(new LeveledItemEntry
        {
            Data = new LeveledItemEntryData
            {
                Level = level,
                Count = count,
                Reference = reference,
            },
        });
    }

    private static void AddKeyword(SkyrimMod mod, string editorId)
    {
        var kw = mod.Keywords.AddNew();
        kw.EditorID = editorId;
    }

    private static MagicEffect AddMagicEffect(SkyrimMod mod, MagicEffectRecordSpec spec)
    {
        var mgef = mod.MagicEffects.AddNew();
        mgef.EditorID = spec.EditorId;

        if (!string.IsNullOrWhiteSpace(spec.Name))
        {
            mgef.Name = ToPluginSafeName(spec.Name, spec.EditorId);
        }

        if (!Enum.TryParse<ActorValue>(spec.ActorValue, ignoreCase: true, out var actorValue))
        {
            throw new InvalidDataException($"Unknown ActorValue: {spec.ActorValue} (MagicEffect: {spec.EditorId})");
        }

        if (!string.IsNullOrWhiteSpace(spec.ResistValue))
        {
            if (!Enum.TryParse<ActorValue>(spec.ResistValue, ignoreCase: true, out var resistValue))
            {
                throw new InvalidDataException(
                    $"Unknown ActorValue: {spec.ResistValue} (MagicEffect.resistValue: {spec.EditorId})");
            }

            mgef.ResistValue = resistValue;
        }

        // Default hit VFX: reuse vanilla enchantment hit shader + impact data where possible.
        // This keeps CK work minimal and makes procs visually readable in-game.
        // FormIDs were sourced from Skyrim.esm (SE), and are stable across runtimes.
        if (mgef.ResistValue == ActorValue.ResistFire)
        {
            // EnchFireDamageFFContact
            mgef.HitShader.SetTo(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x01B212));
            mgef.ImpactData.SetTo(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x01C2AF));
        }
        else if (mgef.ResistValue == ActorValue.ResistFrost)
        {
            // EnchFrostDamageFFContact
            mgef.HitShader.SetTo(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x0435A3));
            mgef.ImpactData.SetTo(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x032DA7));
        }
        else if (mgef.ResistValue == ActorValue.ResistShock)
        {
            // EnchShockDamageFFContact
            mgef.HitShader.SetTo(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x057C67));
            mgef.ImpactData.SetTo(new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"), 0x038B05));
        }

        if (!string.IsNullOrWhiteSpace(spec.MagicSkill))
        {
            if (!Enum.TryParse<ActorValue>(spec.MagicSkill, ignoreCase: true, out var magicSkill))
            {
                throw new InvalidDataException(
                    $"Unknown ActorValue: {spec.MagicSkill} (MagicEffect.magicSkill: {spec.EditorId})");
            }

            mgef.MagicSkill = magicSkill;
        }

        var archetypeType = MagicEffectArchetype.TypeEnum.ValueModifier;
        if (!string.IsNullOrWhiteSpace(spec.Archetype))
        {
            if (!Enum.TryParse<MagicEffectArchetype.TypeEnum>(spec.Archetype, ignoreCase: true, out archetypeType))
            {
                throw new InvalidDataException(
                    $"Unknown MagicEffect archetype: {spec.Archetype} (MagicEffect: {spec.EditorId})");
            }
        }

        mgef.Archetype = new MagicEffectArchetype
        {
            Type = archetypeType,
            ActorValue = actorValue,
        };

        var flags = (MagicEffect.Flag)0;
        if (spec.Hostile)
        {
            flags |= MagicEffect.Flag.Hostile | MagicEffect.Flag.Detrimental;
        }

        if (spec.Recover)
        {
            flags |= MagicEffect.Flag.Recover;
        }

        mgef.Flags = flags;

        return mgef;
    }

    private static void AddSpell(SkyrimMod mod, SpellRecordSpec spec, IReadOnlyDictionary<string, MagicEffect> magicEffectsByEditorId)
    {
        var spell = mod.Spells.AddNew();
        spell.EditorID = spec.EditorId;

        if (!string.IsNullOrWhiteSpace(spec.Name))
        {
            spell.Name = ToPluginSafeName(spec.Name, spec.EditorId);
        }

        spell.Type = spec.SpellType switch
        {
            "Ability" => Mutagen.Bethesda.Skyrim.SpellType.Ability,
            _ => Mutagen.Bethesda.Skyrim.SpellType.Spell,
        };
        spell.CastType = spec.CastType switch
        {
            "ConstantEffect" => Mutagen.Bethesda.Skyrim.CastType.ConstantEffect,
            _ => Mutagen.Bethesda.Skyrim.CastType.FireAndForget,
        };
        spell.TargetType = spec.Delivery switch
        {
            "Self" => TargetType.Self,
            "TargetActor" => TargetType.TargetActor,
            _ => throw new InvalidDataException($"Unknown Spell delivery: {spec.Delivery} (Spell: {spec.EditorId})"),
        };
        spell.Range = (spell.TargetType == TargetType.TargetActor && spell.CastType != Mutagen.Bethesda.Skyrim.CastType.ConstantEffect) ? 4096.0f : 0.0f;
        spell.Flags = (SpellDataFlag)0;

        if (!magicEffectsByEditorId.TryGetValue(spec.Effect.MagicEffectEditorId, out var magicEffect))
        {
            throw new InvalidDataException(
                $"Spell {spec.EditorId} references missing MagicEffect {spec.Effect.MagicEffectEditorId}. " +
                "Define it in records.magicEffect (or generate it in another affix).");
        }

        // MagicEffects are validated (and used for various runtime behaviors) by their own cast/target types.
        // If we leave defaults here, the spell can be cast but silently fail to apply to the intended target.
        magicEffect.CastType = spell.CastType;
        magicEffect.TargetType = spell.TargetType;

        var effect = new Effect
        {
            Data = new EffectData
            {
                Magnitude = (float)spec.Effect.Magnitude,
                Duration = spec.Effect.Duration,
                Area = spec.Effect.Area,
            },
        };
        effect.BaseEffect.SetTo(magicEffect);
        spell.Effects.Add(effect);
    }
}
