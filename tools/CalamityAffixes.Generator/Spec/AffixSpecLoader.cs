using System.Text.Json;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Spec;

public static class AffixSpecLoader
{
    private const string ValidationContractRelativePath = "tools/affix_validation_contract.json";
    private static readonly char[] InvalidModKeyFileNameChars = ['<', '>', ':', '"', '/', '\\', '|', '?', '*'];
    private static readonly ValidationContract Contract = LoadValidationContract();
    private static readonly RunewordContractCatalog.Snapshot RunewordContract = RunewordContractCatalog.Load();
    private static readonly HashSet<string> SupportedTriggers = Contract.SupportedTriggers;
    private static readonly HashSet<string> SupportedActionTypes = Contract.SupportedActionTypes;
    private static readonly string[] SealedLegacyDragonMagicEffectEditorIds =
    [
        "CAFF_MGEF_RW_DRAGON_RESIST_FIRE",
        "CAFF_MGEF_RW_DRAGON_RESIST_FROST",
        "CAFF_MGEF_RW_DRAGON_RESIST_SHOCK",
    ];

    public static AffixSpec Load(string path)
    {
        var json = File.ReadAllText(path);
        using var rootDoc = JsonDocument.Parse(json);
        ValidateRemovedLegacyLootFields(rootDoc.RootElement);
        ValidateAppendedRecordJsonShape(rootDoc.RootElement);

        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
        };

        var spec = JsonSerializer.Deserialize<AffixSpec>(json, options);
        if (spec is null)
        {
            throw new InvalidDataException($"Failed to parse spec JSON: {path}");
        }

