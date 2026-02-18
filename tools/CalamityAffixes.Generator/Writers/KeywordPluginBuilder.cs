using CalamityAffixes.Generator.Spec;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;
using System.IO;
using System.Text;

namespace CalamityAffixes.Generator.Writers;

public static class KeywordPluginBuilder
{
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

        return mod;
    }

    private static void AddRunewordRuneFragments(SkyrimMod mod)
    {
        // Keep this list in sync with the hardcoded runeword recipes in the SKSE plugin.
        // Include the full D2 rune ladder so future recipe expansion does not require another plugin format change.
        // These are physical inventory items so players can "see and collect" rune materials.
        const string sharedFragmentModel = @"Meshes\Clutter\SoulGem\SoulGemPiece01.nif";
        var runes = new[]
        {
            "El",
            "Eld",
            "Tir",
            "Nef",
            "Eth",
            "Ith",
            "Tal",
            "Ral",
            "Ort",
            "Thul",
            "Amn",
            "Sol",
            "Shael",
            "Dol",
            "Hel",
            "Io",
            "Lum",
            "Ko",
            "Fal",
            "Lem",
            "Pul",
            "Um",
            "Mal",
            "Ist",
            "Gul",
            "Vex",
            "Ohm",
            "Lo",
            "Sur",
            "Ber",
            "Jah",
            "Cham",
            "Zod",
        };

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
        var item = mod.MiscItems.AddNew();
        item.EditorID = "CAFF_Misc_ReforgeOrb";
        item.Name = "Reforge Orb";
        item.Weight = 0.0f;
        item.Value = 0;
        item.Model = new Model { File = @"Meshes\Clutter\SoulGem\SoulGemGrandFilled.nif" };
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
