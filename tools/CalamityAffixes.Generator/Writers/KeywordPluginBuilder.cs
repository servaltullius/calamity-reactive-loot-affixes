using CalamityAffixes.Generator.Spec;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;

namespace CalamityAffixes.Generator.Writers;

public static class KeywordPluginBuilder
{
    private static readonly ModKey SkyrimModKey = ModKey.FromNameAndExtension("Skyrim.esm");
    private const string CurrencyDropModeRuntime = "runtime";
    private const string CurrencyDropModeLeveledList = "leveledlist";
    private const string CurrencyDropModeHybrid = "hybrid";

    // Curated Skyrim.esm LVLI targets for world-drop style currency:
    // - generic loot pools (gems/special loot)
    // - hostile humanoid/undead death items (corpse loot)
    // Keep this list conservative to avoid touching equipment-only pools.
    private static readonly IReadOnlyList<FormKey> DefaultCurrencyLeveledListTargets =
    [
        new FormKey(SkyrimModKey, 0x00068525), // LItemGems
        new FormKey(SkyrimModKey, 0x000F961D), // LItemGemsSmall
        new FormKey(SkyrimModKey, 0x001046E2), // LItemGemsSmall25
        new FormKey(SkyrimModKey, 0x0010E0E0), // LItemGemsSmall10
        new FormKey(SkyrimModKey, 0x001046E3), // LItemGems75
        new FormKey(SkyrimModKey, 0x00087138), // LItemLootIMineralsProcessed
        new FormKey(SkyrimModKey, 0x0010B2C0), // LItemSpecialLoot10
        new FormKey(SkyrimModKey, 0x001031D0), // LItemSpecialLoot100
        new FormKey(SkyrimModKey, 0x000C3C9B), // DeathItemBandit
        new FormKey(SkyrimModKey, 0x000C3C9E), // DeathItemBanditWizard
        new FormKey(SkyrimModKey, 0x0003AD7F), // DeathItemDraugr
        new FormKey(SkyrimModKey, 0x0010FACC), // DeathItemDraugrMage
        new FormKey(SkyrimModKey, 0x0003AD84), // DeathItemFalmer
        new FormKey(SkyrimModKey, 0x0003ADA0), // DeathItemVampire
        new FormKey(SkyrimModKey, 0x0003AD7E), // DeathItemDragonPriest
        new FormKey(SkyrimModKey, 0x0003ADA5), // DeathItemDragon01
    ];

    private enum CurrencyDropMode
    {
        Runtime = 0,
        LeveledList = 1,
        Hybrid = 2,
    }

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

    public static SkyrimMod Build(
        AffixSpec spec,
        Func<FormKey, ILeveledItemGetter?>? leveledListResolver = null,
        IReadOnlyList<FormKey>? autoDiscoveredCurrencyLeveledListTargets = null)
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
            var (runewordFragmentDropList, reforgeOrbDropList) = EnsureCurrencyDropLists(mod, spec.Loot);
            var currencyDropMode = ResolveCurrencyDropMode(spec.Loot);
            if (currencyDropMode is CurrencyDropMode.LeveledList or CurrencyDropMode.Hybrid)
            {
                AddCurrencyLeveledListDrops(
                    mod,
                    spec.Loot,
                    runewordFragmentDropList,
                    reforgeOrbDropList,
                    leveledListResolver,
                    autoDiscoveredCurrencyLeveledListTargets);
            }
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

    private static CurrencyDropMode ResolveCurrencyDropMode(LootSpec? loot)
    {
        var modeRaw = loot?.CurrencyDropMode ?? "runtime";
        var mode = modeRaw.Trim().ToLowerInvariant();
        return mode switch
        {
            CurrencyDropModeLeveledList => CurrencyDropMode.LeveledList,
            CurrencyDropModeHybrid => CurrencyDropMode.Hybrid,
            _ => CurrencyDropMode.Runtime,
        };
    }