        Validate(spec);
        return spec;
    }

    public static string BuildValidationContractJson(bool indented = true)
    {
        var supportedTriggers = SupportedTriggers.ToArray();
        var supportedActionTypes = SupportedActionTypes.ToArray();
        Array.Sort(supportedTriggers, StringComparer.Ordinal);
        Array.Sort(supportedActionTypes, StringComparer.Ordinal);

        var runewordCatalog = RunewordContract.Recipes
            .Select(recipe => new
            {
                id = recipe.Id,
                name = recipe.DisplayName,
                runes = recipe.Runes,
                resultAffixId = recipe.ResultAffixId,
                recommendedBase = recipe.RecommendedBase,
            })
            .ToArray();

        var runewordRuneWeights = RunewordContract.RuneWeights
            .Select(weight => new
            {
                rune = weight.Rune,
                weight = weight.Weight,
            })
            .ToArray();

        var payload = new
        {
            supportedTriggers,
            supportedActionTypes,
            runewordCatalog,
            runewordRuneWeights,
        };

        return JsonSerializer.Serialize(
            payload,
            new JsonSerializerOptions
            {
                WriteIndented = indented,
            });
    }

    public static IReadOnlyList<string> GetRunewordRuneLadder()
    {
        return RunewordContract.RuneWeights
            .Select(weight => weight.Rune)
            .ToArray();
    }

    internal static IReadOnlyList<RunewordContractCatalog.RuneWeightEntry> GetRunewordRuneWeights()
    {
        return RunewordContract.RuneWeights;
    }

    internal static void ValidateRunewordContractReferences(AffixSpec spec)
    {
        var affixIds = new HashSet<string>(
            spec.Keywords.Affixes.Select(affix => affix.Id),
            StringComparer.OrdinalIgnoreCase);
        var missingResultAffixIds = RunewordContract.Recipes
            .Select(recipe => recipe.ResultAffixId)
            .Where(resultAffixId => !affixIds.Contains(resultAffixId))
            .OrderBy(resultAffixId => resultAffixId, StringComparer.OrdinalIgnoreCase)
            .ToArray();

        if (missingResultAffixIds.Length > 0)
        {
            throw new InvalidDataException(
                "Runeword contract references missing result affixes: " +
                string.Join(", ", missingResultAffixIds));
        }
    }

    private static void Validate(AffixSpec spec)
    {
        ValidateModKey(spec.ModKey);
        ValidateLootPolicy(spec.Loot);
        ValidateSpidRulePolicy(spec.Keywords);
        ValidateAppendOnlyRecordAllocationContract(spec.Keywords);

        if (!spec.ModKey.EndsWith(".esp", StringComparison.OrdinalIgnoreCase) &&
            !spec.ModKey.EndsWith(".esm", StringComparison.OrdinalIgnoreCase) &&
            !spec.ModKey.EndsWith(".esl", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException($"modKey must end with .esp/.esm/.esl (got: {spec.ModKey})");
        }

        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenMagicEffects = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenSpells = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenArtObjects = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var spellDefinitions = new List<SpellRecordSpec>();
        foreach (var kw in spec.Keywords.Tags)
        {
            if (!seen.Add(kw.EditorId))
            {
                throw new InvalidDataException($"Duplicate keyword editorId: {kw.EditorId}");
            }
        }

        foreach (var affix in spec.Keywords.Affixes)
        {
            if (!seen.Add(affix.EditorId))
            {
                throw new InvalidDataException($"Duplicate keyword editorId: {affix.EditorId}");
            }

            ValidateRuntimeShape(affix);
            ValidateWeaponSubtypePolicy(affix);

            var records = affix.Records;
            var magicEffects = records?.ResolveMagicEffects() ?? [];
            foreach (var magicEffect in magicEffects)
            {
                if (!seenMagicEffects.Add(magicEffect.EditorId))
                {
                    throw new InvalidDataException($"Duplicate MagicEffect editorId: {magicEffect.EditorId}");
                }

                if (!Enum.TryParse<ActorValue>(magicEffect.ActorValue, ignoreCase: true, out _))
                {
                    throw new InvalidDataException($"Unknown ActorValue: {magicEffect.ActorValue} (MagicEffect: {magicEffect.EditorId})");
                }
            }

            var spells = records?.ResolveSpells() ?? [];
            foreach (var spell in spells)
            {
                if (!seenSpells.Add(spell.EditorId))
                {
                    throw new InvalidDataException($"Duplicate Spell editorId: {spell.EditorId}");
                }

                if (spell.Delivery is not ("Self" or "TargetActor"))
                {
                    throw new InvalidDataException($"Unknown Spell delivery: {spell.Delivery} (Spell: {spell.EditorId})");
                }

                if (spell.SpellType is not (null or "Spell" or "Ability"))
                {
                    throw new InvalidDataException(
                        $"Unknown Spell spellType: {spell.SpellType} (Spell: {spell.EditorId}). Valid: Spell, Ability.");
                }

                if (spell.CastType is not (null or "FireAndForget" or "ConstantEffect"))
                {
                    throw new InvalidDataException(
                        $"Unknown Spell castType: {spell.CastType} (Spell: {spell.EditorId}). Valid: FireAndForget, ConstantEffect.");
                }

                var effects = spell.ResolveEffects();
                if (effects.Count == 0)
                {
                    throw new InvalidDataException(
                        $"Spell {spell.EditorId} requires at least one effect via effect or effects[].");
                }

                for (var i = 0; i < effects.Count; i += 1)
                {
                    if (string.IsNullOrWhiteSpace(effects[i].MagicEffectEditorId))
                    {
                        throw new InvalidDataException(
                            $"Spell {spell.EditorId} has an empty magicEffectEditorId at effect index {i}.");
                    }
                }

                spellDefinitions.Add(spell);
            }
        }

        foreach (var magicEffect in spec.Keywords.AppendedMagicEffects)
        {
            if (!seenMagicEffects.Add(magicEffect.EditorId))
            {
                throw new InvalidDataException($"Duplicate appended MagicEffect editorId: {magicEffect.EditorId}");
            }

            if (!Enum.TryParse<ActorValue>(magicEffect.ActorValue, ignoreCase: true, out _))
            {
                throw new InvalidDataException(
                    $"Unknown ActorValue: {magicEffect.ActorValue} (appended MagicEffect: {magicEffect.EditorId})");
            }
        }

        foreach (var appendedRecord in spec.Keywords.AppendedRecords)
        {
            switch (appendedRecord.Type)
            {
                case "MagicEffect" when appendedRecord.MagicEffect is not null && appendedRecord.Spell is null && appendedRecord.ArtObject is null:
                {
                    var magicEffect = appendedRecord.MagicEffect;
                    if (!seenMagicEffects.Add(magicEffect.EditorId))
                    {
                        throw new InvalidDataException(
                            $"Duplicate appended MagicEffect editorId: {magicEffect.EditorId}");
                    }

                    if (!Enum.TryParse<ActorValue>(magicEffect.ActorValue, ignoreCase: true, out _))
                    {
                        throw new InvalidDataException(
                            $"Unknown ActorValue: {magicEffect.ActorValue} (appended MagicEffect: {magicEffect.EditorId})");
                    }
                    break;
                }
                case "Spell" when appendedRecord.Spell is not null && appendedRecord.MagicEffect is null && appendedRecord.ArtObject is null:
                {
                    var spell = appendedRecord.Spell;
                    if (!seenSpells.Add(spell.EditorId))
                    {
                        throw new InvalidDataException($"Duplicate appended Spell editorId: {spell.EditorId}");
                    }

                    if (spell.Delivery is not ("Self" or "TargetActor"))
                    {
                        throw new InvalidDataException(
                            $"Unknown Spell delivery: {spell.Delivery} (appended Spell: {spell.EditorId})");
                    }

                    if (spell.SpellType is not (null or "Spell" or "Ability"))
                    {
                        throw new InvalidDataException(
                            $"Unknown Spell spellType: {spell.SpellType} (appended Spell: {spell.EditorId}). Valid: Spell, Ability.");
                    }

                    if (spell.CastType is not (null or "FireAndForget" or "ConstantEffect"))
                    {
                        throw new InvalidDataException(
                            $"Unknown Spell castType: {spell.CastType} (appended Spell: {spell.EditorId}). Valid: FireAndForget, ConstantEffect.");
                    }

                    var effects = spell.ResolveEffects();
                    if (effects.Count == 0)
                    {
                        throw new InvalidDataException(
                            $"Appended Spell {spell.EditorId} requires at least one effect via effect or effects[].");
                    }

                    for (var i = 0; i < effects.Count; i += 1)
                    {
                        if (string.IsNullOrWhiteSpace(effects[i].MagicEffectEditorId))
                        {
                            throw new InvalidDataException(
                                $"Appended Spell {spell.EditorId} has an empty magicEffectEditorId at effect index {i}.");
                        }
                    }

                    spellDefinitions.Add(spell);
                    break;
                }
                case "ArtObject" when appendedRecord.ArtObject is not null && appendedRecord.MagicEffect is null && appendedRecord.Spell is null:
                {
                    var artObject = appendedRecord.ArtObject;
                    if (!seenArtObjects.Add(artObject.EditorId))
                    {
                        throw new InvalidDataException($"Duplicate appended ArtObject editorId: {artObject.EditorId}");
                    }

                    if (string.IsNullOrWhiteSpace(artObject.EditorId))
                    {
                        throw new InvalidDataException("Appended ArtObject requires a non-empty editorId.");
                    }

                    if (string.IsNullOrWhiteSpace(artObject.ModelPath))
                    {
                        throw new InvalidDataException(
                            $"Appended ArtObject requires a non-empty modelPath (ArtObject: {artObject.EditorId}).");
                    }

                    if (artObject.ArtType is not "MagicHitEffect")
                    {
                        throw new InvalidDataException(
                            $"Unsupported ArtObject artType: {artObject.ArtType} " +
                            $"(ArtObject: {artObject.EditorId}). Valid: MagicHitEffect.");
                    }
                    break;
                }
                default:
                    throw new InvalidDataException(
                        "keywords.appendedRecords entries must use type MagicEffect with only magicEffect, " +
                        "type Spell with only spell, or type ArtObject with only artObject.");
            }
        }

        foreach (var spell in spellDefinitions)
        {
            foreach (var effect in spell.ResolveEffects())
            {
                if (!seenMagicEffects.Contains(effect.MagicEffectEditorId))
                {
                    throw new InvalidDataException(
                        $"Spell {spell.EditorId} references missing MagicEffect {effect.MagicEffectEditorId}.");
                }
            }
        }
    }

    private static void ValidateWeaponSubtypePolicy(AffixDefinition affix)
    {
        if (affix.WeaponSubtypes is null)
        {
            return;
        }

        if (!string.Equals(affix.Slot, "suffix", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException(
                $"weaponSubtypes is supported only for suffix affixes (affix: {affix.Id}).");
        }

        if (affix.WeaponSubtypes.Count == 0)
        {
            throw new InvalidDataException(
                $"weaponSubtypes must contain at least one subtype (affix: {affix.Id}).");
        }

        var allowed = new HashSet<string>(StringComparer.Ordinal)
        {
            "OneHandedMelee",
            "TwoHandedMelee",
            "Bow",
            "Crossbow"
        };
        var seen = new HashSet<string>(StringComparer.Ordinal);
        foreach (var subtype in affix.WeaponSubtypes)
        {
            if (!allowed.Contains(subtype))
            {
                throw new InvalidDataException(
                    $"Unknown weaponSubtypes value '{subtype}' (affix: {affix.Id}).");
            }
            if (!seen.Add(subtype))
            {
                throw new InvalidDataException(
                    $"Duplicate weaponSubtypes value '{subtype}' (affix: {affix.Id}).");
            }
        }
    }

    private static void ValidateLootPolicy(LootSpec? loot)
    {
        if (loot is null)
        {
            return;
        }

        var dropMode = loot.CurrencyDropMode?.Trim();
        if (!string.IsNullOrEmpty(dropMode) &&
            !string.Equals(dropMode, "hybrid", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException(
                $"loot.currencyDropMode supports only 'hybrid' (got: {dropMode}).");
        }
    }

    private static void ValidateSpidRulePolicy(KeywordSpec keywords)
    {
        static bool IsLegacyCorpseDropPerkLine(string line)
        {
            if (!line.TrimStart().StartsWith("Perk", StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }

            return line.Contains("CAFF_Perk_DeathDropRunewordFragment", StringComparison.OrdinalIgnoreCase) ||
                   line.Contains("CAFF_Perk_DeathDropReforgeOrb", StringComparison.OrdinalIgnoreCase);
        }

        for (var i = 0; i < keywords.SpidRules.Count; i += 1)
        {
            var rule = keywords.SpidRules[i];
            if (!rule.TryGetValue("line", out var rawLine) || rawLine is null)
            {
                continue;
            }

            string? line = rawLine switch
            {
                string s => s,
                JsonElement element when element.ValueKind == JsonValueKind.String => element.GetString(),
                _ => null,
            };

            if (string.IsNullOrWhiteSpace(line))
            {
                continue;
            }

            if (line.TrimStart().StartsWith("DeathItem", StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(
                    $"keywords.spidRules[{i}] uses DeathItem distribution, which is disallowed in hybrid policy. " +
                    "Use SPID Item distribution.");
            }

            if (IsLegacyCorpseDropPerkLine(line))
            {
                throw new InvalidDataException(
                    $"keywords.spidRules[{i}] uses legacy corpse-drop Perk distribution, which is disallowed in hybrid policy. " +
                    "Use SPID Item distribution.");
            }
        }
    }

    private static void ValidateRemovedLegacyLootFields(JsonElement root)
    {
        if (!root.TryGetProperty("loot", out var loot) || loot.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        if (loot.TryGetProperty("currencyLeveledListTargets", out _))
        {
            throw new InvalidDataException(
                "loot.currencyLeveledListTargets was removed. " +
                "Use hybrid policy only (corpse=SPID, container/world=runtime).");
        }

        if (loot.TryGetProperty("currencyLeveledListAutoDiscoverDeathItems", out _))
        {
            throw new InvalidDataException(
                "loot.currencyLeveledListAutoDiscoverDeathItems was removed. " +
                "Use hybrid policy only (corpse=SPID, container/world=runtime).");
        }
    }

    private static void ValidateAppendedRecordJsonShape(JsonElement root)
    {
        if (!TryGetUniquePropertyIgnoreCase(root, "keywords", out var keywords) ||
            keywords.ValueKind != JsonValueKind.Object ||
            !TryGetUniquePropertyIgnoreCase(keywords, "appendedRecords", out var appendedRecords))
        {
            return;
        }

        if (appendedRecords.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidDataException("keywords.appendedRecords must be an array.");
        }

        var index = 0;
        foreach (var item in appendedRecords.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.Object)
            {
                throw new InvalidDataException($"keywords.appendedRecords[{index}] must be an object.");
            }

            var seenProperties = new HashSet<string>(StringComparer.Ordinal);
            foreach (var property in item.EnumerateObject())
            {
                if (!seenProperties.Add(property.Name))
                {
                    throw new InvalidDataException(
                        $"keywords.appendedRecords[{index}] contains duplicate property '{property.Name}'.");
                }

                if (property.Name is not ("type" or "magicEffect" or "spell" or "artObject"))
                {
                    throw new InvalidDataException(
                        $"keywords.appendedRecords[{index}] contains unsupported property '{property.Name}'.");
                }
            }

            if (!item.TryGetProperty("type", out var typeElement) || typeElement.ValueKind != JsonValueKind.String)
            {
                throw new InvalidDataException($"keywords.appendedRecords[{index}].type must be a string.");
            }

            var type = typeElement.GetString();
            var hasMagicEffect = item.TryGetProperty("magicEffect", out var magicEffectElement);
            var hasSpell = item.TryGetProperty("spell", out var spellElement);
            var hasArtObject = item.TryGetProperty("artObject", out var artObjectElement);
            switch (type)
            {
                case "MagicEffect" when hasMagicEffect && !hasSpell && !hasArtObject && magicEffectElement.ValueKind == JsonValueKind.Object:
                case "Spell" when hasSpell && !hasMagicEffect && !hasArtObject && spellElement.ValueKind == JsonValueKind.Object:
                case "ArtObject" when hasArtObject && !hasMagicEffect && !hasSpell && artObjectElement.ValueKind == JsonValueKind.Object:
                    break;
                case "MagicEffect":
                    throw new InvalidDataException(
                        $"keywords.appendedRecords[{index}] type MagicEffect requires only an object magicEffect payload.");
                case "Spell":
                    throw new InvalidDataException(
                        $"keywords.appendedRecords[{index}] type Spell requires only an object spell payload.");
                case "ArtObject":
                    throw new InvalidDataException(
                        $"keywords.appendedRecords[{index}] type ArtObject requires only an object artObject payload.");
                default:
                    throw new InvalidDataException(
                        $"keywords.appendedRecords[{index}].type must be MagicEffect, Spell, or ArtObject (got: {type ?? "<null>"}).");
            }

            index += 1;
        }
    }

    internal static void ValidateAppendOnlyRecordAllocationContract(KeywordSpec keywords)
    {
        if (keywords.AppendedMagicEffects.Count == 0 && keywords.AppendedRecords.Count == 0)
        {
            return;
        }

        var actualEditorIds = keywords.AppendedMagicEffects.Select(record => record.EditorId).ToArray();
        if (!actualEditorIds.SequenceEqual(SealedLegacyDragonMagicEffectEditorIds, StringComparer.Ordinal))
        {
            throw new InvalidDataException(
                "keywords.appendedMagicEffects is the sealed v1.4.0 Dragon allocation block and must contain " +
                "exactly these three EditorIDs in order: " +
                string.Join(", ", SealedLegacyDragonMagicEffectEditorIds));
        }
    }

    private static bool TryGetUniquePropertyIgnoreCase(
        JsonElement parent,
        string propertyName,
        out JsonElement value)
    {
        value = default;
        if (parent.ValueKind != JsonValueKind.Object)
        {
            return false;
        }

        var found = false;
        foreach (var property in parent.EnumerateObject())
        {
            if (!property.Name.Equals(propertyName, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            if (found)
            {
                throw new InvalidDataException(
                    $"JSON contains multiple case-insensitive matches for property '{propertyName}'.");
            }

            found = true;
            value = property.Value;
        }

        return found;
    }

    private static void ValidateRuntimeShape(AffixDefinition affix)
    {
        var rt = affix.Runtime;

        var actionElement = JsonSerializer.SerializeToElement(rt.Action);
        if (actionElement.ValueKind != JsonValueKind.Object)
        {
            throw new InvalidDataException($"{affix.Id}: runtime.action must be an object.");
        }

        if (!TryGetRequiredString(actionElement, "type", out var actionType) || !SupportedActionTypes.Contains(actionType))
        {
            throw new InvalidDataException($"{affix.Id}: runtime.action.type must be one of [{string.Join(", ", SupportedActionTypes)}].");
        }

        if (!SupportedTriggers.Contains(rt.Trigger))
        {
            throw new InvalidDataException($"{affix.Id}: runtime.trigger must be one of [{string.Join(", ", SupportedTriggers)}].");
        }

        if (rt.ProcChancePercent.HasValue && rt.ProcChancePercent.Value is < 0.0 or > 100.0)
        {
            throw new InvalidDataException($"{affix.Id}: runtime.procChancePercent must be in range 0..100 (got: {rt.ProcChancePercent.Value}).");
        }

        if (IsSpecialActionType(actionType) && (!rt.ProcChancePercent.HasValue || rt.ProcChancePercent.Value <= 0.0))
        {
            throw new InvalidDataException(
                $"{affix.Id}: special action {actionType} requires runtime.procChancePercent > 0 (use 100 for always-on).");
        }

        if (rt.IcdSeconds.HasValue && rt.IcdSeconds.Value < 0.0)
        {
            throw new InvalidDataException($"{affix.Id}: runtime.icdSeconds must be >= 0 (got: {rt.IcdSeconds.Value}).");
        }

        if (rt.PerTargetICDSeconds.HasValue && rt.PerTargetICDSeconds.Value < 0.0)
        {
            throw new InvalidDataException($"{affix.Id}: runtime.perTargetICDSeconds must be >= 0 (got: {rt.PerTargetICDSeconds.Value}).");
        }

    }

    private static bool IsSpecialActionType(string actionType)
    {
        return actionType is
            "CastOnCrit" or
            "ConvertDamage" or
            "MindOverMatter" or
            "Archmage" or
            "CorpseExplosion" or
            "SummonCorpseExplosion";
    }

    private static bool TryGetRequiredString(JsonElement parent, string key, out string value)
    {
        value = string.Empty;
        if (!parent.TryGetProperty(key, out var property) || property.ValueKind != JsonValueKind.String)
        {
            return false;
        }

        var raw = property.GetString();
        if (string.IsNullOrWhiteSpace(raw))
        {
            return false;
        }

        value = raw;
        return true;
    }

    private static bool TryGetOptionalNumber(JsonElement parent, string key, string affixId, out double value)
    {
        value = 0.0;
        if (!parent.TryGetProperty(key, out var property))
        {
            return false;
        }

        if (property.ValueKind != JsonValueKind.Number || !property.TryGetDouble(out value))
        {
            throw new InvalidDataException($"{affixId}: runtime.{key} must be a number.");
        }

        return true;
    }

    private static void ValidateOptionalNonNegativeNumber(JsonElement parent, string key, string affixId)
    {
        if (!TryGetOptionalNumber(parent, key, affixId, out var value))
        {
            return;
        }

        if (value < 0.0)
        {
            throw new InvalidDataException($"{affixId}: runtime.{key} must be >= 0 (got: {value}).");
        }
    }

    private static ValidationContract LoadValidationContract()
    {
        var path = FindValidationContractPath();
        if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
        {
            return GetFallbackContract();
        }

        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(path));
            var root = doc.RootElement;

            var triggers = ReadStringSet(root, "supportedTriggers");
            var actionTypes = ReadStringSet(root, "supportedActionTypes");
            return new ValidationContract(triggers, actionTypes);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(
                $"WARNING: Failed to load validation contract ({ex.Message}). Using hardcoded fallback.");
            return GetFallbackContract();
        }
    }

    private static string? FindValidationContractPath()
    {
        var starts = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        void AddStart(string? candidate)
        {
            if (!string.IsNullOrWhiteSpace(candidate))
            {
                starts.Add(Path.GetFullPath(candidate));
            }
        }

        AddStart(Directory.GetCurrentDirectory());
        AddStart(AppContext.BaseDirectory);

        foreach (var start in starts)
        {
            var current = start;
            while (!string.IsNullOrWhiteSpace(current))
            {
                var candidate = Path.Combine(current, ValidationContractRelativePath);
                if (File.Exists(candidate))
                {
                    return candidate;
                }

                var parent = Directory.GetParent(current);
                if (parent is null)
                {
                    break;
                }
                current = parent.FullName;
            }
        }

        return null;
    }

    private static HashSet<string> ReadStringSet(JsonElement root, string key)
    {
        if (!root.TryGetProperty(key, out var property) || property.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidDataException($"Validation contract missing array: {key}");
        }

        var output = new HashSet<string>(StringComparer.Ordinal);
        foreach (var item in property.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.String)
            {
                throw new InvalidDataException($"Validation contract '{key}' entries must be strings.");
            }

            var value = item.GetString();
            if (string.IsNullOrWhiteSpace(value))
            {
                throw new InvalidDataException($"Validation contract '{key}' contains an empty value.");
            }
            output.Add(value);
        }

        if (output.Count == 0)
        {
            throw new InvalidDataException($"Validation contract '{key}' must not be empty.");
        }

        return output;
    }

    private static ValidationContract GetFallbackContract()
    {
        return new ValidationContract(
            new HashSet<string>(StringComparer.Ordinal)
            {
                "Hit",
                "IncomingHit",
                "DotApply",
                "Kill",
                "LowHealth",
            },
            new HashSet<string>(StringComparer.Ordinal)
            {
                "DebugNotify",
                "CastSpell",
                "CastSpellAdaptiveElement",
                "CastOnCrit",
                "ConvertDamage",
                "MindOverMatter",
                "Archmage",
                "CorpseExplosion",
                "SummonCorpseExplosion",
                "SpawnTrap",
            });
    }

    private sealed record ValidationContract(HashSet<string> SupportedTriggers, HashSet<string> SupportedActionTypes);

    private static void ValidateModKey(string modKey)
    {
        if (string.IsNullOrWhiteSpace(modKey))
        {
            throw new InvalidDataException("modKey must be a non-empty file name.");
        }

        if (Path.IsPathRooted(modKey))
        {
            throw new InvalidDataException($"modKey must not be rooted: {modKey}");
        }

        if (!string.Equals(modKey, Path.GetFileName(modKey), StringComparison.Ordinal))
        {
            throw new InvalidDataException($"modKey must be a file name only (no path segments): {modKey}");
        }

        if (modKey.IndexOfAny(InvalidModKeyFileNameChars) >= 0)
        {
            throw new InvalidDataException($"modKey contains invalid file name characters: {modKey}");
        }

        if (modKey.EndsWith(' ') || modKey.EndsWith('.'))
        {
            throw new InvalidDataException($"modKey must not end with space or dot: {modKey}");
        }
    }
}