    private static (LeveledItem RunewordFragmentDropList, LeveledItem ReforgeOrbDropList) EnsureCurrencyDropLists(
        SkyrimMod mod,
        LootSpec lootSpec)
    {
        var runewordFragmentItems = mod.MiscItems
            .Where(item => item.EditorID?.StartsWith("CAFF_RuneFrag_", StringComparison.Ordinal) == true)
            .OrderBy(item => item.EditorID, StringComparer.Ordinal)
            .ToArray();
        if (runewordFragmentItems.Length == 0)
        {
            throw new InvalidDataException("No runeword fragment misc items generated before currency list setup.");
        }

        var runewordFragmentDropList = mod.LeveledItems.AddNew();
        runewordFragmentDropList.EditorID = "CAFF_LItem_RunewordFragmentDrops";
        runewordFragmentDropList.ChanceNone = ToChanceNonePercent(lootSpec.RunewordFragmentChancePercent);
        foreach (var fragmentItem in runewordFragmentItems)
        {
            AddLeveledItemEntryIfMissing(runewordFragmentDropList, fragmentItem.ToLink<IItemGetter>(), level: 1, count: 1);
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

    private static void AddCurrencyLeveledListDrops(
        SkyrimMod mod,
        LootSpec lootSpec,
        LeveledItem runewordFragmentDropList,
        LeveledItem reforgeOrbDropList,
        Func<FormKey, ILeveledItemGetter?>? leveledListResolver,
        IReadOnlyList<FormKey>? autoDiscoveredCurrencyLeveledListTargets)
    {
        if (leveledListResolver is null)
        {
            throw new InvalidDataException(
                "currencyDropMode=leveledList/hybrid requires a leveled-list resolver loaded from masters. " +
                "Provide --masters <GameDataPath> when running the generator.");
        }

        var targetLists = ResolveCurrencyLeveledListTargets(lootSpec, autoDiscoveredCurrencyLeveledListTargets);
        foreach (var targetListKey in targetLists)
        {
            var targetList = GetOrAddLeveledListOverride(mod, targetListKey, leveledListResolver);
            AddLeveledItemEntryIfMissing(targetList, runewordFragmentDropList.ToLink<IItemGetter>(), level: 1, count: 1);
            AddLeveledItemEntryIfMissing(targetList, reforgeOrbDropList.ToLink<IItemGetter>(), level: 1, count: 1);
        }
    }

    private static IReadOnlyList<FormKey> ResolveCurrencyLeveledListTargets(
        LootSpec lootSpec,
        IReadOnlyList<FormKey>? autoDiscoveredTargets)
    {
        IReadOnlyList<FormKey> configuredTargets;
        if (lootSpec.CurrencyLeveledListTargets is null || lootSpec.CurrencyLeveledListTargets.Count == 0)
        {
            configuredTargets = DefaultCurrencyLeveledListTargets;
        }
        else
        {
            var targets = new List<FormKey>(lootSpec.CurrencyLeveledListTargets.Count);
            var seen = new HashSet<FormKey>();
            foreach (var raw in lootSpec.CurrencyLeveledListTargets)
            {
                if (string.IsNullOrWhiteSpace(raw))
                {
                    continue;
                }

                var parsed = ParseFormKey(raw.Trim(), "loot.currencyLeveledListTargets");
                if (seen.Add(parsed))
                {
                    targets.Add(parsed);
                }
            }

            configuredTargets = targets.Count == 0 ? DefaultCurrencyLeveledListTargets : targets;
        }

        if (autoDiscoveredTargets is null || autoDiscoveredTargets.Count == 0)
        {
            return configuredTargets;
        }

        var merged = new List<FormKey>(configuredTargets.Count + autoDiscoveredTargets.Count);
        var mergedSeen = new HashSet<FormKey>();

        foreach (var target in configuredTargets)
        {
            if (mergedSeen.Add(target))
            {
                merged.Add(target);
            }
        }

        foreach (var target in autoDiscoveredTargets)
        {
            if (mergedSeen.Add(target))
            {
                merged.Add(target);
            }
        }

        return merged;
    }

    private static FormKey ParseFormKey(string value, string fieldName)
    {
        var split = value.Split('|', 2, StringSplitOptions.TrimEntries);
        if (split.Length != 2 || string.IsNullOrWhiteSpace(split[0]) || string.IsNullOrWhiteSpace(split[1]))
        {
            throw new InvalidDataException($"{fieldName} entry must be 'ModName.esm|00ABCDEF' (got: {value}).");
        }

        var modKey = ModKey.FromNameAndExtension(split[0]);
        var formIdText = split[1].StartsWith("0x", StringComparison.OrdinalIgnoreCase)
            ? split[1][2..]
            : split[1];
        if (!uint.TryParse(formIdText, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var formId))
        {
            throw new InvalidDataException($"{fieldName} entry has invalid FormID hex value: {value}");
        }

        return new FormKey(modKey, formId);
    }

    private static LeveledItem GetOrAddLeveledListOverride(
        SkyrimMod mod,
        FormKey targetListFormKey,
        Func<FormKey, ILeveledItemGetter?> leveledListResolver)
    {
        var existing = mod.LeveledItems.FirstOrDefault(record => record.FormKey == targetListFormKey);
        if (existing is not null)
        {
            return existing;
        }

        var baseRecord = leveledListResolver(targetListFormKey);
        if (baseRecord is null)
        {
            throw new InvalidDataException(
                $"Failed to resolve target leveled list from masters: {targetListFormKey}. " +
                "Ensure --masters points to game Data containing required plugins.");
        }

        // Create a true override that preserves original entries/flags/chanceNone.
        return mod.LeveledItems.GetOrAddAsOverride(baseRecord);
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
